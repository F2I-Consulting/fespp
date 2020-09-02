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
VtkWellboreTrajectoryRepresentation::~VtkWellboreTrajectoryRepresentation()
{
	if (repositoryRepresentation != nullptr) {
		repositoryRepresentation = nullptr;
	}

	if (repositorySubRepresentation != nullptr) {
		repositorySubRepresentation = nullptr;
	}
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
int VtkWellboreTrajectoryRepresentation::createOutput(const std::string & uuid)
{
	polyline.visualize(uuid);
	return 1;
}
//----------------------------------------------------------------------------
void VtkWellboreTrajectoryRepresentation::visualize(const std::string & uuid)
{
	std::string uuid_to_attach = uuid;

	// detach all representation to multiblock
	detach();

	// wellboreTrajectory representation
	createOutput(getUuid());
	if (attachUuids.empty()) {
		attachUuids.push_back(getUuid());
	}

	// add uuids's wellboreFrame
	if (uuid_Informations[uuid].getType() == VtkEpcCommon::Resqml2Type::WELL_MARKER_FRAME ||
			uuid_Informations[uuid].getType() == VtkEpcCommon::Resqml2Type::WELL_FRAME) {
		uuid_to_VtkWellboreFrame[uuid]->visualize(uuid);
	}
	else if (uuid_Informations[uuid].getType() == VtkEpcCommon::Resqml2Type::WELL_MARKER) {
		uuid_to_VtkWellboreFrame[uuid_Informations[uuid].getParent()]->visualize(uuid);
		uuid_to_attach = uuid_Informations[uuid].getParent();
	}

	// attach representation to multiblock
	if (std::find(attachUuids.begin(), attachUuids.end(), uuid_to_attach) == attachUuids.end()) {
		attachUuids.push_back(uuid_to_attach);
	}
	attach();
}

//----------------------------------------------------------------------------
void VtkWellboreTrajectoryRepresentation::toggleMarkerOrientation(const bool & orientation) {
	// Iterate over an unordered_map using range based for loop
	for (std::pair<std::string, VtkEpcCommon> element : uuid_Informations) {
		if (element.second.getType() == VtkEpcCommon::Resqml2Type::WELL_MARKER){
			uuid_to_VtkWellboreFrame[element.second.getParent()]->toggleMarkerOrientation(orientation);
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
		detach();
		attachUuids.erase(std::find(attachUuids.begin(), attachUuids.end(), uuid));
		attach();
	}
	else if (uuid_Informations[uuid].getType() == VtkEpcCommon::Resqml2Type::WELL_MARKER) {
		uuid_to_VtkWellboreFrame[uuid_Informations[uuid].getParent()]->remove(uuid);
	}
}

//----------------------------------------------------------------------------
void VtkWellboreTrajectoryRepresentation::attach()
{
	if (attachUuids.size() > (std::numeric_limits<unsigned int>::max)()) {
		throw std::range_error("Too much attached uuids");
	}

	for (unsigned int newBlockIndex = 0; newBlockIndex < attachUuids.size(); ++newBlockIndex) {
		std::string uuid = attachUuids[newBlockIndex];
		if (uuid == getUuid())	{
			vtkOutput->SetBlock(newBlockIndex, polyline.getOutput());
			vtkOutput->GetMetaData(newBlockIndex)->Set(vtkCompositeDataSet::NAME(), polyline.getName().c_str());
		}
		else {
			vtkOutput->SetBlock(newBlockIndex, uuid_to_VtkWellboreFrame[uuid]->getOutput());
			vtkOutput->GetMetaData(newBlockIndex)->Set(vtkCompositeDataSet::NAME(), uuid_to_VtkWellboreFrame[uuid]->getName().c_str());
		}
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
