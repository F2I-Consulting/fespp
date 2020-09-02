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
#include "VtkWellboreFrame.h"

// FESAPI
#include <fesapi/resqml2/WellboreMarker.h>
#include <fesapi/resqml2/WellboreMarkerFrameRepresentation.h>

#include <vtkInformation.h>
#include "VtkWellboreMarker.h"



//----------------------------------------------------------------------------
VtkWellboreFrame::VtkWellboreFrame(const std::string & fileName, const std::string & name, const std::string & uuid, const std::string & uuidParent, const COMMON_NS::DataObjectRepository *repoRepresentation, const COMMON_NS::DataObjectRepository *repoSubRepresentation) :
VtkResqml2MultiBlockDataSet(fileName, name, uuid, uuidParent), repositoryRepresentation(repoRepresentation), repositorySubRepresentation(repoSubRepresentation)
{
}

//----------------------------------------------------------------------------
void VtkWellboreFrame::createTreeVtk(const std::string & uuid, const std::string & uuidParent, const std::string & name, VtkEpcCommon::Resqml2Type type)
{
	if (type == VtkEpcCommon::Resqml2Type::WELL_MARKER) {
		uuid_to_VtkWellboreMarker[uuid] = new VtkWellboreMarker(getFileName(), name, uuid, uuidParent, repositoryRepresentation, repositorySubRepresentation);
	}
}

//----------------------------------------------------------------------------
void VtkWellboreFrame::visualize(const std::string & uuid)
{
	// detach all representation to multiblock
	detach();

	// create representation
	if (uuid == getUuid()) {
		attachUuids.clear();
		// Traversing an unordered map
		for (auto marker : uuid_to_VtkWellboreMarker) {
			marker.second->visualize(uuid);
			attachUuids.push_back(marker.first);
		}
	} else {
		uuid_to_VtkWellboreMarker[uuid]->visualize(uuid);
		attachUuids.push_back(uuid);
	}

	// attach all representation to multiblock
	attach();
}

//----------------------------------------------------------------------------
void VtkWellboreFrame::toggleMarkerOrientation(const bool & orientation) {

	for (auto &marker : uuid_to_VtkWellboreMarker) {
		marker.second->toggleMarkerOrientation(orientation);
	}
}

//----------------------------------------------------------------------------
void VtkWellboreFrame::attach()
{
	if (attachUuids.size() > (std::numeric_limits<unsigned int>::max)()) {
		throw std::range_error("Too much attached uuids");
	}

	for (unsigned int newBlockIndex = 0; newBlockIndex < attachUuids.size(); ++newBlockIndex) {
		std::string uuid = attachUuids[newBlockIndex];
		vtkOutput->SetBlock(newBlockIndex, uuid_to_VtkWellboreMarker[uuid]->getOutput());
		vtkOutput->GetMetaData(newBlockIndex)->Set(vtkCompositeDataSet::NAME(), uuid_to_VtkWellboreMarker[uuid]->getUuid().c_str());
	}
}

void VtkWellboreFrame::remove(const std::string & uuid)
{
	if (uuid == getUuid()) {
		vtkOutput = nullptr;
	}	else { // => wellbore_Marker
		detach();
		attachUuids.erase(std::find(attachUuids.begin(), attachUuids.end(), uuid));
		attach();
	}
}
