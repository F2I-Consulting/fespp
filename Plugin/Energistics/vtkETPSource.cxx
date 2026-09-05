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
#include "vtkETPSource.h"

#include <algorithm>
#include <exception>
#include <iterator>
#include <limits>
#include <thread>

#include <vtkDataAssembly.h>
#include <vtkIndent.h>
#include <vtkInformation.h>
#include <vtkInformationVector.h>
#include <vtkObjectFactory.h>
#include <vtkPartitionedDataSetCollection.h>
#include <vtkStdString.h>
#include <vtkStreamingDemandDrivenPipeline.h>

#include <VTK/vtkCustomProgressBar.h>

vtkStandardNewMacro(vtkETPSource);

//----------------------------------------------------------------------------
vtkETPSource::vtkETPSource() :
	ETPUrl("wss://osdu-ship.msft-osdu-test.org:443/oetp/reservoir-ddms/"),
	ProxyUrl(""),
	OSDUDataPartition("osdu"),
	ETPTokenType("Bearer"),
	ProxyTokenType("Basic"),
	ETPToken(""),
	ProxyToken(""),
	AllDataspaces(vtkStringArray::New()),
	_newSelection(false),
	AssemblyTag(0),
	ConnectionTag(1),
	DisconnectionTag(0),
	MarkerOrientation(true),
	MarkerSize(10),
	colorApplyLoading(false)
{
	SetNumberOfInputPorts(0);
	SetNumberOfOutputPorts(1);
}

//----------------------------------------------------------------------------
vtkETPSource::~vtkETPSource() = default;

//------------------------------------------------------------------------------
int vtkETPSource::RequestInformation(vtkInformation* info,
	vtkInformationVector** vtkNotUsed(inputVector), vtkInformationVector* outputVector)
{
	vtkInformation* outInfo = outputVector->GetInformationObject(0);
	outInfo->Set(CAN_HANDLE_PIECE_REQUEST(), 1);
	return 1;
}

//----------------------------------------------------------------------------
void vtkETPSource::setETPUrlConnection(char* etp_url)
{
	ETPUrl = std::string(etp_url);
}

//----------------------------------------------------------------------------
void vtkETPSource::setProxyUrlConnection(char* proxy_url)
{
	ProxyUrl = std::string(proxy_url);
}

//----------------------------------------------------------------------------
void vtkETPSource::setOSDUDataPartition(char* data_partition)
{
	OSDUDataPartition = std::string(data_partition);
}

//----------------------------------------------------------------------------
void vtkETPSource::setETPTokenType(int auth_type)
{
	if (auth_type == 0)
	{
		ETPTokenType = "Bearer";
	}
	else if (auth_type == 1)
	{
		ETPTokenType = "Basic";
	}
	else
	{
		ETPTokenType = "unknown";
	}
}

//----------------------------------------------------------------------------
void vtkETPSource::setProxyTokenType(int auth_type)
{
	if (auth_type == 0)
	{
		ProxyTokenType = "Bearer";
	}
	else if (auth_type == 1)
	{
		ProxyTokenType = "Basic";
	}
	else
	{
		ProxyTokenType = "unknown";
	}
}

//----------------------------------------------------------------------------
void vtkETPSource::setETPToken(char* auth_connection)
{
	ETPToken = std::string(auth_connection);
}

//----------------------------------------------------------------------------
void vtkETPSource::setProxyToken(char* auth_connection)
{
	ProxyToken = std::string(auth_connection);
}

//----------------------------------------------------------------------------
// The vtkCustomProgressBar blocks that used to live here (and in
// SetDataspaces / RequestData) spawned a DETACHED thread capturing the
// stack-local vtkNew by reference: whenever the function body was fast
// (empty repository, no-op dataspace), the thread outlived the frame —
// use-after-free, hard process crash. Useless in a headless server; removed.
void vtkETPSource::confirmConnectionClicked()
{
	try
	{
		// Clear AllDataspaces before filling to avoid accumulation
		AllDataspaces->SetNumberOfValues(0);

		const auto dataspaces = repository.connect(ETPUrl, OSDUDataPartition, ETPTokenType + " " + ETPToken, ProxyUrl, ProxyTokenType + " " + ProxyToken);

		for (const std::string dataspace : dataspaces)
		{
			AllDataspaces->InsertNextValue(vtkStdString(dataspace));
		}

		AllDataspaces->InsertNextValue(vtkStdString("eml:///"));

		ConnectionTag = 0;
		DisconnectionTag = 1;
		Modified();
		Update();
	}
	catch (const std::exception& e)
	{
		vtkErrorMacro(<< e.what());
	}
}

