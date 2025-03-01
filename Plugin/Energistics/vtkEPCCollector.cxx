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
#include "vtkEPCCollector.h"

#include <exception>
#include <iterator>
#include <algorithm>
#include <limits>
#include <sstream>

#include "vtkEnergisticsExtractor.h"

#include <vtkIndent.h>
#include <vtkInformation.h>
#include <vtkInformationVector.h>
#include <vtkPartitionedDataSetCollection.h>
#include <vtkDataAssembly.h>
#include <vtkMultiProcessController.h>
#include <vtkStreamingDemandDrivenPipeline.h>
#include <vtkPVDataInformation.h>
#include <vtkPartitionedDataSet.h>
#include "vtkSMInputProperty.h"
#include "vtkSMProxyManager.h"
#include "vtkSMSessionProxyManager.h"
#include "vtkSMSourceProxy.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMProxyIterator.h"
#include <vtkSMParaViewPipelineController.h>
#include <vtkDataSet.h>
#include <vtkSMPropertyIterator.h>
#include <vtkSmartPointer.h>
#include <vtkPVTrivialProducer.h>
#include <vtkSMStringVectorProperty.h>
#include <vtkNew.h>
#include <vtkSMSession.h>

vtkStandardNewMacro(vtkEPCCollector);
vtkCxxSetObjectMacro(vtkEPCCollector, Controller, vtkMultiProcessController);

//----------------------------------------------------------------------------
vtkEPCCollector::vtkEPCCollector() : Files(vtkStringArray::New()),
Controller(nullptr),
AssemblyTag(0),
ExtractTag(0),
DataSetList(vtkStringArray::New()),
DataSetListSelection({}),
DataSetListForCopy(vtkStringArray::New()),
DataSetListSelectionForCopy({}),
MarkerOrientation(true),
MarkerSize(10),
colorApplyLoading(false)
{
	SetNumberOfInputPorts(0);
	SetController(vtkMultiProcessController::GetGlobalController());
}

vtkEPCCollector::~vtkEPCCollector()
{
	SetController(nullptr);
}

//----------------------------------------------------------------------------
void vtkEPCCollector::AddFileNameToFiles(const char* fname)
{
	if (fname != nullptr)
	{
		Files->InsertNextValue(fname);
	}
}

//----------------------------------------------------------------------------
void vtkEPCCollector::ClearFileName()
{
}

//----------------------------------------------------------------------------
const char* vtkEPCCollector::GetFileName(int index) const
{
	if (Files->GetNumberOfValues() > index)
	{
		return Files->GetValue(index).c_str();
	}
	return nullptr;
}

//----------------------------------------------------------------------------
size_t vtkEPCCollector::GetNumberOfFileNames() const
{
	return Files->GetNumberOfValues();
}

//------------------------------------------------------------------------------
vtkStringArray* vtkEPCCollector::GetAllFiles() // call only by GUI
{
	for (auto index = 0; index < Files->GetNumberOfValues(); index++)
	{
		auto file_property = Files->GetValue(index);
		auto search = FileNamesLoaded.find(file_property);
		if (search == FileNamesLoaded.end())
		{
			std::string msg = repository.addFile(file_property.c_str());

			FileNamesLoaded.insert(file_property);
			// add selector
			for (auto selector : selectorNotLoaded)
			{
				if (AddSelector(selector.c_str()))
				{
					selectorNotLoaded.erase(selector);
				}
			}

			if (Controller->GetLocalProcessId() == 0 && !msg.empty())
			{
				vtkWarningMacro(<< msg);
			}
			Modified();
			Update();
		}
	}
	return Files;
}

//------------------------------------------------------------------------------
void vtkEPCCollector::AddFiles(const std::string& file)
{
	if (file != "0" && file != "1") //  => ArraySelectionDomain <= state of file
	{
		bool exist = false;
		for (auto index = 0; index < Files->GetNumberOfValues(); ++index)
		{
			auto file_property = Files->GetValue(index);
			if (file_property == file)
			{
				exist = true;
			}
		}
		if (!exist)
		{
			Files->InsertNextValue(file);
		}
	}
	if (Controller->GetLocalProcessId() > 0) // pvserver without GUI
	{
		GetAllFiles();
	}
	Modified();
	Update();
}

