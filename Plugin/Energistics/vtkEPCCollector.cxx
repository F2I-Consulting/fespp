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
#include <vtkOutputWindow.h>
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
	vtkOutputWindowDisplayText("fespp build: " __DATE__ " " __TIME__ "\n");
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
			// add selector — collect resolved paths first to avoid erasing during iteration
			{
				std::vector<std::string> resolved;
				for (const auto& selector : selectorNotLoaded)
				{
					if (AddSelector(selector.c_str()))
					{
						resolved.push_back(selector);
					}
				}
				for (const auto& selector : resolved)
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
			// Modified() marks the filter dirty; ParaView triggers the actual
			// pipeline update once at the end of the batched property push
			// (UpdatePipelineInformation / RequestData). Calling Update() here
			// would force a full pipeline execution on every AddSelector,
			// turning a "set N selectors" client call into N full executions
			// (N times slower per added grid as N grows).
			Modified();
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
		// See AddSelector: Modified() is enough; ParaView re-executes once
		// at the end of the property push. Update() here would compound
		// with the AddSelector calls into N+1 full pipeline executions.
		Modified();
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
void vtkEPCCollector::SetExplicitSelection(bool value)
{
	if (this->ExplicitSelection != value)
	{
		this->ExplicitSelection = value;
		repository.setExplicitSelection(value);
		Modified();
	}
}

//----------------------------------------------------------------------------
void vtkEPCCollector::SetTreeHierarchyMode(int value)
{
	if (this->TreeHierarchyMode != value)
	{
		this->TreeHierarchyMode = value;
		// Qualify the enum explicitly: the int member by the same
		// name shadows the unqualified token in this scope.
		repository.setTreeHierarchyMode(static_cast<::TreeHierarchyMode>(value));
		// Re-traverse the in-memory fesapi repository so the
		// assembly reflects the new layout without requiring an EPC
		// re-import.
		repository.rebuildAssembly();
		// Drop pending and active selector paths — their node ids
		// belonged to the previous layout. Keeping them around
		// would cause GetFirstNodeByPath to log "Invalid parameters"
		// warnings on each RequestData; the user must re-pick what
		// they want under the new layout.
		selectorNotLoaded.clear();
		selectors.clear();
		++this->AssemblyTag;
		Modified();
	}
}

//------------------------------------------------------------------------------
int vtkEPCCollector::RequestInformation(vtkInformation* vtkNotUsed(request),
	vtkInformationVector** vtkNotUsed(inputVector), vtkInformationVector* outputVector)
{
	vtkInformation* outInfo = outputVector->GetInformationObject(0);
	outInfo->Remove(vtkStreamingDemandDrivenPipeline::TIME_STEPS());
	const std::vector<double> times = repository.getTimes();

	if (times.size() > (std::numeric_limits<int>::max)())
	{
		throw std::out_of_range("Too much times.");
	}

	if (!times.empty())
	{
		const auto minmax = std::minmax_element(begin(times), end(times));
		outInfo->Set(vtkStreamingDemandDrivenPipeline::TIME_STEPS(), &times[0], static_cast<int>(times.size()));
		static double timeRange[] = { *minmax.first, *minmax.second };
		outInfo->Set(vtkStreamingDemandDrivenPipeline::TIME_RANGE(), timeRange, 2);
	}
	return 1;
}

//----------------------------------------------------------------------------
int vtkEPCCollector::RequestData(vtkInformation* info,
	vtkInformationVector** inputVector,
	vtkInformationVector* outputVector)
{
	// Load state (load selection in wait)
	// Collect paths to resolve first to avoid erasing from container during iteration
	if (!selectorNotLoaded.empty())
	{
		vtkDataAssembly* assembly = GetAssembly();
		if (assembly)
		{
			std::vector<std::string> resolved;
			for (const auto& path : selectorNotLoaded)
			{
				int node_id = assembly->GetFirstNodeByPath(path.c_str());
				if (node_id > -1)
				{
					repository.selectNodeId(node_id);
					resolved.push_back(path);
				}
			}
			for (const auto& path : resolved)
			{
				selectorNotLoaded.erase(path);
			}
		}
	}

	auto* outInfo = outputVector->GetInformationObject(0);
	// current timeStep value
	double requestedTimeStep = outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP());

	bool dataLoaded = false;
	try
	{
		vtkSmartPointer<vtkPartitionedDataSetCollection> pdc = repository.getVtkPartitionedDatasSetCollection(requestedTimeStep, Controller->GetNumberOfProcesses(), Controller->GetLocalProcessId());
		vtkPartitionedDataSetCollection::GetData(outInfo)->DeepCopy(pdc);
		// close hdfProxies in case the system would want reuse hdf files
		repository.closeHdfProxies();
		dataLoaded = true;
	}
	catch (const std::exception& e)
	{
		vtkWarningMacro(<< e.what());
	}

	if (dataLoaded)
	{
		if (GetOutput())
		{
			vtkSmartPointer<vtkPartitionedDataSetCollection> pdc = GetOutput();
			ExtractTag = pdc->GetNumberOfPartitionedDataSets() > 0 ? 0 : ExtractTag++;
		}
		AssemblyTag++;
	}
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
	return repository.GetAssembly();
}

