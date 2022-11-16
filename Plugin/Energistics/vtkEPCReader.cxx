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
#include "vtkEPCReader.h"

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

vtkStandardNewMacro(vtkEPCReader);
vtkCxxSetObjectMacro(vtkEPCReader, Controller, vtkMultiProcessController);

//----------------------------------------------------------------------------
vtkEPCReader::vtkEPCReader() : Files(vtkStringArray::New()),
                               Controller(nullptr),
                               AssemblyTag(0),
                               MarkerOrientation(true),
                               MarkerSize(10)
{
  SetNumberOfInputPorts(0);
  SetNumberOfOutputPorts(1);

  this->SetController(vtkMultiProcessController::GetGlobalController());
}

//----------------------------------------------------------------------------
void vtkEPCReader::AddFileNameToFiles(const char *fname)
{
  if (fname != nullptr)
  {
    this->Files->InsertNextValue(fname);
  }
}

//----------------------------------------------------------------------------
void vtkEPCReader::ClearFileName()
{
}

//----------------------------------------------------------------------------
const char *vtkEPCReader::GetFileName(int index) const
{
  if (this->Files->GetNumberOfValues() > index)
  {
    return this->Files->GetValue(index).c_str();
  }
  return nullptr;
}

//----------------------------------------------------------------------------
size_t vtkEPCReader::GetNumberOfFileNames() const
{
  return this->Files->GetNumberOfValues();
}

//------------------------------------------------------------------------------
vtkStringArray *vtkEPCReader::GetAllFiles() // call only by GUI
{
  if (this->Files->GetNumberOfValues() > 0)
  {
    for (auto index = 0; index < this->Files->GetNumberOfValues(); index++)
    {
      auto file_property = this->Files->GetValue(index);
      auto search = this->FileNamesLoaded.find(file_property);
      if (search == this->FileNamesLoaded.end())
      {
        std::string msg = this->repository.addFile(file_property.c_str());
        this->FileNamesLoaded.insert(file_property);
        // add selector
        for (auto selector : selectorNotLoaded)
        {
          if (AddSelector(selector.c_str()))
          {
            selectorNotLoaded.erase(selector);
          }
        }
        if (!msg.empty())
        {
          vtkWarningMacro(<< msg);
        }

        this->AssemblyTag++;
        this->Modified();
        this->Update();
      }
    }
  }
  return this->Files;
}

//------------------------------------------------------------------------------

void vtkEPCReader::SetFiles(const std::string &file)
{
  if (file != "0")
  {
    bool exist = false;
    for (auto index = 0; index < this->Files->GetNumberOfValues(); ++index)
    {
      auto file_property = this->Files->GetValue(index);
      if (file_property == file)
      {
        exist = true;
      }
    }
    if (!exist)
    {
      this->Files->InsertNextValue(file);
    }
  }
  if (this->Controller->GetLocalProcessId() > 0) // pvserver without GUI
  {
    this->GetAllFiles();
  }
}

//----------------------------------------------------------------------------
bool vtkEPCReader::AddSelector(const char *path)
{
  if (path != nullptr && this->selectors.insert(path).second)
  {
    int node_id = GetAssembly()->GetFirstNodeByPath(path);

    if (node_id == -1)
    {
      selectorNotLoaded.insert(std::string(path));
    }
    else
    {
      std::string selection_parent = this->repository.selectNodeId(node_id);
      /*
             if (GetAssembly()->HasAttribute(node_id, "traj"))
             {
                 int node_id_parent = GetAssembly()->GetAttributeOrDefault(node_id, "traj", 0);
             }
      */
      Modified();
      Update();
      UpdateDataObject();
      UpdateInformation();
      UpdateWholeExtent();
      return true;
    }
  }
  return false;
}

//----------------------------------------------------------------------------
void vtkEPCReader::ClearSelectors()
{
  this->repository.clearSelection();
  if (!this->selectors.empty())
  {
    this->selectors.clear();
    this->Modified();
  }
}

//----------------------------------------------------------------------------
int vtkEPCReader::GetNumberOfSelectors() const
{
  if (selectors.size() > (std::numeric_limits<int>::max)())
  {
    throw std::out_of_range("Too much selectors.");
  }
  return static_cast<int>(selectors.size());
}

//----------------------------------------------------------------------------
void vtkEPCReader::SetSelector(const char *selector)
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
const char *vtkEPCReader::GetSelector(int index) const
{
  if (index >= 0 && index < this->GetNumberOfSelectors())
  {
    auto iter = std::next(this->selectors.begin(), index);
    return iter->c_str();
  }
  return nullptr;
}

//----------------------------------------------------------------------------
void vtkEPCReader::setMarkerOrientation(bool orientation)
{
  this->repository.setMarkerOrientation(orientation);
  this->Modified();
  this->Update();
  this->UpdateDataObject();
  this->UpdateInformation();
  this->UpdateWholeExtent();
}

//----------------------------------------------------------------------------
void vtkEPCReader::setMarkerSize(int size)
{
  this->repository.setMarkerSize(size);
  this->Modified();
  this->Update();
  this->UpdateDataObject();
  this->UpdateInformation();
  this->UpdateWholeExtent();
}

//----------------------------------------------------------------------------
int vtkEPCReader::RequestData(vtkInformation *,
                              vtkInformationVector **,
                              vtkInformationVector *outputVector)
{
  // Load state (load selection in wait)
  if (this->selectorNotLoaded.size() > 0)
  {
    for (auto path : this->selectorNotLoaded)
    {
      int node_id = GetAssembly()->GetFirstNodeByPath(path.c_str());
      if (node_id > -1)
      {
        std::string selection_parent = this->repository.selectNodeId(node_id);
        this->selectorNotLoaded.erase(path);
      }
    }
  }

  auto *outInfo = outputVector->GetInformationObject(0);
  outInfo->Remove(vtkStreamingDemandDrivenPipeline::TIME_STEPS());
  const std::vector<double> times = this->repository.getTimes();
  if (times.size() > (std::numeric_limits<int>::max)())
  {
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
    vtkPartitionedDataSetCollection::GetData(outInfo)->DeepCopy(this->repository.getVtkPartitionedDatasSetCollection(requestedTimeStep, this->Controller->GetNumberOfProcesses(), this->Controller->GetLocalProcessId()));
  }
  catch (const std::exception &e)
  {
    vtkWarningMacro(<< e.what());
  }

  return 1;
}

//----------------------------------------------------------------------------
void vtkEPCReader::PrintSelf(ostream &os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
vtkDataAssembly *vtkEPCReader::GetAssembly()
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
