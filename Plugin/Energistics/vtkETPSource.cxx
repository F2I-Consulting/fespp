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
#include <vtkDataObject.h>
#include <vtkIndent.h>
#include <vtkInformation.h>
#include <vtkInformationVector.h>
#include <vtkMultiProcessController.h>
#include <vtkObjectFactory.h>
#include <vtkPartitionedDataSet.h>
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
void vtkETPSource::confirmConnectionClicked()
{
	vtkNew<vtkCustomProgressBar>  progressBar;
	progressBar->setWindowName("connection...");
	progressBar->setIndeterminate(true);

	std::thread t1([&progressBar] { progressBar->task1(); });
	t1.detach();
	try
	{
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
	progressBar->setIndeterminate(false);
}

//----------------------------------------------------------------------------
void vtkETPSource::disconnectionClicked()
{
	repository.disconnect();
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
		vtkNew<vtkCustomProgressBar>  progressBar;
		progressBar->setWindowName("get dataSpace");
		progressBar->setIndeterminate(true);

		std::thread t1([&progressBar] { progressBar->task1(); });
		t1.detach();

		if (std::string(dataspaces) != "")
		{
			std::string msg = repository.addDataspace(dataspaces);
			if (!msg.empty())
			{
				vtkWarningMacro(<< msg);
			}
		}
		AssemblyTag++;

		progressBar->setIndeterminate(false);
	}
}

//----------------------------------------------------------------------------
bool vtkETPSource::AddSelector(const char* selector)
{
	if (selector != nullptr && selectors.insert(selector).second)
	{
		int node_id = GetAssembly()->GetFirstNodeByPath(selector);
		repository.selectNodeId(node_id);
		_newSelection = true;
		Modified();
		Modified();
		Update();
		UpdateDataObject();
		UpdateInformation();
		UpdateWholeExtent();
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
	if (ConnectionTag == 0 && _newSelection)
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
			vtkNew<vtkCustomProgressBar>  progressBar;
			progressBar->setWindowName("get data");
			progressBar->setIndeterminate(true);

			std::thread t1([&progressBar] { progressBar->task1(); });
			t1.detach();

			vtkPartitionedDataSetCollection::GetData(outInfo)->DeepCopy(repository.getVtkPartitionedDatasSetCollection(requestedTimeStep));

			_newSelection = false;
			
			if (vtkPartitionedDataSetCollection::GetData(outInfo)->GetNumberOfPartitionedDataSets() > 0 && !colorApplyLoading) {
				colorApplyLoading = true;
				repository.addResqmlColor();
				colorApplyLoading = false;
			}

			progressBar->setIndeterminate(false);
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