//----------------------------------------------------------------------------
void vtkETPSource::disconnectionClicked()
{
	repository.disconnect();

	// Clear AllDataspaces when disconnecting
	AllDataspaces->SetNumberOfValues(0);

	Modified();
	Modified();
	Update();
	UpdateDataObject();
	UpdateInformation();
	UpdateWholeExtent();
}

//------------------------------------------------------------------------------
void vtkETPSource::SetDataspaces(const char* dataspaces)
{
	if (ConnectionTag == 0)
	{
		if (std::string(dataspaces) != "")
		{
			std::string msg = repository.addDataspace(dataspaces);
			if (!msg.empty())
			{
				vtkWarningMacro(<< msg);
			}
		}
		AssemblyTag++;
	}
}

//----------------------------------------------------------------------------
int vtkETPSource::GetNumberOfDataspaces()
{
	if (AllDataspaces == nullptr)
	{
		return 0;
	}
	return static_cast<int>(AllDataspaces->GetNumberOfValues());
}

//----------------------------------------------------------------------------
const char* vtkETPSource::GetDataspace(int index)
{
	if (AllDataspaces == nullptr)
	{
		return nullptr;
	}

	if (index < 0 || index >= AllDataspaces->GetNumberOfValues())
	{
		vtkWarningMacro(<< "GetDataspace: index " << index << " is out of range [0, "
		                << AllDataspaces->GetNumberOfValues() << ")");
		return nullptr;
	}

	return AllDataspaces->GetValue(index).c_str();
}

//----------------------------------------------------------------------------
bool vtkETPSource::AddSelector(const char* selector)
{
	if (selector != nullptr && selectors.insert(selector).second)
	{
		int node_id = GetAssembly()->GetFirstNodeByPath(selector);
		repository.selectNodeId(node_id);
		_newSelection = true;
		// See vtkEPCCollector::AddSelector: ParaView pushes selectors as a
		// repeatable property; calling Update / UpdateInformation per
		// AddSelector turns "set N selectors" into N full pipeline executions.
		// Modified() suffices — ParaView re-executes once at the end.
		Modified();
		return true;
	}
	return false;
}

//----------------------------------------------------------------------------
void vtkETPSource::ClearSelectors()
{
	repository.clearSelection();
	if (!selectors.empty())
	{
		selectors.clear();
		Modified();
	}
}

//----------------------------------------------------------------------------
int vtkETPSource::GetNumberOfSelectors() const
{
	if (selectors.size() > (std::numeric_limits<int>::max)())
	{
		throw std::out_of_range("Too much selectors.");
	}

	return static_cast<int>(selectors.size());
}

//----------------------------------------------------------------------------
void vtkETPSource::SetSelector(const char* selector)
{
	ClearSelectors();
	AddSelector(selector);

	Modified();
	Update();
	UpdateDataObject();
	UpdateInformation();
	UpdateWholeExtent();
}

//----------------------------------------------------------------------------
const char* vtkETPSource::GetSelector(int index) const
{
	if (index >= 0 && index < GetNumberOfSelectors())
	{
		auto iter = std::next(selectors.begin(), index);
		return iter->c_str();
	}
	return nullptr;
}

//----------------------------------------------------------------------------
void vtkETPSource::setMarkerOrientation(bool orientation)
{
	repository.setMarkerOrientation(orientation);
	Modified();
	Update();
	UpdateDataObject();
	UpdateInformation();
	UpdateWholeExtent();
}

//----------------------------------------------------------------------------
void vtkETPSource::setMarkerSize(int size)
{
	repository.setMarkerSize(size);
	Modified();
	Update();
	UpdateDataObject();
	UpdateInformation();
	UpdateWholeExtent();
}

//----------------------------------------------------------------------------
int vtkETPSource::RequestData(vtkInformation*,
	vtkInformationVector**,
	vtkInformationVector* outputVector)
{
	// No `_newSelection` gate: mirror vtkEPCCollector::RequestData, which
	// deep-copies the repository PDC unconditionally. The pipeline OUTPUT is
	// the only surface a non-wrapped-Python client can read the assembly
	// from (GetOutput()->GetDataAssembly() on the wrapped base class), so it
	// must carry the assembly right after a dataspace load, selection or not.
	if (ConnectionTag == 0)
	{
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
			vtkPartitionedDataSetCollection::GetData(outInfo)->DeepCopy(repository.getVtkPartitionedDatasSetCollection(requestedTimeStep));

			_newSelection = false;
			
			/*
			if (vtkPartitionedDataSetCollection::GetData(outInfo)->GetNumberOfPartitionedDataSets() > 0 && !colorApplyLoading) {
				colorApplyLoading = true;
				repository.addResqmlColor();
				colorApplyLoading = false;
			}
			*/
		}
		catch (const std::exception& e)
		{
			vtkWarningMacro(<< e.what());
		}

		Modified();
	}
	return 1;
}