//----------------------------------------------------------------------------
vtkDataAssembly* vtkEPCCollector::GetLiveAssembly()
{
	return repository.GetAssembly();
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
			this->ExtractWithCopy(readerProxy, DataSetList->LookupValue(name));
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
// For extract without copy: each block with selection status
void vtkEPCCollector::SetDataSetListForCopy(const char* name, int status)
{
	if (status == 1)
	{
		vtkSMSourceProxy* readerProxy = GetThisProxy();

		if (readerProxy)
		{
			this->ExtractWithoutCopy(readerProxy, DataSetListForCopy->LookupValue(name));
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
// "WithCopy" semantics: a fully INDEPENDENT snapshot of the partition data.
// Implemented as a standalone PVTrivialProducer holding a one-shot DeepCopy.
// The producer is registered in the "sources" group, detached from this
// collector's pipeline — once created it does NOT track upstream changes.
// Use this when you want a frozen view of the data (export, comparison).
void vtkEPCCollector::ExtractWithCopy(vtkSMSourceProxy* /*readerProxy*/, int index)
{
	vtkSMSessionProxyManager* sessionProxyManager = vtkSMProxyManager::GetProxyManager()->GetActiveSessionProxyManager();

	vtkSmartPointer<vtkPartitionedDataSetCollection> pdc = repository.getVtkPartitionedDatasSetCollection();
	if (!pdc)
	{
		vtkWarningMacro(<< "No data available to copy.");
		return;
	}
	vtkPartitionedDataSet* partitionedDataSet = pdc->GetPartitionedDataSet(index);
	if (!partitionedDataSet || partitionedDataSet->GetNumberOfPartitions() == 0)
	{
		vtkWarningMacro(<< "No partition available at index " << index);
		return;
	}
	vtkDataObject* original = partitionedDataSet->GetPartitionAsDataObject(0);
	if (!original)
	{
		vtkWarningMacro(<< "Partition[0] is null at index " << index);
		return;
	}

	// One-shot DeepCopy: this snapshot lives independently of the source.
	vtkSmartPointer<vtkDataObject> snapshot;
	snapshot.TakeReference(original->NewInstance());
	snapshot->DeepCopy(original);

	vtkSMSourceProxy* producerProxy = vtkSMSourceProxy::SafeDownCast(sessionProxyManager->NewProxy("sources", "PVTrivialProducer"));
	if (!producerProxy)
	{
		vtkWarningMacro(<< "Failed to create PVTrivialProducer proxy.");
		return;
	}
	producerProxy->UpdateVTKObjects();

	vtkPVTrivialProducer* realProducer = vtkPVTrivialProducer::SafeDownCast(producerProxy->GetClientSideObject());
	if (realProducer)
	{
		realProducer->SetOutput(snapshot);
	}

	sessionProxyManager->RegisterProxy("sources", DataSetList->GetValue(index).c_str(), producerProxy);
	producerProxy->Delete();
}

//----------------------------------------------------------------------------
// "WithoutCopy" semantics: a sub-source FILTER chained on this collector
// (registered in the "sources" group — RegisterPipelineProxy puts
// pipeline-typed proxies there). The filter does a ShallowCopy in
// RequestData (vtkEnergisticsExtractor) — no real data duplication, just
// shared array pointers. Because the filter stays in the pipeline, upstream
// updates (selector changes, realization swap, property addDataArray)
// propagate naturally through the standard VTK Modified/RequestData flow.
// Use this when you want a live, lightweight view of a specific block.
void vtkEPCCollector::ExtractWithoutCopy(vtkSMSourceProxy* readerProxy, int index)
{
	vtkSMSessionProxyManager* sessionProxyManager = vtkSMProxyManager::GetProxyManager()->GetActiveSessionProxyManager();

	vtkSMSourceProxy* extract = vtkSMSourceProxy::SafeDownCast(sessionProxyManager->NewProxy("filters", "EnergisticsExtractor"));
	if (!extract)
	{
		vtkWarningMacro(<< "Failed to create EnergisticsExtractor proxy.");
		return;
	}

	vtkSMInputProperty* inputProperty = vtkSMInputProperty::SafeDownCast(extract->GetProperty("Input"));
	if (!inputProperty)
	{
		vtkWarningMacro(<< "EnergisticsExtractor proxy has no Input property.");
		extract->Delete();
		return;
	}
	inputProperty->SetInputConnection(0, readerProxy, 0);

	vtkDataAssembly* assembly = GetAssembly();
	if (assembly)
	{
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
					) &&
				indices[0] == static_cast<unsigned int>(index)
				)
			{
				vtkSMPropertyHelper(extract, "ExtractPath").Set(assembly->GetNodePath(node).c_str());
			}
		}
	}

	extract->UpdateVTKObjects();
	extract->UpdatePipelineInformation();

	vtkNew<vtkSMParaViewPipelineController> controller;
	controller->InitializeProxy(extract);
	controller->RegisterPipelineProxy(extract, DataSetListForCopy->GetValue(index).c_str());
	extract->Delete();
}

//----------------------------------------------------------------------------
// Programmatic per-representation extractor used by fespp_on_trame.
// Aligns with the "WithoutCopy" semantics: creates an EnergisticsExtractor
// FILTER chained on this collector (registered in the "sources" group —
// RegisterPipelineProxy puts pipeline-typed proxies there). The filter
// does a ShallowCopy in RequestData, so upstream changes (selector add,
// realization swap, property addDataArray) propagate naturally — no
// explicit Modified() bump required from the Python side.
//
// Idempotent: repeated calls for the same rep_path return the existing
// registration name (cached in repProducerNames).
//
// Property command: triggers the per-rep filter creation and stores the
// registration name in lastExtractedProducerName for the info-only readback
// (GetExtractedRepProducerName). The Python side reads that name and
// resolves it via the proxy manager (sources group).
void vtkEPCCollector::SetExtractRepPath(const char* rep_path)
{
	if (!lastExtractedProducerName)
		lastExtractedProducerName = vtkSmartPointer<vtkStringArray>::New();
	lastExtractedProducerName->SetNumberOfValues(0);
	if (rep_path == nullptr || rep_path[0] == '\0')
		return;
	const std::string key(rep_path);
	vtkSMSessionProxyManager* spm = vtkSMProxyManager::GetProxyManager()->GetActiveSessionProxyManager();
	auto cached = repProducerNames.find(key);
	if (cached != repProducerNames.end())
	{
		// Reuse only if the registered proxy still exists; the Python side
		// may have called Delete() on a previous release() and we'd return
		// a stale name otherwise. Search the "sources" group (where
		// RegisterPipelineProxy actually puts EnergisticsExtractor — the
		// older comment said "filters" but RegisterPipelineProxy registers
		// pipeline-typed proxies in "sources"; we keep the "filters" lookup
		// as a defensive fallback for old saved sessions).
		if (spm && (spm->GetProxy("sources", cached->second.c_str()) != nullptr
		            || spm->GetProxy("filters", cached->second.c_str()) != nullptr))
		{
			lastExtractedProducerName->InsertNextValue(cached->second);
			return;
		}
		repProducerNames.erase(cached);
	}

	vtkDataAssembly* assembly = repository.GetAssembly();
	if (!assembly)
		return;
	int node = assembly->GetFirstNodeByPath(rep_path);
	if (node < 0)
		return;
	auto indices = assembly->GetDataSetIndices(node);
	if (indices.empty())
		return;

	vtkSMSourceProxy* readerProxy = GetThisProxy();
	if (!readerProxy)
		return;

	vtkSMSourceProxy* extract = vtkSMSourceProxy::SafeDownCast(spm->NewProxy("filters", "EnergisticsExtractor"));
	if (!extract)
	{
		vtkWarningMacro(<< "Failed to create EnergisticsExtractor for " << rep_path);
		return;
	}

	vtkSMInputProperty* inputProperty = vtkSMInputProperty::SafeDownCast(extract->GetProperty("Input"));
	if (!inputProperty)
	{
		vtkWarningMacro(<< "EnergisticsExtractor has no Input property.");
		extract->Delete();
		return;
	}
	inputProperty->SetInputConnection(0, readerProxy, 0);

	vtkSMPropertyHelper(extract, "ExtractPath").Set(rep_path);
	extract->UpdateVTKObjects();
	extract->UpdatePipelineInformation();

	// Build a registration name unique per rep_path. The leading '/' and any
	// '/' in the path are not valid in proxy registration names.
	std::string regName = "rep";
	for (const char* p = rep_path; *p; ++p)
		regName += (*p == '/' ? '_' : *p);

	vtkNew<vtkSMParaViewPipelineController> controller;
	controller->InitializeProxy(extract);
	controller->RegisterPipelineProxy(extract, regName.c_str());
	extract->Delete();

	repProducerNames[key] = regName;
	lastExtractedProducerName->InsertNextValue(regName);
}

// Info-only readback: returns a 1-element vtkStringArray with the
// registration name set by the most recent SetExtractRepPath call. Empty
// array if SetExtractRepPath failed or wasn't called.
vtkStringArray* vtkEPCCollector::GetExtractedRepProducerName()
{
	if (!lastExtractedProducerName)
		lastExtractedProducerName = vtkSmartPointer<vtkStringArray>::New();
	return lastExtractedProducerName;
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
	vtkDataAssembly* assembly = repository.GetAssembly();
	if (!assembly)
	{
		return result;
	}

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