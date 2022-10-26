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

#include <exception>
#include <iterator>
#include <algorithm>

#include <vtkIndent.h>
#include <vtkInformation.h>
#include <vtkInformationVector.h>
#include <vtkPartitionedDataSetCollection.h>
#include <vtkDataAssembly.h>
#include <vtkObjectFactory.h>
#include <vtkMultiProcessController.h>
#include <vtkStreamingDemandDrivenPipeline.h>
#include <vtkDataObject.h>
#include <vtkStdString.h>

vtkStandardNewMacro(vtkETPSource);

//----------------------------------------------------------------------------
vtkETPSource::vtkETPSource() : ETPUrl("wss://osdu-ship.msft-osdu-test.org:443/oetp/reservoir-ddms/"),
DataPartition("opendes"),
Authentification("Bearer"),
AuthPwd(""),
AllDataspaces(vtkStringArray::New()),
                                                     AssemblyTag(0),
    ConnectionTag(1),
    DisconnectionTag(0),
                                                     MarkerOrientation(true),
                                                     MarkerSize(10)
{
  SetNumberOfInputPorts(0);
  SetNumberOfOutputPorts(1);
}

//----------------------------------------------------------------------------
vtkETPSource::~vtkETPSource() = default;

//------------------------------------------------------------------------------
int vtkETPSource::RequestInformation(vtkInformation *vtkNotUsed(request),
                                                vtkInformationVector **vtkNotUsed(inputVector), vtkInformationVector *outputVector)
{
  vtkInformation *outInfo = outputVector->GetInformationObject(0);
  outInfo->Set(CAN_HANDLE_PIECE_REQUEST(), 1);
  return 1;
}

//----------------------------------------------------------------------------
void vtkETPSource::setETPUrlConnection(char *etp_url)
{
  this->ETPUrl = std::string(etp_url);
}

//----------------------------------------------------------------------------
void vtkETPSource::setDataPartition(char *data_partition)
{
  this->DataPartition = std::string(data_partition);
}

//----------------------------------------------------------------------------
void vtkETPSource::setAuthType(int auth_type)
{
  if (auth_type == 0)
  {
    this->Authentification = "Bearer";
  }
  else if (auth_type == 1)
  {
    this->Authentification = "Basic";
  }
  else
  {
      this->Authentification = "unknow";
  }
}

//----------------------------------------------------------------------------
void vtkETPSource::setAuthPwd(char *auth_connection)
{
  this->AuthPwd = std::string(auth_connection);
}

//----------------------------------------------------------------------------
void vtkETPSource::confirmConnectionClicked()
{
  const auto dataspaces = this->repository.connect(this->ETPUrl, this->DataPartition, this->Authentification + " " + this->AuthPwd);
  this->AllDataspaces->InsertNextValue(vtkStdString(""));
  for (const std::string dataspace : dataspaces)
  {
      this->AllDataspaces->InsertNextValue(vtkStdString(dataspace));
  }

  this->ConnectionTag = 0;
  this->DisconnectionTag = 1;
  this->Modified();
  this->Update();
}

//----------------------------------------------------------------------------
void vtkETPSource::disconnectionClicked()
{
    this->repository.disconnect();
    this->Modified();
    Modified();
    Update();
    UpdateDataObject();
    UpdateInformation();
    UpdateWholeExtent();
}

//------------------------------------------------------------------------------
void vtkETPSource::SetDataspaces(const char* dataspaces)
{
    if (std::string(dataspaces) != "")
    {
        std::string msg = this->repository.addDataspace(dataspaces);
        if (!msg.empty())
        {
            vtkWarningMacro(<< msg);
        }
    }
    this->AssemblyTag++;
}

//----------------------------------------------------------------------------
bool vtkETPSource::AddSelector(const char *selector)
{
  if (selector != nullptr && this->selectors.insert(selector).second)
  {
    int node_id = GetAssembly()->GetFirstNodeByPath(selector);
    this->repository.selectNodeId(node_id);
    this->Modified();
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
  this->repository.clearSelection();
  if (!this->selectors.empty())
  {
    this->selectors.clear();
    this->Modified();
  }
}

//----------------------------------------------------------------------------
int vtkETPSource::GetNumberOfSelectors() const
{
  if (selectors.size() > (std::numeric_limits<int>::max)()) {
	throw std::out_of_range("Too much selectors.");
  }

  return static_cast<int>(selectors.size());
}

//----------------------------------------------------------------------------
void vtkETPSource::SetSelector(const char *selector)
{
  this->ClearSelectors();
  this->AddSelector(selector);

  Modified();
  Update();
  UpdateDataObject();
  UpdateInformation();
  UpdateWholeExtent();
}

//----------------------------------------------------------------------------
const char *vtkETPSource::GetSelector(int index) const
{
  if (index >= 0 && index < this->GetNumberOfSelectors())
  {
    auto iter = std::next(this->selectors.begin(), index);
    return iter->c_str();
  }
  return nullptr;
}

//----------------------------------------------------------------------------
void vtkETPSource::setMarkerOrientation(bool orientation)
{
  this->repository.setMarkerOrientation(orientation);
  this->Modified();
  this->Update();
  this->UpdateDataObject();
  this->UpdateInformation();
  this->UpdateWholeExtent();
}

//----------------------------------------------------------------------------
void vtkETPSource::setMarkerSize(int size)
{
  this->repository.setMarkerSize(size);
  this->Modified();
  this->Update();
  this->UpdateDataObject();
  this->UpdateInformation();
  this->UpdateWholeExtent();
}

//----------------------------------------------------------------------------
int vtkETPSource::RequestData(vtkInformation *,
                                         vtkInformationVector **,
                                         vtkInformationVector *outputVector)
{
  auto* outInfo = outputVector->GetInformationObject(0);
  outInfo->Remove(vtkStreamingDemandDrivenPipeline::TIME_STEPS());
  const std::vector<double> times = this->repository.getTimes();
  if (times.size() > (std::numeric_limits<int>::max)()) {
	throw std::out_of_range("Too much times.");
  }
  double requestedTimeStep = 0;
  if (!times.empty())
  {
    const auto minmax = std::minmax_element(begin(times), end(times));
	outInfo->Set(vtkStreamingDemandDrivenPipeline::TIME_STEPS(), &times[0], static_cast<int>(times.size()));
    static double timeRange[] = {*minmax.first, *minmax.second};
    outInfo->Set(vtkStreamingDemandDrivenPipeline::TIME_RANGE(), timeRange, 2);

    // current timeStep value
    requestedTimeStep = outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP());
  }

  try
  {
	  vtkPartitionedDataSetCollection::GetData(outInfo)->DeepCopy(this->repository.getVtkPartitionedDatasSetCollection(requestedTimeStep));
  }
  catch (const std::exception &e)
  {
    vtkWarningMacro(<< e.what());
  }

  return 1;
}

//----------------------------------------------------------------------------
void vtkETPSource::PrintSelf(ostream &os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
vtkDataAssembly *vtkETPSource::GetAssembly()
{
  try
  {
    return this->repository.GetAssembly();
  }
  catch (const std::exception &e)
  {
    vtkErrorMacro(<< e.what());
  }
}
