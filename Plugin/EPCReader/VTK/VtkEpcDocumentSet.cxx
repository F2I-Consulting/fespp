﻿/*-----------------------------------------------------------------------
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
#include "VtkEpcDocumentSet.h"

// include system
#include <algorithm>
#include <sstream>

// include Vtk
#include <vtkInformation.h>
#include <vtkSmartPointer.h>
#include <vtkMultiBlockDataSet.h>

#include "VtkEpcDocument.h"

//----------------------------------------------------------------------------
VtkEpcDocumentSet::VtkEpcDocumentSet(int idProc, int maxProc, VtkEpcCommon::modeVtkEpc mode) :
	vtkOutput(vtkSmartPointer<vtkMultiBlockDataSet>::New()),
	procRank(idProc), nbProc(maxProc),
	treeViewMode(mode == VtkEpcCommon::modeVtkEpc::Both || mode == VtkEpcCommon::modeVtkEpc::TreeView),
	representationMode(mode == VtkEpcCommon::modeVtkEpc::Both || mode == VtkEpcCommon::modeVtkEpc::Representation)
{}

//----------------------------------------------------------------------------
VtkEpcDocumentSet::~VtkEpcDocumentSet()
{
	for (const auto vtkEpc : vtkEpcList) {
		delete vtkEpc;
	}
}

//----------------------------------------------------------------------------
std::string VtkEpcDocumentSet::visualize(const std::string& uuid)
{
	if (std::find(badUuid.begin(), badUuid.end(), uuid) == badUuid.end()) {
		if (representationMode) {
			try {
				uuidToVtkEpc.at(uuid)->visualize(uuid);
			}
			catch (const std::out_of_range& oor) {
				badUuid.push_back(uuid);
				return "Out of range exception when trying to visualize UUID " + uuid + " of file " + uuidToVtkEpc[uuid]->getFileName() + " : " + oor.what();
			}
			catch (const std::exception& e) {
				badUuid.push_back(uuid);
				return "EXCEPTION (probably in FESAPI) when trying to visualize UUID " + uuid + " of file " + uuidToVtkEpc[uuid]->getFileName() + " : " + e.what();
			}
		}
		return "";
	}

	return "This object cannot be visualized";
}

//----------------------------------------------------------------------------
void VtkEpcDocumentSet::visualizeFull()
{
	if (representationMode) {
		for (auto vtkEpcElem : vtkEpcList) {
			for (const auto &uuidListElem : vtkEpcElem->getListUuid()) {
				vtkEpcElem->visualize(uuidListElem);
			}
		}
	}
}

//----------------------------------------------------------------------------
void VtkEpcDocumentSet::visualizeFullWell(const std::string& fileName)
{
	if (representationMode) {
		for (const auto vtkEpcElem : vtkEpcList) {
			if (vtkEpcElem->getFileName() == fileName) 	{
				vtkEpcElem->visualizeFullWell();
				break;
			}
		}
	}
}

//----------------------------------------------------------------------------
void VtkEpcDocumentSet::unvisualize(const std::string &uuid)
{
	if (representationMode) {
		// Only unvisualize uuid which are referenced
		// For example do not unvizualize an uuid which is an epc filename.
		auto iterator = uuidToVtkEpc.find(uuid);
		if (iterator != uuidToVtkEpc.end()) {
			iterator->second->remove(uuid);
		}
	}
}

//----------------------------------------------------------------------------
void VtkEpcDocumentSet::unvisualizeFullWell(const std::string& fileName)
{
	if (representationMode) {
		for (auto vtkEpcElem : vtkEpcList) {
			if (vtkEpcElem->getFileName() == fileName) {
				vtkEpcElem->unvisualizeFullWell();
				break;
			}
		}
	}
}

//----------------------------------------------------------------------------
void VtkEpcDocumentSet::toggleMarkerOrientation(const bool orientation)
{
	if (representationMode) {
		for (VtkEpcDocument *epcDoc : vtkEpcList) {
			epcDoc->toggleMarkerOrientation(orientation);
		}
	}
}

//----------------------------------------------------------------------------
void VtkEpcDocumentSet::setMarkerSize(int size) {
	if (representationMode) {
		for (VtkEpcDocument *epcDoc : vtkEpcList) {
			epcDoc->setMarkerSize(size);
		}
	}
}


//----------------------------------------------------------------------------
VtkEpcCommon::Resqml2Type VtkEpcDocumentSet::getType(std::string uuid)
{
	return uuidToVtkEpc[uuid]->getType(uuid);
}

//----------------------------------------------------------------------------
VtkEpcCommon VtkEpcDocumentSet::getInfoUuid(std::string uuid)
{
	return uuidToVtkEpc[uuid]->getInfoUuid(uuid);
}

//----------------------------------------------------------------------------
vtkSmartPointer<vtkMultiBlockDataSet> VtkEpcDocumentSet::getVisualization() const
{
	if (representationMode) {
		vtkOutput->Initialize();
		unsigned int index = 0;
		for (const auto vtkEpcElem : vtkEpcList) {
			if (vtkEpcElem->getOutput()->GetNumberOfBlocks() > 0) {
				vtkOutput->SetBlock(index, vtkEpcElem->getOutput());
				vtkOutput->GetMetaData(index++)->Set(vtkCompositeDataSet::NAME(), vtkEpcElem->getFileName().c_str());
			}
		}
	}

	return vtkOutput;
}

//----------------------------------------------------------------------------
std::string VtkEpcDocumentSet::addEpcDocument(const std::string& fileName)
{
	// Check if the file relative to fileName has already been imported
	for (auto epcDoc : vtkEpcList) {
		if (epcDoc->getFileName() == fileName) {
			return "The file \"" + fileName +"\" has already been imported";
		}
	}

	// Import the file relative to fileName
	auto vtkEpc = new VtkEpcDocument(fileName, procRank, nbProc, this);
	auto uuidList = vtkEpc->getListUuid();
	for (const auto& uuidListElem : uuidList) {
		uuidToVtkEpc[uuidListElem] = vtkEpc;
	}
	vtkEpcList.push_back(vtkEpc);

	return vtkEpc->getError();
}

//----------------------------------------------------------------------------
VtkEpcDocument* VtkEpcDocumentSet::getVtkEpcDocument(const std::string& uuid)
{
	auto result = uuidToVtkEpc.find(uuid);
	return result != uuidToVtkEpc.end() ? result->second : nullptr;
}

//----------------------------------------------------------------------------
VtkEpcCommon::Resqml2Type VtkEpcDocumentSet::getTypeInEpcDocument(const std::string& uuid)
{
	auto vtkEpcDoc = getVtkEpcDocument(uuid);
	return vtkEpcDoc != nullptr
		? vtkEpcDoc->getType(uuid)
		: VtkEpcCommon::Resqml2Type::UNKNOW;
}

//----------------------------------------------------------------------------
std::vector<VtkEpcCommon const *> VtkEpcDocumentSet::getAllVtkEpcCommons() const
{
	std::vector<VtkEpcCommon const *> result;

	// concatenate all VtkEpcCommons of all epc documents (i.e repos).
	for (auto vtkRepo : vtkEpcList) {
		auto repoAllVtkEpcCommons = vtkRepo->getAllVtkEpcCommons();
		result.insert(result.end(),
					  repoAllVtkEpcCommons.begin(), repoAllVtkEpcCommons.end());
	}

	return result;
}