//----------------------------------------------------------------------------
void vtkETPSource::PrintSelf(ostream& os, vtkIndent indent)
{
	Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
vtkDataAssembly* vtkETPSource::GetAssembly()
{
	try
	{
		return repository.GetAssembly();
	}
	catch (const std::exception& e)
	{
		vtkErrorMacro(<< e.what());
	}
	return nullptr;
}

//----------------------------------------------------------------------------
// Query channel — the mirror of vtkEPCCollector's, kept in step field for
// field (see the rationale there). The fetch it reads from must be over: the
// ETP session runs on a detached thread and shares the repository's gsoap
// context with serializeIntoString(); the Python side only asks after the
// dataspace load has returned.
void vtkETPSource::SetObjectXmlKey(const char* key)
{
	if (!ObjectXmlReply)
		ObjectXmlReply = vtkSmartPointer<vtkStringArray>::New();
	ObjectXmlReply->SetNumberOfValues(0);
	if (key == nullptr || key[0] == '\0')
		return;
	std::string w_arg(key);
	const auto w_bar = w_arg.find('|');
	if (w_bar != std::string::npos)
		w_arg.erase(0, w_bar + 1);
	std::size_t w_start = 0;
	while (true)
	{
		const auto w_comma = w_arg.find(',', w_start);
		const std::string w_uuid = w_arg.substr(w_start,
			w_comma == std::string::npos ? std::string::npos : w_comma - w_start);
		ObjectXmlReply->InsertNextValue(w_uuid.empty() ? std::string() : repository.getObjectXml(w_uuid));
		if (w_comma == std::string::npos)
			break;
		w_start = w_comma + 1;
	}
}

//----------------------------------------------------------------------------
vtkStringArray* vtkETPSource::GetObjectXml()
{
	if (!ObjectXmlReply)
		ObjectXmlReply = vtkSmartPointer<vtkStringArray>::New();
	return ObjectXmlReply;
}

//----------------------------------------------------------------------------
void vtkETPSource::SetObjectUuidsKey(const char* key)
{
	if (!ObjectUuidsReply)
		ObjectUuidsReply = vtkSmartPointer<vtkStringArray>::New();
	const std::string w_key = key == nullptr ? std::string() : std::string(key);
	if (w_key.empty())
	{
		ObjectUuidsReply->SetNumberOfValues(0);
		ObjectUuidsKeyCached.clear();
		return;
	}
	if (w_key == ObjectUuidsKeyCached)
		return;
	std::string w_type = w_key;
	const auto w_bar = w_type.find('|');
	if (w_bar != std::string::npos)
		w_type.erase(0, w_bar + 1);
	ObjectUuidsReply->SetNumberOfValues(0);
	for (const std::string& w_uuid : repository.getObjectUuids(w_type))
		ObjectUuidsReply->InsertNextValue(w_uuid);
	ObjectUuidsKeyCached = w_key;
}

//----------------------------------------------------------------------------
vtkStringArray* vtkETPSource::GetObjectUuids()
{
	if (!ObjectUuidsReply)
		ObjectUuidsReply = vtkSmartPointer<vtkStringArray>::New();
	return ObjectUuidsReply;
}

//----------------------------------------------------------------------------
void vtkETPSource::SetObjectFieldsKey(const char* key)
{
	if (!ObjectFieldsReply)
		ObjectFieldsReply = vtkSmartPointer<vtkStringArray>::New();
	ObjectFieldsReply->SetNumberOfValues(0);
	if (key == nullptr || key[0] == '\0')
		return;
	// "<tag>|<uuid>" — the tag is the caller's cache key, not ours
	std::string w_uuid(key);
	const auto w_bar = w_uuid.find('|');
	if (w_bar != std::string::npos)
		w_uuid.erase(0, w_bar + 1);
	for (const std::string& w_line : repository.getObjectFields(w_uuid))
		ObjectFieldsReply->InsertNextValue(w_line);
}

//----------------------------------------------------------------------------
vtkStringArray* vtkETPSource::GetObjectFields()
{
	if (!ObjectFieldsReply)
		ObjectFieldsReply = vtkSmartPointer<vtkStringArray>::New();
	return ObjectFieldsReply;
}