//------------------------------------------------------------------------------
void vtkEPCCollector::SetFiles(const std::string& file)
{
	AddFiles(file);
}

//----------------------------------------------------------------------------
bool vtkEPCCollector::AddSelector(const char* path)
{
	if (path != nullptr && selectors.insert(path).second)
	{
		int node_id = repository.GetAssembly()->GetFirstNodeByPath(path);

		if (node_id == -1)
		{
			selectorNotLoaded.insert(std::string(path));
		}
		else
		{
			repository.selectNodeId(node_id);
			ExtractTag = 0;
			DataSetList = vtkStringArray::New();
			/*
				   if (repository.GetAssembly()->HasAttribute(node_id, "traj"))
				   {
					   int node_id_parent = repository.GetAssembly()->GetAttributeOrDefault(node_id, "traj", 0);
				   }
			*/
			Modified();
			Update();
			return true;
		}
	}

	return false;

}

//----------------------------------------------------------------------------
void vtkEPCCollector::ClearSelectors()
{
	repository.clearSelection();
	ExtractTag = 1;
	DataSetList = vtkStringArray::New();
	if (!selectors.empty())
	{
		selectors.clear();
		Modified();
		Update();
	}
}

//----------------------------------------------------------------------------
int vtkEPCCollector::GetNumberOfSelectors() const
{
	if (selectors.size() > (std::numeric_limits<int>::max)())
	{
		throw std::out_of_range("Too much selectors.");
	}
	return static_cast<int>(selectors.size());
}

//----------------------------------------------------------------------------
const char* vtkEPCCollector::GetSelector(int index) const
{
	if (index >= 0 && index < GetNumberOfSelectors())
	{
		auto iter = std::next(selectors.begin(), index);
		return iter->c_str();
	}
	return nullptr;
}

//----------------------------------------------------------------------------
void vtkEPCCollector::setMarkerOrientation(bool orientation)
{
	repository.setMarkerOrientation(orientation);
	Modified();
	UpdateInformation();
}

//----------------------------------------------------------------------------
void vtkEPCCollector::setMarkerSize(int size)
{
	repository.setMarkerSize(size);
	Modified();
}

//----------------------------------------------------------------------------
int vtkEPCCollector::RequestData(vtkInformation* info,
	vtkInformationVector** inputVector,
	vtkInformationVector* outputVector)
{
	// Load state (load selection in wait)
	if (selectorNotLoaded.size() > 0)
	{
		for (auto path : selectorNotLoaded)
		{
			int node_id = GetAssembly()->GetFirstNodeByPath(path.c_str());
			if (node_id > -1)
			{
				repository.selectNodeId(node_id);
				selectorNotLoaded.erase(path);
			}
		}
	}

	auto* outInfo = outputVector->GetInformationObject(0);
	outInfo->Remove(vtkStreamingDemandDrivenPipeline::TIME_STEPS());
	const std::vector<double> times = repository.getTimes();

	if (times.size() > (std::numeric_limits<int>::max)())
	{
		throw std::out_of_range("Too much times.");
	}
	double requestedTimeStep = 0;
	if (!times.empty())
	{
		const auto minmax = std::minmax_element(begin(times), end(times));
		outInfo->Set(vtkStreamingDemandDrivenPipeline::TIME_STEPS(), &times[0], static_cast<int>(times.size()));
		static double timeRange[] = { *minmax.first, *minmax.second };
		outInfo->Set(vtkStreamingDemandDrivenPipeline::TIME_RANGE(), timeRange, 2);

		// current timeStep value
		requestedTimeStep = outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP());
	}

	try
	{
		vtkSmartPointer < vtkPartitionedDataSetCollection> pdc = repository.getVtkPartitionedDatasSetCollection(requestedTimeStep, Controller->GetNumberOfProcesses(), Controller->GetLocalProcessId());
		vtkPartitionedDataSetCollection::GetData(outInfo)->DeepCopy(pdc);
		// close hdfProxies in case the system would want reuse hdf files
		repository.closeHdfProxies();
	}
	catch (const std::exception& e)
	{
		vtkWarningMacro(<< e.what());
	}
	if (GetOutput())
	{
		vtkSmartPointer < vtkPartitionedDataSetCollection> pdc = GetOutput();
		ExtractTag = pdc->GetNumberOfPartitionedDataSets() > 0 ? 0 : ExtractTag++;
	}
	AssemblyTag++;
	Modified();
	return 1;
}

