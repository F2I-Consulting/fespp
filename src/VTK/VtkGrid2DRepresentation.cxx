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
#include "VtkGrid2DRepresentation.h"

#include <sstream>

// include VTK library
#include <vtkInformation.h>

// include F2i-consulting Energistics Standards API
#include <fesapi/common/EpcDocument.h>

// include F2i-consulting Energistics Standards ParaView Plugin
#include "VtkGrid2DRepresentationPoints.h"
#include "VtkGrid2DRepresentationCells.h"

//----------------------------------------------------------------------------
VtkGrid2DRepresentation::VtkGrid2DRepresentation(const std::string & fileName, const std::string & name, const std::string & uuid, const std::string & uuidParent, COMMON_NS::DataObjectRepository const * pckEPCRep, COMMON_NS::DataObjectRepository const * pckEPCSubRep) :
VtkResqml2MultiBlockDataSet(fileName, name, uuid, uuidParent), repositoryRepresentation(pckEPCRep), repositorySubRepresentation(pckEPCSubRep), grid2DPoints(getFileName(), name, uuid+"-Points", uuidParent, repositoryRepresentation, repositorySubRepresentation)
{
	//BUG in PARAVIEW
	//	std::stringstream grid2DCellsUuid;
	//	grid2DCellsUuid << uuid << "-Cells";
	//
	//	grid2DCells = new VtkGrid2DRepresentationCells(getFileName(), name, grid2DCellsUuid.str(), uuidParent, epcPackage);
}

VtkGrid2DRepresentation::~VtkGrid2DRepresentation()
{
	if (repositoryRepresentation != nullptr) {
		repositoryRepresentation = nullptr;
	}

	if (repositorySubRepresentation != nullptr) {
		repositorySubRepresentation = nullptr;
	}

	/*
	if (grid2DCells != nullptr) {
		delete grid2DCells;
		grid2DCells = nullptr;
	}
	 */
}

//----------------------------------------------------------------------------
void VtkGrid2DRepresentation::createTreeVtk(const std::string & uuid, const std::string & uuidParent, const std::string & name, VtkEpcCommon::Resqml2Type type)
{
	if (uuid != getUuid()) {
		grid2DPoints.createTreeVtk(uuid, uuidParent, name, type);
		//		grid2DCells->createTreeVtk(uuid, uuidParent, name, type);
	}
}

//----------------------------------------------------------------------------
int VtkGrid2DRepresentation::createOutput(const std::string & uuid)
{
	grid2DPoints.visualize(uuid);
	//	grid2DCells->visualize(uuid);

	return 1;
}

//----------------------------------------------------------------------------
void VtkGrid2DRepresentation::visualize(const std::string & uuid)
{
	createOutput(uuid);

	attach();
}

//----------------------------------------------------------------------------
void VtkGrid2DRepresentation::remove(const std::string & uuid)
{
	if (uuid == getUuid()) {
		detach();

		grid2DPoints.remove(uuid);

		//	std::stringstream grid2DCellsUuid;
		//	grid2DCellsUuid << uuid << "-Cells";
		//	
		//	grid2DCells = new VtkGrid2DRepresentationCells(getFileName(), getName(), grid2DCellsUuid.str(), getParent(), epcPackage);
	}
}

//----------------------------------------------------------------------------
void VtkGrid2DRepresentation::attach()
{
	unsigned int index =0;

	vtkOutput->SetBlock(index, grid2DPoints.getOutput());
	vtkOutput->GetMetaData(index++)->Set(vtkCompositeDataSet::NAME(),grid2DPoints.getUuid().c_str());

	//	vtkOutput->SetBlock(index, grid2DCells->getOutput());
	//	vtkOutput->GetMetaData(index++)->Set(vtkCompositeDataSet::NAME(),grid2DCells->getUuid().c_str());
}

void VtkGrid2DRepresentation::addProperty(const std::string & uuidProperty, vtkDataArray* dataProperty)
{
	grid2DPoints.addProperty(uuidProperty, dataProperty);
}

long VtkGrid2DRepresentation::getAttachmentPropertyCount(const std::string & uuid, VtkEpcCommon::FesppAttachmentProperty propertyUnit)
{
	return grid2DPoints.getAttachmentPropertyCount(uuid, propertyUnit);
}
