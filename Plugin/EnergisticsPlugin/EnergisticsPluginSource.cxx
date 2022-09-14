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
#include "EnergisticsPluginSource.h"

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

vtkStandardNewMacro(EnergisticsPluginSource);

//----------------------------------------------------------------------------
EnergisticsPluginSource::EnergisticsPluginSource() :
    IpConnection("51.210.100.205"),
    PortConnection(9002),
    AuthConnection("Basic Zm9vOmJhcg =="),
    AssemblyTag(0),
    MarkerOrientation(true),
    MarkerSize(10)
{
  SetNumberOfInputPorts(0);
  SetNumberOfOutputPorts(1);
}

//----------------------------------------------------------------------------
EnergisticsPluginSource::~EnergisticsPluginSource() = default;

//------------------------------------------------------------------------------
int EnergisticsPluginSource::RequestInformation(vtkInformation* vtkNotUsed(request),
    vtkInformationVector** vtkNotUsed(inputVector), vtkInformationVector* outputVector)
{
    vtkInformation* outInfo = outputVector->GetInformationObject(0);
    outInfo->Set(CAN_HANDLE_PIECE_REQUEST(), 1);
    return 1;
}

//----------------------------------------------------------------------------
void EnergisticsPluginSource::setIpConnection(char * ip_connection)
{
    this->IpConnection = std::string(ip_connection);
}


//----------------------------------------------------------------------------
void EnergisticsPluginSource::setPortConnection(int port_connection)
{
    this->PortConnection = port_connection;
}

//----------------------------------------------------------------------------
void EnergisticsPluginSource::setAuthConnection(char* auth_connection)
{
    this->AuthConnection = std::string(auth_connection);
}

//----------------------------------------------------------------------------
void EnergisticsPluginSource::confirmConnectionClicked()
{
        this->repository.connect(this->IpConnection, this->PortConnection, this->AuthConnection);
}

//----------------------------------------------------------------------------
bool EnergisticsPluginSource::AddSelector(const char *selector)
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
void EnergisticsPluginSource::ClearSelectors()
{
  this->repository.clearSelection();
  if (!this->selectors.empty())
  {
    this->selectors.clear();
    this->Modified();
  }
}

//----------------------------------------------------------------------------
int EnergisticsPluginSource::GetNumberOfSelectors() const
{
  return this->selectors.size();
}

//----------------------------------------------------------------------------
void EnergisticsPluginSource::SetSelector(const char *selector)
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
const char *EnergisticsPluginSource::GetSelector(int index) const
{
  if (index >= 0 && index < this->GetNumberOfSelectors())
  {
    auto iter = std::next(this->selectors.begin(), index);
    return iter->c_str();
  }
  return nullptr;
}

//----------------------------------------------------------------------------
void EnergisticsPluginSource::setMarkerOrientation(bool orientation)
{
  this->repository.setMarkerOrientation(orientation);
  this->Modified();
  this->Update();
  this->UpdateDataObject();
  this->UpdateInformation();
  this->UpdateWholeExtent();
}

//----------------------------------------------------------------------------
void EnergisticsPluginSource::setMarkerSize(int size)
{
  this->repository.setMarkerSize(size);
  this->Modified();
  this->Update();
  this->UpdateDataObject();
  this->UpdateInformation();
  this->UpdateWholeExtent();
}

//----------------------------------------------------------------------------
int EnergisticsPluginSource::RequestData(vtkInformation *,
                                   vtkInformationVector **,
                                   vtkInformationVector *outputVector)
{

    auto outInfo = outputVector->GetInformationObject(0);
    auto output = vtkPartitionedDataSetCollection::GetData(outInfo);
    
  outInfo->Remove(vtkStreamingDemandDrivenPipeline::TIME_STEPS());
  std::vector<double> times = this->repository.getTimes();
  double requestedTimeStep = 0;
  if (!times.empty())
  {
    std::pair<std::vector<double>::iterator, std::vector<double>::iterator> minmax = std::minmax_element(begin(times), end(times));
    outInfo->Set(vtkStreamingDemandDrivenPipeline::TIME_STEPS(), &times[0], times.size());
    static double timeRange[] = {*minmax.first, *minmax.second};
    outInfo->Set(vtkStreamingDemandDrivenPipeline::TIME_RANGE(), timeRange, 2);

    // current timeStep value
    requestedTimeStep = outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP());
  }
  try
  {
    output->DeepCopy(this->repository.getVtkPartitionedDatasSetCollection(requestedTimeStep));
  }
  catch (const std::exception &e)
  {
    vtkWarningMacro(<< e.what());
  }
  

  return 1;
}

//----------------------------------------------------------------------------
void EnergisticsPluginSource::PrintSelf(ostream &os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}


//----------------------------------------------------------------------------
vtkDataAssembly *EnergisticsPluginSource::GetAssembly()
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
