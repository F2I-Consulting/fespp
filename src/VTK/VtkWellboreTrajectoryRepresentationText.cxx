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
#include "VtkWellboreTrajectoryRepresentationText.h"

// FESAPI
#include <fesapi/common/EpcDocument.h>

// FESPP
#include "VtkResqml2PolyData.h"

//----------------------------------------------------------------------------
VtkWellboreTrajectoryRepresentationText::VtkWellboreTrajectoryRepresentationText(const std::string & fileName, const std::string & name, const std::string & uuid, const std::string & uuidParent, COMMON_NS::DataObjectRepository *repoRepresentation, COMMON_NS::DataObjectRepository *repoSubRepresentation) :
VtkResqml2PolyData(fileName, name, uuid, uuidParent, repoRepresentation, repoSubRepresentation)
{
}

//----------------------------------------------------------------------------
void VtkWellboreTrajectoryRepresentationText::createOutput(const std::string & uuid)
{
}

//----------------------------------------------------------------------------
void VtkWellboreTrajectoryRepresentationText::addProperty(const std::string & uuidProperty, vtkDataArray* dataProperty)
{
}

long VtkWellboreTrajectoryRepresentationText::getAttachmentPropertyCount(const std::string & uuid, VtkEpcCommon::FesppAttachmentProperty propertyUnit)
{
	return 0;
}