//----------------------------------------------------------------------------
void vtkEPCCollector::PrintSelf(ostream& os, vtkIndent indent)
{
	Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
vtkDataAssembly* vtkEPCCollector::GetAssembly()
{
	vtkPVDataInformation* dinfo = vtkPVDataInformation::New();
	dinfo->CopyFromObject(this->GetOutputDataObject(0));

	return dinfo->GetDataAssembly();
}

//------------------------------------------------------------------------------
// For extract by reference: clear selection
void vtkEPCCollector::ClearDataSetList()
{
	DataSetListSelection.clear();
}

//------------------------------------------------------------------------------
// For extract by reference: each block with selection status
void vtkEPCCollector::SetDataSetList(const char* name, int status)
{
	if (status == 1)
	{
		vtkSMSourceProxy* readerProxy = GetThisProxy();

		if (readerProxy)
		{
			this->Extract(readerProxy, DataSetList->LookupValue(name));
		}
	}
}

//------------------------------------------------------------------------------
// Call only by GUI: for extract by reference selection of blocks
vtkStringArray* vtkEPCCollector::GetAllDataSet()
{
	return GetHierarchyBlocks("REFERENCE");
}

//------------------------------------------------------------------------------
// For extract with copy: clear selection
// 
// ! important: (1) DataSetListForCopy is last declare in properties xml 
//
void vtkEPCCollector::ClearDataSetListForCopy()
{
	DataSetListSelectionForCopy.clear();

	this->ClearExtractAndCopy(); // (1) 
}

//------------------------------------------------------------------------------
// For extract with copy: each block with selection status
void vtkEPCCollector::SetDataSetListForCopy(const char* name, int status)
{
	if (status == 1)
	{
		vtkSMSourceProxy* readerProxy = GetThisProxy();

		if (readerProxy)
		{
			this->Copy(readerProxy, DataSetListForCopy->LookupValue(name));
		}
	}
}

//------------------------------------------------------------------------------
// Call only by GUI: for extract with copy selection of blocks
vtkStringArray* vtkEPCCollector::GetAllDataSetForCopy() 
{
	return GetHierarchyBlocks("COPY");
}

//----------------------------------------------------------------------------
// Create a new EnergisticsExtractor sub-pipeline 
void vtkEPCCollector::Extract(vtkSMSourceProxy* readerProxy, int index)
{
	vtkSMSessionProxyManager* sessionProxyManager = vtkSMProxyManager::GetProxyManager()->GetActiveSessionProxyManager();

	vtkSmartPointer<vtkStringArray> list = nullptr;
	std::map<std::string, bool> map;

	list = DataSetList;

	vtkSMSourceProxy* extract = vtkSMSourceProxy::SafeDownCast(sessionProxyManager->NewProxy("filters", "EnergisticsExtractor"));;

	// set the input
	vtkSMInputProperty* inputProperty = vtkSMInputProperty::SafeDownCast(extract->GetProperty("Input"));
	inputProperty->SetInputConnection(0, readerProxy, 0);

	for (const auto& node : GetAssembly()->GetChildNodes(0))
	{
		std::vector<unsigned int> indices = GetAssembly()->GetDataSetIndices(node);
		if (!indices.empty() &&
			(strcmp(GetAssembly()->GetAttributeOrDefault(node, "type", GetAssembly()->GetNodeName(node)), std::to_string(static_cast<int>(TreeViewNodeType::Representation)).c_str()) == 0 ||
				strcmp(GetAssembly()->GetAttributeOrDefault(node, "type", GetAssembly()->GetNodeName(node)), std::to_string(static_cast<int>(TreeViewNodeType::SubRepresentation)).c_str()) == 0 ||
				strcmp(GetAssembly()->GetAttributeOrDefault(node, "type", GetAssembly()->GetNodeName(node)), std::to_string(static_cast<int>(TreeViewNodeType::WellboreTrajectory)).c_str()) == 0 ||
				strcmp(GetAssembly()->GetAttributeOrDefault(node, "type", GetAssembly()->GetNodeName(node)), std::to_string(static_cast<int>(TreeViewNodeType::WellboreChannel)).c_str()) == 0 ||
				strcmp(GetAssembly()->GetAttributeOrDefault(node, "type", GetAssembly()->GetNodeName(node)), std::to_string(static_cast<int>(TreeViewNodeType::WellboreMarker)).c_str()) == 0 ||
				strcmp(GetAssembly()->GetAttributeOrDefault(node, "type", GetAssembly()->GetNodeName(node)), std::to_string(static_cast<int>(TreeViewNodeType::Perforation)).c_str()) == 0
				) &&
			GetAssembly()->GetDataSetIndices(node)[0] == index
			)
		{
			vtkSMPropertyHelper(extract, "ExtractPath").Set(GetAssembly()->GetNodePath(node).c_str());

		}
	}



	extract->UpdateVTKObjects();
	extract->UpdatePipelineInformation();

	vtkNew<vtkSMParaViewPipelineController> controller;
	controller->InitializeProxy(extract);
	controller->RegisterPipelineProxy(extract, list->GetValue(index));
}

//----------------------------------------------------------------------------
// Create a new vtkDataSet pipeline
void vtkEPCCollector::Copy(vtkSMSourceProxy* readerProxy, int index)
{
	vtkSMSessionProxyManager* sessionProxyManager = vtkSMProxyManager::GetProxyManager()->GetActiveSessionProxyManager();

	vtkSmartPointer<vtkStringArray> list = nullptr;
	std::map<std::string, bool> map;

	list = DataSetListForCopy;

	vtkSMSourceProxy* producerCopyProxy = vtkSMSourceProxy::SafeDownCast(sessionProxyManager->NewProxy("sources", "PVTrivialProducer"));
	producerCopyProxy->UpdateVTKObjects();

	auto* clientSideObject = producerCopyProxy->GetClientSideObject();
	vtkPVTrivialProducer* realProducer = vtkPVTrivialProducer::SafeDownCast(clientSideObject);
	if (realProducer)
	{
		vtkSmartPointer < vtkPartitionedDataSetCollection> pdc = repository.getVtkPartitionedDatasSetCollection();
		vtkPartitionedDataSet* partitionedDataSet = pdc->GetPartitionedDataSet(index);
		realProducer->SetOutput(partitionedDataSet->GetPartitionAsDataObject(0));
	}

	sessionProxyManager->RegisterProxy("sources", list->GetValue(index), producerCopyProxy);
}

//----------------------------------------------------------------------------
// Clear property Extract with and without copy
void vtkEPCCollector::ClearExtractAndCopy()
{
	vtkSMSourceProxy* readerProxy = GetThisProxy();

	if (readerProxy)
	{

		// reset extract selection
		vtkSmartPointer<vtkSMPropertyIterator> iterProperty;
		iterProperty.TakeReference(readerProxy->NewPropertyIterator());
		for (iterProperty->Begin(); !iterProperty->IsAtEnd(); iterProperty->Next())
		{
			// selectors are VectorProperty
			auto property = vtkSMStringVectorProperty::SafeDownCast(iterProperty->GetProperty());
			if (property == nullptr)
			{
				continue;
			}
			else
			{
				if (strcmp(property->GetXMLName(), "DataSetList") == 0 ||
					strcmp(property->GetXMLName(), "DataSetListForCopy") == 0)
				{
					unsigned int nbElements = property->GetNumberOfElements();
					std::vector<const char *> values(nbElements);
					std::vector<int> states(nbElements);

					for (unsigned int i = 0; i < nbElements; i++)
					{
						values[i] = property->GetElement(i);
					}

					for (unsigned int i = 0; i < nbElements; i = i+2)
					{
						if (strcmp(values[i + 1], "1") == 0)
						{
							property->SetElement(i + 1, "0");
						}
					}

					property->Modified();
					property->GetImmediateUpdate();
				}
			}
		}
	}
}

//------------------------------------------------------------------------------
// Call only by GUI: for extract by reference selection of blocks
vtkStringArray* vtkEPCCollector::GetHierarchyBlocks(std::string type)
{
	vtkStringArray* result = (type=="COPY")? DataSetListForCopy:DataSetList;
	result->Initialize();
	vtkPVDataInformation* dinfo = vtkPVDataInformation::New();
	dinfo->CopyFromObject(this->GetOutputDataObject(0));
	vtkSmartPointer<vtkDataAssembly> assembly = dinfo->GetDataAssembly();

	for (const auto& node : assembly->GetChildNodes(0))
	{
		std::vector<unsigned int> indices = assembly->GetDataSetIndices(node);
		if (!indices.empty() &&
			(strcmp(assembly->GetAttributeOrDefault(node, "type", assembly->GetNodeName(node)), std::to_string(static_cast<int>(TreeViewNodeType::Representation)).c_str()) == 0 ||
				strcmp(assembly->GetAttributeOrDefault(node, "type", assembly->GetNodeName(node)), std::to_string(static_cast<int>(TreeViewNodeType::SubRepresentation)).c_str()) == 0 ||
				strcmp(assembly->GetAttributeOrDefault(node, "type", assembly->GetNodeName(node)), std::to_string(static_cast<int>(TreeViewNodeType::WellboreTrajectory)).c_str()) == 0 ||
				strcmp(assembly->GetAttributeOrDefault(node, "type", assembly->GetNodeName(node)), std::to_string(static_cast<int>(TreeViewNodeType::WellboreChannel)).c_str()) == 0 ||
				strcmp(assembly->GetAttributeOrDefault(node, "type", assembly->GetNodeName(node)), std::to_string(static_cast<int>(TreeViewNodeType::WellboreMarker)).c_str()) == 0 ||
				strcmp(assembly->GetAttributeOrDefault(node, "type", assembly->GetNodeName(node)), std::to_string(static_cast<int>(TreeViewNodeType::Perforation)).c_str()) == 0
			))
		{
				result->InsertNextValue(assembly->GetAttributeOrDefault(node, "label", assembly->GetNodeName(node)));

		}
	}

	return result;
}

//------------------------------------------------------------------------------
vtkSMSourceProxy* vtkEPCCollector::GetThisProxy()
{
	vtkSMProxyManager* proxyManager = vtkSMProxyManager::GetProxyManager();
	vtkSMSession* session = proxyManager->GetActiveSession();
	
	vtkSMSourceProxy* collectorProxy = nullptr;
	vtkNew<vtkSMProxyIterator> iterProxy;
	iterProxy->SetSession(session);
	for (iterProxy->Begin("sources"); !iterProxy->IsAtEnd(); iterProxy->Next())
	{
		vtkSMSourceProxy* sourceProxy = vtkSMSourceProxy::SafeDownCast(iterProxy->GetProxy());
		if (sourceProxy && sourceProxy->GetClientSideObject() == this)
		{
			collectorProxy = sourceProxy;
			break;
		}
	}

	if (collectorProxy)
	{
		return collectorProxy;
	}
	return nullptr;
}

// --------------------------------------------------------------------------
void vtkEPCCollector::ApplyColors()
{
	repository.addResqmlColor();
}