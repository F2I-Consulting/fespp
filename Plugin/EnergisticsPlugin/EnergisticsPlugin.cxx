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
										 Controller(nullptr),
										 AssemblyTag(0),
										 MarkerOrientation(true),
										 MarkerSize(10)

{
	//SetNumberOfInputPorts(0);
	//SetNumberOfOutputPorts(1);

	this->SetController(vtkMultiProcessController::GetGlobalController());

  	vtkMultiProcessController *controller = vtkMultiProcessController::GetGlobalController();


	this->repository = new ResqmlDataRepositoryToVtkPartitionedDataSetCollection(this->Controller);
}

//----------------------------------------------------------------------------
EnergisticsPlugin::~EnergisticsPlugin()
{
	this->SetController(nullptr);
	delete this->repository;
}

//----------------------------------------------------------------------------
int EnergisticsPlugin::FillOutputPortInformation(int vtkNotUsed(port), vtkInformation *info)
{
	info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkPartitionedDataSetCollection");
	return 1;
}

//----------------------------------------------------------------------------
void EnergisticsPlugin::SetFileName(const char* fname)
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
void EnergisticsPlugin::AddFileName(const char* fname)
{
  if (fname != nullptr && !this->FileNames.insert(fname).second)
  {
    this->Modified();
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
const char* EnergisticsPlugin::GetFileName(int index) const
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

/*****************************************
	*  OLD => DELETE ?
//----------------------------------------------------------------------------
int vtkEPCReader::GetFilesListArrayStatus(const char *file)
{
	return FilesList->ArrayIsEnabled(file);
}

//----------------------------------------------------------------------------
int vtkEPCReader::GetNumberOfFilesListArrays()
{
	return FilesList->GetNumberOfArrays();
}

//----------------------------------------------------------------------------
const char *vtkEPCReader::GetFilesListArrayName(int index)
{
	return FilesList->GetArrayName(index);
}

//----------------------------------------------------------------------------
void vtkEPCReader::SetFilesList(const char *file, int status)
{
	if (status)
	{
		if (strlen(file) != 0) {
			std::string extension = vtksys::SystemTools::GetFilenameExtension(std::string(fileName));
			
			// const std::string fileStr(file);
			// FilesList->AddArray(file, status);
			// const std::string extension = std::string(fileName).length() > 3
			// 						  ? std::string(fileName).substr(std::string(fileName).length() - 3, 3)
			// 						  : "";

			if (extension == "epc")
			{
				repository.addEpcDocument(std::string(fileName))
			}
		}
	}
	//******* TODO ********
	// status disable 
}
*/

//----------------------------------------------------------------------------
vtkDataArraySelection* EnergisticsPlugin::GetEntitySelection(int type)
{
  if (type < 0 || type >= NUMBER_OF_ENTITY_TYPES)
  {
    vtkErrorMacro("Invalid type '" << type
                                   << "'. Supported values are "
                                      "EnergisticsPlugin::WELL_TRAJ (0), ... EnergisticsPlugin::SUB_REP ("
                                   << EnergisticsPlugin::SUB_REP << ").");
    return nullptr;
  }
  return this->EntitySelection[type];
}

//----------------------------------------------------------------------------
void EnergisticsPlugin::RemoveAllEntitySelections()
{
  for (int cc = ENTITY_START; cc < ENTITY_END; ++cc)
  {
    this->GetEntitySelection(cc)->RemoveAllArrays();
  }
}

//----------------------------------------------------------------------------
const char* EnergisticsPlugin::GetDataAssemblyNodeNameForEntityType(int type)
{
  switch (type)
  {
    case WELL_TRAJ:
      return "wellbore_trajectory";
    case WELL_MARKER:
      return "wellbore_marker";
    case WELL_MARKER_FRAME:
      return "wellbore_marker_frame";
    case WELL_FRAME:
      return "wellbore_frame";
    case POLYLINE_SET:
      return "polyline_set";
    case TRIANGULATED_SET:
      return "triangulated_set";
    case GRID_2D:
      return "grid_2D";
    case IJK_GRID:
      return "ijk_grid";
    case UNSTRUC_GRID:
      return "unstructured_grid";
    case SUB_REP:
      return "sub_representation";
    default:
      vtkLogF(ERROR, "Invalid type '%d'", type);
      return nullptr;
  }
}

//----------------------------------------------------------------------------
vtkDataAssembly* EnergisticsPlugin::GetAssembly()
{
  return this->Assembly;
}


//----------------------------------------------------------------------------
bool EnergisticsPlugin::AddSelector(const char* selector)
{
  if (selector != nullptr && this->Selectors.insert(selector).second)
  {
    this->Modified();
    return true;
  }

  return false;
}

//----------------------------------------------------------------------------
void EnergisticsPlugin::ClearSelectors()
{
  if (!this->Selectors.empty())
  {
    this->Selectors.clear();
    this->Modified();
  }
}

//----------------------------------------------------------------------------
int EnergisticsPlugin::GetNumberOfSelectors() const
{
  return this->Selectors.size();
}

//----------------------------------------------------------------------------
void EnergisticsPlugin::SetSelector(const char* selector)
{
  this->ClearSelectors();
  this->AddSelector(selector);
}

//----------------------------------------------------------------------------
const char* EnergisticsPlugin::GetSelector(int index) const
{
  if (index >= 0 && index < this->GetNumberOfSelectors())
  {
    auto iter = std::next(this->Selectors.begin(), index);
    return iter->c_str();
  }
  return nullptr;
}

//----------------------------------------------------------------------------
void EnergisticsPlugin::setMarkerOrientation(bool orientation)
{
	/******* TODO ********/
}

//----------------------------------------------------------------------------
void EnergisticsPlugin::setMarkerSize(int size)
{
	/******* TODO ********/
}

//----------------------------------------------------------------------------
int EnergisticsPlugin::RequestInformation(
	vtkInformation* metadata,
	vtkInformationVector **,
	vtkInformationVector *)
{
  vtkLogScopeF(TRACE, "ReadMetaData");
  /*
  auto& internals = (*this->Internals);
  if (!internals.UpdateDatabaseNames(this))
  {
    return 0;
  }

  // read time information and generate that.
  if (!internals.UpdateTimeInformation(this))
  {
    return 0;
  }
  else
  {
    // add timesteps to metadata
    const auto& timesteps = internals.GetTimeSteps();
    if (!timesteps.empty())
    {
      metadata->Set(vtkStreamingDemandDrivenPipeline::TIME_STEPS(), &timesteps[0],
        static_cast<int>(timesteps.size()));
      double time_range[2] = { timesteps.front(), timesteps.back() };
      metadata->Set(vtkStreamingDemandDrivenPipeline::TIME_RANGE(), time_range, 2);
    }
    else
    {
      metadata->Remove(vtkStreamingDemandDrivenPipeline::TIME_STEPS());
      metadata->Remove(vtkStreamingDemandDrivenPipeline::TIME_RANGE());
    }
  }

  // read field/entity selection meta-data. i.e. update vtkDataArraySelection
  // instances for all available entity-blocks, entity-sets, and their
  // corresponding data arrays.
  if (!internals.UpdateEntityAndFieldSelections(this))
  {
    return 0;
  }

  // read assembly information.
  if (!internals.UpdateAssembly(this, &this->AssemblyTag))
  {
    return 0;
  }

  metadata->Set(vtkAlgorithm::CAN_HANDLE_PIECE_REQUEST(), 1);
  */
  return 1;
}

//----------------------------------------------------------------------------
int EnergisticsPlugin::RequestData(vtkInformation *,
								   vtkInformationVector **,
								   vtkInformationVector *outputVector)
{

	if (!FileNames.empty())
	{
		vtkInformation *outInfo = outputVector->GetInformationObject(0);
		vtkPartitionedDataSetCollection *output = vtkPartitionedDataSetCollection::SafeDownCast(outInfo->Get(vtkPartitionedDataSetCollection::DATA_OBJECT()));

		output->DeepCopy(repository->getVtkPartionedDatasSetCollection());
	}
	return 1;
}

//----------------------------------------------------------------------------
void EnergisticsPlugin::PrintSelf(ostream &os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "WELL_TRAJ selection: " << endl;
  this->GetWellTrajSelection()->PrintSelf(os, indent.GetNextIndent());
  os << indent << "WELL_MARKER selection: " << endl;
  this->GetWellMarkerSelection()->PrintSelf(os, indent.GetNextIndent());
  os << indent << "WELL_MARKER_FRAME selection: " << endl;
  this->GetWellMarkerFrameSelection()->PrintSelf(os, indent.GetNextIndent());
  os << indent << "WELL_FRAME selection: " << endl;
  this->GetWellFrameSelection()->PrintSelf(os, indent.GetNextIndent());

  os << indent << "POLYLINE_SET selection: " << endl;
  this->GetWellPolylineSetSelection()->PrintSelf(os, indent.GetNextIndent());
  os << indent << "TRIANGULATED_SET selection: " << endl;
  this->GetWellTriangulatedFrameSelection()->PrintSelf(os, indent.GetNextIndent());
  os << indent << "GRID_2D selection: " << endl;
  this->GetWellGrid2DSelection()->PrintSelf(os, indent.GetNextIndent());

  os << indent << "IJK_GRID selection: " << endl;
  this->GetWellIjkGridSelection()->PrintSelf(os, indent.GetNextIndent());
  os << indent << "UNSTRUC_GRID selection: " << endl;
  this->GetWellUnstructuredGridSelection()->PrintSelf(os, indent.GetNextIndent());
  os << indent << "SUB_REP selection: " << endl;
  this->GetWellSubRepSelection()->PrintSelf(os, indent.GetNextIndent());
}
