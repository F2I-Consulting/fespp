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

VtkWellboreFrame::~VtkWellboreFrame()
{
	for (auto& marker : uuid_to_VtkWellboreMarker) {
		delete marker.second;
	}
	/*
	for (auto& marker : uuid_to_VtkWellboreChannel) {
		delete marker.second;
	}
	*/
}

//----------------------------------------------------------------------------
void VtkWellboreFrame::createTreeVtk(const std::string & uuid, const std::string & uuidParent, const std::string & name, VtkEpcCommon::Resqml2Type type)
{
	if (type == VtkEpcCommon::Resqml2Type::WELL_MARKER) {
		uuid_to_VtkWellboreMarker[uuid] = new VtkWellboreMarker(getFileName(), name, uuid, uuidParent, repositoryRepresentation, repositorySubRepresentation);
	}
	/*
	else if (type == VtkEpcCommon::Resqml2Type::PROPERTY) {
		uuid_to_VtkWellboreChannel[uuid] = new VtkWellboreMarker(getFileName(), name, uuid, uuidParent, repositoryRepresentation, repositorySubRepresentation);
	}
	*/
}

//----------------------------------------------------------------------------
void VtkWellboreFrame::visualize(const std::string & uuid)
{
	// MARKER
	auto vtkMarkerIt = uuid_to_VtkWellboreMarker.find(uuid);
	if (vtkMarkerIt != uuid_to_VtkWellboreMarker.end()) {
		auto vtkMarker = vtkMarkerIt->second;
		vtkMarker->visualize(uuid);

		if (getBlockNumberOf(vtkMarker->getOutput()) == (std::numeric_limits<unsigned int>::max)()) {
			unsigned int blockCount = vtkOutput->GetNumberOfBlocks();
			vtkOutput->SetBlock(blockCount, vtkMarker->getOutput());
			vtkOutput->GetMetaData(blockCount)->Set(vtkCompositeDataSet::NAME(), vtkMarker->getName().c_str());
		}

		return;
	}

	/*
	// CHANNEL
	auto vtkChannelIt = uuid_to_VtkWellboreChannel.find(uuid);
	if (vtkChannelIt != uuid_to_VtkWellboreChannel.end()) {
		auto vtkChannel = vtkChannelIt->second;
		vtkChannel->visualize(uuid);

		if (getBlockNumberOf(vtkChannel->getOutput()) == (std::numeric_limits<unsigned int>::max)()) {
			unsigned int blockCount = vtkOutput->GetNumberOfBlocks();
			vtkOutput->SetBlock(blockCount, vtkChannel->getOutput());
			vtkOutput->GetMetaData(blockCount)->Set(vtkCompositeDataSet::NAME(), vtkChannel->getName().c_str());
		}

		return;
	}
	*/
}

//----------------------------------------------------------------------------
void VtkWellboreFrame::toggleMarkerOrientation(bool orientation)
{
	for (auto &marker : uuid_to_VtkWellboreMarker) {
		// Check if the marker is visible or not.
		if (marker.second->getOutput() != nullptr) {
			remove(marker.first);
			marker.second->toggleMarkerOrientation(orientation);
			visualize(marker.first);
		}
		else {
			marker.second->toggleMarkerOrientation(orientation);
		}
	}
}

//----------------------------------------------------------------------------
void VtkWellboreFrame::setMarkerSize(int size)
{
	for (auto &marker : uuid_to_VtkWellboreMarker) {
		// Check if the marker is visible or not.
		if (marker.second->getOutput() != nullptr) {
			remove(marker.first);
			marker.second->setMarkerSize(size);
			visualize(marker.first);
		}
		else {
			marker.second->setMarkerSize(size);
		}
	}
}

//----------------------------------------------------------------------------
void VtkWellboreFrame::remove(const std::string & uuid)
{
	if (uuid == getUuid()) {
		vtkOutput = nullptr;
		return;
	}

	// MARKER
	auto vtkMarkerIt = uuid_to_VtkWellboreMarker.find(uuid);
	if (vtkMarkerIt != uuid_to_VtkWellboreMarker.end()) {
		removeFromVtkOutput(vtkMarkerIt->second->getOutput());
		vtkMarkerIt->second->remove(uuid);
	}

	/*
	// CHANNEL
	auto vtkChannelIt = uuid_to_VtkWellboreChannel.find(uuid);
	if (vtkChannelIt != vtkChannelIt.end()) {
		removeFromVtkOutput(vtkChannelIt->second->getOutput());
		vtkChannelIt->second->remove(uuid);
	}
	*/
}
