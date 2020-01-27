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

// FESAPI
#include <fesapi/common/EpcDocument.h>
#include <fesapi/resqml2_0_1/WellboreTrajectoryRepresentation.h>

// FESPP
#include "VtkWellboreTrajectoryRepresentationDatum.h"
#include "VtkWellboreTrajectoryRepresentationText.h"
#include "VtkWellboreTrajectoryRepresentationPolyLine.h"

//----------------------------------------------------------------------------
VtkWellboreTrajectoryRepresentation::VtkWellboreTrajectoryRepresentation(const std::string & fileName, const std::string & name, const std::string & uuid, const std::string & uuidParent, COMMON_NS::DataObjectRepository *repoRepresentation, COMMON_NS::DataObjectRepository *repoSubRepresentation) :
VtkResqml2MultiBlockDataSet(fileName, name, uuid, uuidParent), repositoryRepresentation(repoRepresentation), repositorySubRepresentation(repoSubRepresentation),
polyline(getFileName(), name, uuid+"-Polyline", uuidParent, repoRepresentation, repoSubRepresentation),
head(getFileName(), name, uuid+"-Head", uuidParent, repoRepresentation, repoSubRepresentation),
text(getFileName(), name, uuid+"-Text", uuidParent, repoRepresentation, repoSubRepresentation)
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
	if (uuid != getUuid())	{
		polyline.createTreeVtk(uuid, uuidParent, name, type);
	}
}

//----------------------------------------------------------------------------
int VtkWellboreTrajectoryRepresentation::createOutput(const std::string & uuid)
{
	polyline.createOutput(uuid);
	//	head.createOutput(uuid);
	return 1;
}
//----------------------------------------------------------------------------
void VtkWellboreTrajectoryRepresentation::visualize(const std::string & uuid)
{
	createOutput(uuid);

	attach();
}

//----------------------------------------------------------------------------
void VtkWellboreTrajectoryRepresentation::remove(const std::string & uuid)
{
	if (uuid == getUuid()) {
		detach();
		std::stringstream polylineUuid;
		polylineUuid << getUuid() << "-Polyline";
		polyline.remove(uuid);
	}
}

//----------------------------------------------------------------------------
void VtkWellboreTrajectoryRepresentation::attach()
{
	unsigned int index =0;
	vtkOutput->SetBlock(index, polyline.getOutput());
	vtkOutput->GetMetaData(index++)->Set(vtkCompositeDataSet::NAME(),polyline.getName().c_str());

	vtkOutput->SetBlock(index, head.getOutput());
	vtkOutput->GetMetaData(index++)->Set(vtkCompositeDataSet::NAME(),head.getName().c_str());
}

//----------------------------------------------------------------------------
void VtkWellboreTrajectoryRepresentation::addProperty(const std::string & uuidProperty, vtkDataArray* dataProperty)
{
	polyline.addProperty(uuidProperty, dataProperty);
}

//----------------------------------------------------------------------------
long VtkWellboreTrajectoryRepresentation::getAttachmentPropertyCount(const std::string & uuid, VtkEpcCommon::FesppAttachmentProperty propertyUnit)
{
	return 	 polyline.getAttachmentPropertyCount(uuid, propertyUnit);
}
