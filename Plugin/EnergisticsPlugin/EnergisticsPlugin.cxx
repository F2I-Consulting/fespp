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

#include <algorithm>
#include <assert.h>

#include <vtkDataArraySelection.h>
#include <vtkIndent.h>
#include <vtkInformation.h>
#include <vtkInformationVector.h>
#include <vtkPartitionedDataSetCollection.h>
#include <vtkDataAssembly.h>
#include <vtkObjectFactory.h>
#include <vtkMultiProcessController.h>
#include <vtkLogger.h>

#include "ResqmlMapping/ResqmlDataRepositoryToVtkPartitionedDataSetCollection.h"

vtkStandardNewMacro(EnergisticsPlugin);
vtkCxxSetObjectMacro(EnergisticsPlugin, Controller, vtkMultiProcessController);

//----------------------------------------------------------------------------
EnergisticsPlugin::EnergisticsPlugin() : FileNames({}),
                                         FilesNames(vtkStringArray::New()),
                                         Controller(nullptr),
                                         MarkerOrientation(true),
                                         AssemblyTag(0),
                                         MarkerSize(10)
{
  SetNumberOfInputPorts(0);
  SetNumberOfOutputPorts(1);

  this->SetController(vtkMultiProcessController::GetGlobalController());

  vtkMultiProcessController *controller = vtkMultiProcessController::GetGlobalController();

  this->repository = new ResqmlDataRepositoryToVtkPartitionedDataSetCollection(this->Controller->GetLocalProcessId(), this->Controller->GetNumberOfProcesses());
}

//----------------------------------------------------------------------------
EnergisticsPlugin::~EnergisticsPlugin()
{
  this->SetController(nullptr);
//  delete this->repository;
}

//----------------------------------------------------------------------------
void EnergisticsPlugin::SetFileName(const char *fname)
{
  if (fname == nullptr)
  {
    if (!this->FileNames.empty())
    {
      this->FileNames.clear();
      this->Modified();
    }
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
  if (fname != nullptr) //&& !this->FileNames.insert(fname).second)
  {
    this->FilesNames->InsertNextValue(fname);
    this->repository->addFile(fname);
    this->dataAssembly = this->repository->getVtkPartionedDatasSetCollection()->GetDataAssembly();
    this->AssemblyTag++;
    this->Modified();
    this->Update();
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
int EnergisticsPlugin::GetNumberOfFileNames() const
{
  return this->FileNames.size();
}

//------------------------------------------------------------------------------
vtkStringArray *EnergisticsPlugin::GetAllFilesNames()
{
  return this->FilesNames;
}

//------------------------------------------------------------------------------
void EnergisticsPlugin::SetFiles(const std::string &file)
{
}

//----------------------------------------------------------------------------
bool EnergisticsPlugin::AddSelector(const char *selector)
{
  if (selector != nullptr && this->selectors.insert(selector).second)
  {
    auto node_id = this->dataAssembly->GetFirstNodeByPath(selector);
    this->repository->selectNodeId(node_id);
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
void EnergisticsPlugin::ClearSelectors()
{
  this->repository->clearSelection();
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
  this->repository->setMarkerOrientation(orientation);
}

//----------------------------------------------------------------------------
void EnergisticsPlugin::setMarkerSize(int size)
{
  this->repository->setMarkerSize(size);
}

//----------------------------------------------------------------------------
int EnergisticsPlugin::RequestInformation(
    vtkInformation *metadata,
    vtkInformationVector **,
    vtkInformationVector *outputVector)
{
  return 1;
}

//----------------------------------------------------------------------------
int EnergisticsPlugin::RequestData(vtkInformation *,
                                   vtkInformationVector **,
                                   vtkInformationVector *outputVector)
{
  vtkInformation *outInfo = outputVector->GetInformationObject(0);
  vtkPartitionedDataSetCollection *output = vtkPartitionedDataSetCollection::SafeDownCast(outInfo->Get(vtkPartitionedDataSetCollection::DATA_OBJECT()));
  output->DeepCopy(this->repository->getVtkPartionedDatasSetCollection());

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
  return dataAssembly;
}
