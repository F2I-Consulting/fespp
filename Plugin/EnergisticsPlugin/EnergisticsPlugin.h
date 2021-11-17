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
#ifndef __EnergisticsPlugin_h
#define __EnergisticsPlugin_h

// include system
#include <string>
#include <vector>
#include <set>

#include <vtkSmartPointer.h>
#include <vtkPartitionedDataSetCollectionAlgorithm.h>

#include "EnergisticsPluginModule.h"
#include "ResqmlMapping/ResqmlDataRepositoryToVtkPartitionedDataSetCollection.h"

class vtkDataArraySelection;
class vtkDataAssembly;
class vtkMultiProcessController;

/**
 * A VTK reader for EPC document.
 */
class ENERGISTICSPLUGIN_EXPORT EnergisticsPlugin : public vtkPartitionedDataSetCollectionAlgorithm
{
public:
	static EnergisticsPlugin *New();
	vtkTypeMacro(EnergisticsPlugin, vtkPartitionedDataSetCollectionAlgorithm);
	void PrintSelf(ostream &os, vtkIndent indent) final;

	// --------------- PART: files------ -------------

	///@{
	/**
    * API to set the filenames.
    */
	void AddFileName(const char *fname);
	void ClearFileNames();
	const char *GetFileName(int index) const;
	int GetNumberOfFileNames() const;
	///@}

	/**
   * Set a single filename. Note, this will clear all existing filenames.
   */
	void SetFileName(const char *fname);

	/*****************************************
	*  OLD => DELETE ?
	// Description:
	// Specify file name of the .epc file.
	vtkSetStringMacro(FileName)
	vtkGetStringMacro(FileName)

	
	vtkGetObjectMacro(FilesList, vtkDataArraySelection) void SetFilesList(const char *file, int status);
	// Used by Paraview, derived from the attribute UuidList
	int GetFilesListArrayStatus(const char *name);
	int GetNumberOfFilesListArrays();
	const char *GetFilesListArrayName(int index);
	*******************************************/

	// --------------- PART: Multi-Processor -------------

	///@{
	/**
	* Description:
	* Get/set the multi process controller to use for coordinated reads.
	* By default, set to the global controller.
	*/
	vtkGetObjectMacro(Controller, vtkMultiProcessController);
	void SetController(vtkMultiProcessController *controller);
	///@}

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
	static const char *GetDataAssemblyNodeNameForEntityType(int type);

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

	void RemoveAllEntitySelections();
	
	///@{
	/**
    * Assemblies provide yet another way of selection blocks/sets to load, if
    * available in the dataset. If a block (or set) is enabled either in the
    * block (or set) selection or using assembly selector then it is treated as
    * enabled and will be read.
    *
    * This method returns the vtkDataAssembly. he can have multiple
    * assemblies, all are nested under the root "Assemblies" node.
    *
    * If the file has no assemblies, this will return nullptr.
    */
	vtkDataAssembly *GetAssembly();
	///@}

	/**
   	* Whenever the assembly is changed, this tag gets changed. Note, users should
   	* not assume that this is monotonically increasing but instead simply rely on
   	* its value to determine if the assembly may have changed since last time.
   	*
	* It is set to 0 whenever there's no valid assembly available.
   	*/
	vtkGetMacro(AssemblyTag, int);

	///@{
	/**
   * API to specify selectors that indicate which branches on the assembly are
   * chosen.
   */
	bool AddSelector(const char *selector);
	void ClearSelectors();
	void SetSelector(const char *selector);
	///@}

	///@{
	/**
    * API to access selectors.
    */
	int GetNumberOfSelectors() const;
	const char *GetSelector(int index) const;
	///@}

	// --------------- PART: Properties ---------------------

	///@{
	/**
   	* Wellbore marker properties
	*/
	void setMarkerOrientation(bool orientation);
	void setMarkerSize(int size);
	///@}

protected:
	EnergisticsPlugin();
	~EnergisticsPlugin() final;

	int FillOutputPortInformation(int port, vtkInformation *info) override;

private:
	EnergisticsPlugin(const EnergisticsPlugin &);

	/*****************************************
	*  OLD => DELETE ?
	//  pipename
	char *FileName;
	vtkDataArraySelection *FilesList; // files loaded
	*******************************************/

	// files
	std::set<std::string> FileNames;

	// multi-processor
	vtkMultiProcessController *Controller;

	// treeview
	int AssemblyTag;
	vtkSmartPointer<vtkDataAssembly> Assembly;
	vtkNew<vtkDataArraySelection> EntitySelection[NUMBER_OF_ENTITY_TYPES];
	std::set<std::string> Selectors;
	
	// Properties
	bool MarkerOrientation;
	int MarkerSize;

	int RequestInformation(	vtkInformation* metadata, vtkInformationVector **, vtkInformationVector *) override;
	int RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *) override;

	ResqmlDataRepositoryToVtkPartitionedDataSetCollection *repository;
};
#endif
