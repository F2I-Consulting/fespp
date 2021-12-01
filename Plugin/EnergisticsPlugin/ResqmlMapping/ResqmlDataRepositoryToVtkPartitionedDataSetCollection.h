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
#ifndef __ResqmlDataRepositoryToVtkPartitionedDataSetCollection_h
#define __ResqmlDataRepositoryToVtkPartitionedDataSetCollection_h

// include system
#include <string>
#include <map>
#include <vector>

#include <vtkSmartPointer.h>
#include <vtkPartitionedDataSetCollection.h>
#include <vtkMultiProcessController.h>

namespace common
{
    class DataObjectRepository;
}

namespace resqml2
{
    class AbstractRepresentation;
}

/** 
 * @brief	class description.
 */
class ResqmlDataRepositoryToVtkPartitionedDataSetCollection : public vtkPartitionedDataSetCollection
{
public:
    ResqmlDataRepositoryToVtkPartitionedDataSetCollection(int proc_number, int max_proc);
    ~ResqmlDataRepositoryToVtkPartitionedDataSetCollection() final;

    	// --------------- PART: TreeView ---------------------

	// different tab
	enum EntityType
	{
		WELL_TRAJ,
		WELL_MARKER,
		WELL_MARKER_FRAME,
		WELL_FRAME,
		POLYLINE_SET,
		TRIANGULATED_SET,
		GRID_2D,
		IJK_GRID,
		UNSTRUC_GRID,
		SUB_REP,
		NUMBER_OF_ENTITY_TYPES,

		INTERPRETATION_1D_START = WELL_TRAJ,
		INTERPRETATION_1D_END = POLYLINE_SET,
		INTERPRETATION_2D_START = POLYLINE_SET,
		INTERPRETATION_2D_END = IJK_GRID,
		INTERPRETATION_3D_START = IJK_GRID,
		INTERPRETATION_3D_END = NUMBER_OF_ENTITY_TYPES,

		ENTITY_START = WELL_TRAJ,
		ENTITY_END = NUMBER_OF_ENTITY_TYPES,
	};

	static bool GetEntityTypeIs1D(int type) { return (type >= INTERPRETATION_1D_START && type < INTERPRETATION_1D_END); }
	static bool GetEntityTypeIs2D(int type) { return (type >= INTERPRETATION_2D_START && type < INTERPRETATION_2D_END); }
	static bool GetEntityTypeIs3D(int type) { return (type >= INTERPRETATION_3D_START && type < INTERPRETATION_3D_END); }

	vtkDataArraySelection* GetEntitySelection(int type);
	vtkDataArraySelection* GetWellTrajSelection() { return this->GetEntitySelection(WELL_TRAJ); }
	vtkDataArraySelection* GetWellMarkerSelection() { return this->GetEntitySelection(WELL_MARKER); }
	vtkDataArraySelection* GetWellMarkerFrameSelection() { return this->GetEntitySelection(WELL_MARKER_FRAME); }
	vtkDataArraySelection* GetWellFrameSelection() { return this->GetEntitySelection(WELL_FRAME); }
	vtkDataArraySelection* GetWellPolylineSetSelection() { return this->GetEntitySelection(POLYLINE_SET); }
	vtkDataArraySelection* GetWellTriangulatedFrameSelection() { return this->GetEntitySelection(TRIANGULATED_SET); }
	vtkDataArraySelection* GetWellGrid2DSelection() { return this->GetEntitySelection(GRID_2D); }
	vtkDataArraySelection* GetWellIjkGridSelection() { return this->GetEntitySelection(IJK_GRID); }
	vtkDataArraySelection* GetWellUnstructuredGridSelection() { return this->GetEntitySelection(UNSTRUC_GRID); }
	vtkDataArraySelection* GetWellSubRepSelection() { return this->GetEntitySelection(SUB_REP); }

    vtkDataAssembly *GetAssembly();

    //---------------------------------
    
	void addFile(const char* file, int status);

    // Wellbore Options
    void setMarkerOrientation(bool orientation);
	void setMarkerSize(int size);

    vtkPartitionedDataSetCollection *getVtkPartionedDatasSetCollection() { return output; }

	void selectNodeId(int node);
    

protected:
private:
    ResqmlDataRepositoryToVtkPartitionedDataSetCollection(const ResqmlDataRepositoryToVtkPartitionedDataSetCollection&);

    std::string searchFaultPolylines(const std::string &fileName);
    std::string searchHorizonPolylines(const std::string &fileName);
    std::string searchUnstructuredGrid(const std::string &fileName);
    std::string searchFaultTriangulated(const std::string &fileName);
    std::string searchHorizonTriangulated(const std::string &fileName);
    std::string searchGrid2d(const std::string &fileName);
    std::string searchIjkGrid(const std::string &fileName);
    std::string searchWellboreTrajectory(const std::string &fileName);
    std::string searchSubRepresentation(const std::string &fileName);
    std::string searchTimeSeries(const std::string &fileName);
    std::string searchRepresentations(resqml2::AbstractRepresentation * representation, EntityType type);

	vtkNew<vtkDataArraySelection> EntitySelection[NUMBER_OF_ENTITY_TYPES];
    
	char* FileName; // pipename

    bool markerOrientation; 
    int markerSize;

 	int proc_number;
    int max_proc;
    
    common::DataObjectRepository *repository;

    vtkSmartPointer<vtkPartitionedDataSetCollection> output;

    std::map<int, std::string> nodeId_to_uuid; // index of VtkDataAssembly to Resqml uuid
	std::map<int, EntityType> nodeId_to_EntityType; // index of VtkDataAssembly to entity type
};
#endif