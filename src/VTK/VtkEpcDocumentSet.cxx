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
	treeViewMode(mode == VtkEpcCommon::Both || mode == VtkEpcCommon::TreeView),
	representationMode(mode == VtkEpcCommon::Both || mode == VtkEpcCommon::Representation)
{
}

//----------------------------------------------------------------------------
VtkEpcDocumentSet::~VtkEpcDocumentSet()
{
	for (const auto& vtkEpc : vtkEpcList) {
		delete vtkEpc;
	}
}

//----------------------------------------------------------------------------
std::string VtkEpcDocumentSet::visualize(const std::string & uuid)
{
	if (representationMode) {
		try {
			uuidToVtkEpc[uuid]->visualize(uuid);
		}
		catch  (const std::exception & e) {
			return "EXCEPTION in fesapi " + uuidToVtkEpc[uuid]->getFileName() + " : " + e.what();
		}
	}
	return std::string();
}

//----------------------------------------------------------------------------
void VtkEpcDocumentSet::visualizeFull()
{
	if(representationMode) {
		for (auto &vtkEpcElem : vtkEpcList) {
			auto uuidList = vtkEpcElem->getListUuid();
			for (const auto& uuidListElem : uuidList)	{
				vtkEpcElem->visualize(uuidListElem);
			}
		}
	}
}

//----------------------------------------------------------------------------
void VtkEpcDocumentSet::visualizeFullWell(std::string fileName)
{
	if(representationMode) {
		for (const auto& vtkEpcElem : vtkEpcList) {
			if (vtkEpcElem->getFileName() == fileName) {
				vtkEpcElem->visualizeFullWell();
				break;
			}
		}
	}
}

//----------------------------------------------------------------------------
void VtkEpcDocumentSet::unvisualize(const std::string & uuid)
{
	if(representationMode) {
		uuidToVtkEpc[uuid]->remove(uuid);
	}
}

//----------------------------------------------------------------------------
void VtkEpcDocumentSet::unvisualizeFullWell(std::string fileName)
{
	if(representationMode) {
		for (auto &vtkEpcElem : vtkEpcList) {
			if (vtkEpcElem->getFileName() == fileName) {
				vtkEpcElem->unvisualizeFullWell();
				break;
			}
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
	return  uuidToVtkEpc[uuid]->getInfoUuid(uuid);
}

//----------------------------------------------------------------------------
vtkSmartPointer<vtkMultiBlockDataSet> VtkEpcDocumentSet::getVisualization() const
{
	if(representationMode) {
		vtkOutput->Initialize();
		unsigned int index = 0;
		for (auto &vtkEpcElem : vtkEpcList) {
			if(vtkEpcElem->getOutput()->GetNumberOfBlocks()>0) {
				vtkOutput->SetBlock(index, vtkEpcElem->getOutput());
				vtkOutput->GetMetaData(index++)->Set(vtkCompositeDataSet::NAME(), vtkEpcElem->getFileName().c_str());
			}
		}
	}
	return vtkOutput;
}

//----------------------------------------------------------------------------
std::string VtkEpcDocumentSet::addEpcDocument(const std::string & fileName)
{
	if (std::find(vtkEpcNameList.begin(), vtkEpcNameList.end(),fileName)==vtkEpcNameList.end())	{
		auto vtkEpc = new VtkEpcDocument(fileName, procRank, nbProc, this);
		auto uuidList = vtkEpc->getListUuid();
		for (auto &uuidListElem : uuidList) {
			uuidToVtkEpc[uuidListElem] = vtkEpc;
		}
		vtkEpcList.push_back(vtkEpc);
		vtkEpcNameList.push_back(fileName);

		return vtkEpc->getError();
	}
	return std::string();
}

//----------------------------------------------------------------------------
VtkEpcDocument* VtkEpcDocumentSet::getVtkEpcDocument(const std::string & uuid)
{
	return uuidToVtkEpc.find(uuid) != uuidToVtkEpc.end() ? uuidToVtkEpc[uuid] : nullptr;
}

//----------------------------------------------------------------------------
VtkEpcCommon::Resqml2Type VtkEpcDocumentSet::getTypeInEpcDocument(const std::string & uuid)
{

	auto epcDoc = uuidToVtkEpc.find(uuid) != uuidToVtkEpc.end() ? uuidToVtkEpc[uuid] : nullptr;
	if (epcDoc!=nullptr) {
		return epcDoc->getType(uuid);
	}
	return VtkEpcCommon::UNKNOW;
}

//----------------------------------------------------------------------------
std::vector<VtkEpcCommon> VtkEpcDocumentSet::getAllVtkEpcCommons() const
{
	std::vector<VtkEpcCommon> result;

	// concatenate all VtkEpcCommons of all epc documents (i.e repos).
	for (const auto& vtkRepo : vtkEpcList) {
		result.insert(result.end(),
			vtkRepo->getAllVtkEpcCommons().begin(), vtkRepo->getAllVtkEpcCommons().end());
	}

	return result;
}
