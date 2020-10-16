/*-----------------------------------------------------------------------
Licensed to the Apache Software Foundation (ASF) under one
or more contributor license agreements.  See the NOTICE file
distributed with this work for additional information
regarding copyright ownership.  The ASF licenses this file
to you under the Apache License, Version 2.0 (the
"License"; you may not use this file except in compliance
with the License.  You may obtain a copy of the License at

  http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing,
software distributed under the License is distributed on an
"AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
KIND, either express or implied.  See the License for the
specific language governing permissions and limitations
under the License.
-----------------------------------------------------------------------*/
#include "VtkWellboreTrajectoryRepresentation.h"

//SYSTEM
#include <sstream>

// VTK
#include <vtkInformation.h>

// FESPP
#include "VtkWellboreTrajectoryRepresentationPolyLine.h"

//----------------------------------------------------------------------------
VtkWellboreTrajectoryRepresentation::VtkWellboreTrajectoryRepresentation(const std::string & fileName, const std::string & name, const std::string & uuid, const std::string & uuidParent, COMMON_NS::DataObjectRepository const * repoRepresentation, COMMON_NS::DataObjectRepository const * repoSubRepresentation) :
	VtkResqml2MultiBlockDataSet(fileName, name, uuid, uuidParent), repositoryRepresentation(repoRepresentation), repositorySubRepresentation(repoSubRepresentation),
	polyline(getFileName(), name, uuid+"-Polyline", uuidParent, repoRepresentation, repoSubRepresentation)
{
}

//----------------------------------------------------------------------------
void VtkWellboreTrajectoryRepresentation::createTreeVtk(const std::string & uuid, const std::string & uuidParent, const std::string & name, VtkEpcCommon::Resqml2Type type)
{
	VtkEpcCommon informations(uuid, uuidParent, name, type);
	uuid_Informations[uuid] = informations;
	if (type == VtkEpcCommon::Resqml2Type::WELL_FRAME || type == VtkEpcCommon::Resqml2Type::WELL_MARKER_FRAME )	{
		uuid_to_VtkWellboreFrame[uuid] = new VtkWellboreFrame(getFileName(), name, uuid, uuidParent, repositoryRepresentation, repositorySubRepresentation);
	}
	else if (type == VtkEpcCommon::Resqml2Type::WELL_MARKER) {
		uuid_to_VtkWellboreFrame[uuidParent]->createTreeVtk(uuid, uuidParent, name, type);
	}
}

//----------------------------------------------------------------------------
void VtkWellboreTrajectoryRepresentation::visualize(const std::string & uuid)
{
	// Create an empty output if necessary
	// An empty output is automatically created in the (super) constructor.
	// However it is deleted when we remove the trajectory from the display.
	if (vtkOutput == nullptr) {
		vtkOutput = vtkSmartPointer<vtkMultiBlockDataSet>::New();
	}

	// Build the VTK vtkPolyData which will be displayed
	if (!polyline.getOutput()) {
		polyline.visualize(getUuid());
	}
	// Add the built VTK vtkPolyData to the multiblock vtk output
	unsigned int blockCount = vtkOutput->GetNumberOfBlocks();
	if (blockCount == 0) {
		vtkOutput->SetBlock(blockCount, polyline.getOutput());
		vtkOutput->GetMetaData(blockCount)->Set(vtkCompositeDataSet::NAME(), polyline.getName().c_str());
	}

	// add wellbore Marker and/or Frame
	VtkEpcCommon uuidInfo = uuid_Informations[uuid];
	if (uuidInfo.getType() == VtkEpcCommon::Resqml2Type::WELL_MARKER_FRAME ||
		uuidInfo.getType() == VtkEpcCommon::Resqml2Type::WELL_FRAME ||
		uuidInfo.getType() == VtkEpcCommon::Resqml2Type::WELL_MARKER) {
		VtkWellboreFrame* frame = uuidInfo.getType() == VtkEpcCommon::Resqml2Type::WELL_MARKER
			? uuid_to_VtkWellboreFrame[uuidInfo.getParent()]
			: uuid_to_VtkWellboreFrame[uuid];
		frame->visualize(uuid);
		if (getBlockNumberOf(frame->getOutput()) == (std::numeric_limits<unsigned int>::max)()) {
			blockCount = vtkOutput->GetNumberOfBlocks();
			vtkOutput->SetBlock(blockCount, frame->getOutput());
			vtkOutput->GetMetaData(blockCount)->Set(vtkCompositeDataSet::NAME(), frame->getName().c_str());
		}
	}
}

//----------------------------------------------------------------------------
void VtkWellboreTrajectoryRepresentation::toggleMarkerOrientation(bool orientation) {
	// Iterate over an unordered_map using range based for loop
	for (std::pair<std::string, VtkEpcCommon> element : uuid_Informations) {
		if (element.second.getType() == VtkEpcCommon::Resqml2Type::WELL_MARKER) {
			uuid_to_VtkWellboreFrame[element.second.getParent()]->toggleMarkerOrientation(orientation);
		}
	}
}

//----------------------------------------------------------------------------
void VtkWellboreTrajectoryRepresentation::setMarkerSize(int size) {
	// Iterate over an unordered_map using range based for loop
	for (std::pair<std::string, VtkEpcCommon> element : uuid_Informations) {
		if (element.second.getType() == VtkEpcCommon::Resqml2Type::WELL_MARKER) {
			uuid_to_VtkWellboreFrame[element.second.getParent()]->setMarkerSize(size);
		}
	}
}

//----------------------------------------------------------------------------
void VtkWellboreTrajectoryRepresentation::remove(const std::string & uuid)
{
	if (uuid == getUuid()) {
		vtkOutput = nullptr;
	}
	else if (uuid_Informations[uuid].getType() == VtkEpcCommon::Resqml2Type::WELL_MARKER_FRAME ||
			uuid_Informations[uuid].getType() == VtkEpcCommon::Resqml2Type::WELL_FRAME) {
		removeFromVtkOutput(uuid_to_VtkWellboreFrame[uuid]->getOutput());
	}
	else if (uuid_Informations[uuid].getType() == VtkEpcCommon::Resqml2Type::WELL_MARKER) {
		uuid_to_VtkWellboreFrame[uuid_Informations[uuid].getParent()]->remove(uuid);
	}
}

//----------------------------------------------------------------------------
void VtkWellboreTrajectoryRepresentation::addProperty(const std::string & uuidProperty, vtkDataArray* dataProperty)
{
	polyline.addProperty(uuidProperty, dataProperty);
}

//----------------------------------------------------------------------------
long VtkWellboreTrajectoryRepresentation::getAttachmentPropertyCount(const std::string & uuid, VtkEpcCommon::FesppAttachmentProperty propertyUnit)
{
	return polyline.getAttachmentPropertyCount(uuid, propertyUnit);
}
