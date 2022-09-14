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
#include "EnergisticsPlugin.h"

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

vtkStandardNewMacro(EnergisticsPlugin);
vtkCxxSetObjectMacro(EnergisticsPlugin, Controller, vtkMultiProcessController);

//----------------------------------------------------------------------------
EnergisticsPlugin::EnergisticsPlugin() : FileNames(),
                                         FilesNames(vtkStringArray::New()),
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
void EnergisticsPlugin::SetFileName(const char *fname)
{
  if (fname == nullptr)
  {
    ClearFileNames();
    return;
  }

  if (this->FileNames.size() == 1 && *this->FileNames.begin() == fname)
  {
    return;
  }

  this->FileNames.clear();
  this->FileNames.insert(fname);
  this->Modified();
}

//----------------------------------------------------------------------------
void EnergisticsPlugin::AddFileName(const char *fname)
{
  if (fname != nullptr) 
  {
          this->FilesNames->InsertNextValue(fname);
  }
}

//----------------------------------------------------------------------------
void EnergisticsPlugin::ClearFileNames()
{
  if (!this->FileNames.empty())
  {
    this->FileNames.clear();
    this->Modified();
  }
}

//----------------------------------------------------------------------------
const char *EnergisticsPlugin::GetFileName(int index) const
{
  if (this->FileNames.size() > index)
  {
    auto iter = std::next(this->FileNames.begin(), index);
    return iter->c_str();
  }
  return nullptr;
}

//----------------------------------------------------------------------------
size_t EnergisticsPlugin::GetNumberOfFileNames() const
{
  return this->FileNames.size();
}

//------------------------------------------------------------------------------
vtkStringArray *EnergisticsPlugin::GetAllFilesNames()
{
    if (this->FilesNames->GetNumberOfValues() > 0)
    {
        for (auto index = 0; index < this->FilesNames->GetNumberOfValues(); index++) {
            auto file_property = this->FilesNames->GetValue(index);
            auto search = this->FileNamesLoaded.find(file_property); 
            if (search == this->FileNamesLoaded.end()) {
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
    return this->FilesNames;
}

//------------------------------------------------------------------------------
void EnergisticsPlugin::SetFiles(const std::string &file)
{
    if (file != "0") {
             this->FilesNames->InsertNextValue(file);
    }
}

//----------------------------------------------------------------------------
bool EnergisticsPlugin::AddSelector(const char *selector)
{
  if (selector != nullptr)
  {
    int node_id = GetAssembly()->GetFirstNodeByPath(selector);
    
    if (node_id == -1)
    {
        selectorNotLoaded.insert(std::string(selector));
    }
    else
    {
        this->repository.selectNodeId(node_id);
        this->Modified();
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
void EnergisticsPlugin::ClearSelectors()
{
  this->repository.clearSelection();
  if (!this->selectors.empty())
  {
    this->selectors.clear();
    this->Modified();
  }
}

//----------------------------------------------------------------------------
int EnergisticsPlugin::GetNumberOfSelectors() const
{
  return this->selectors.size();
}

//----------------------------------------------------------------------------
void EnergisticsPlugin::SetSelector(const char *selector)
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
const char *EnergisticsPlugin::GetSelector(int index) const
{
  if (index >= 0 && index < this->GetNumberOfSelectors())
  {
    auto iter = std::next(this->selectors.begin(), index);
    return iter->c_str();
  }
  return nullptr;
}

//----------------------------------------------------------------------------
void EnergisticsPlugin::setMarkerOrientation(bool orientation)
{
  this->repository.setMarkerOrientation(orientation);
  this->Modified();
  this->Update();
  this->UpdateDataObject();
  this->UpdateInformation();
  this->UpdateWholeExtent();
}

//----------------------------------------------------------------------------
void EnergisticsPlugin::setMarkerSize(int size)
{
  this->repository.setMarkerSize(size);
  this->Modified();
  this->Update();
  this->UpdateDataObject();
  this->UpdateInformation();
  this->UpdateWholeExtent();
}

//----------------------------------------------------------------------------
int EnergisticsPlugin::RequestData(vtkInformation *,
                                   vtkInformationVector **,
                                   vtkInformationVector *outputVector)
{
    auto outInfo = outputVector->GetInformationObject(0);
    auto output = vtkPartitionedDataSetCollection::GetData(outInfo);
    /*
  vtkInformation *outInfo = outputVector->GetInformationObject(0);
  vtkPartitionedDataSetCollection *output = vtkPartitionedDataSetCollection::SafeDownCast(outInfo->Get(vtkPartitionedDataSetCollection::DATA_OBJECT()));
  */
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
void EnergisticsPlugin::PrintSelf(ostream &os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
vtkDataAssembly *EnergisticsPlugin::GetAssembly()
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
