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
#ifndef __vtkEPCCollector_h
#define __vtkEPCCollector_h

// include system
#include <string>
#include <set>
#include <utility>

#include <vtkPartitionedDataSetCollectionAlgorithm.h>
#include <vtkSmartPointer.h>
#include <vtkStringArray.h>
#include <vtkIntArray.h>

#include <vtkVector.h>
#include <vtkScalarsToColors.h>


#include "EnergisticsModule.h"
#include "Mapping/ResqmlDataRepositoryToVtkPartitionedDataSetCollection.h"

class vtkDataAssembly;
class vtkMultiProcessController;
class vtkSMSourceProxy;
class vtkSMViewProxy;
class vtkPVRenderView;

/**
 * A VTK reader for EPC document.
 */
class ENERGISTICS_EXPORT  vtkEPCCollector : public vtkPartitionedDataSetCollectionAlgorithm
{
public:
	static vtkEPCCollector *New();
	vtkTypeMacro(vtkEPCCollector, vtkPartitionedDataSetCollectionAlgorithm);
	void PrintSelf(ostream &os, vtkIndent indent) final;

	// --------------- PART: files------ -------------

	///@{
	/**
	* API to set the filenames.
	*/
	void AddFileNameToFiles(const char* fname);
	void ClearFileName();
	const char* GetFileName(int index) const;
	size_t GetNumberOfFileNames() const;
	///@}

	 ///@{
  /**
   * Get/Set the scene to be used by the reader
   */
	void AddFiles(const std::string& file);
	///@}
	  /**
   * Get/Set the scene to be used by the reader
   */
	void SetFiles(const std::string& file);
	///@}

   /**
	 * Get a list all file names as a vtkStringArray.
	 */
	vtkStringArray* GetAllFiles();

	///@{


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

  /**
   * Assemblies provide yet another way of selection blocks/sets to load, if
   * available in the dataset. If a block (or set) is enabled either in the
   * block (or set) selection or using assembly selector then it is treated as
   * enabled and will be read.
   *
   * This method returns the vtkDataAssembly. Since IOSS can have multiple
   * assemblies, all are nested under the root "Assemblies" node.
   *
   * If the file has no assemblies, this will return nullptr.
   */
	vtkDataAssembly* GetAssembly();

  /**
   * Whenever the assembly is changed, this tag gets changed. Note, users should
   * not assume that this is monotonically increasing but instead simply rely on
   * its value to determine if the assembly may have changed since last time.
   *
   * It is set to 0 whenever there's no valid assembly available.
   */
  vtkGetMacro(AssemblyTag, int);

  vtkGetMacro(ExtractTag, int);

  	///@{
	/**
   * API to specify selectors that indicate which branches on the assembly are
   * chosen.
   */
	bool AddSelector(const char *selector);
	void ClearSelectors();
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

		///@{
     /**
	 * Get a list all file names as a vtkStringArray.
	 */
	void SetDataSetList(const char* name, int status);
	void ClearDataSetList();
	vtkStringArray* GetAllDataSet();
	///@}

			///@{
	 /**
	 * Get a list all file names as a vtkStringArray.
	 */
	void SetDataSetListForCopy(const char* name, int status);
	void ClearDataSetListForCopy();
	vtkStringArray* GetAllDataSetForCopy();
	///@}

	void ApplyColors();

protected:
	vtkEPCCollector();
	~vtkEPCCollector() final;

private:
	int RequestInformation(vtkInformation*, vtkInformationVector**, vtkInformationVector*) final;
	int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) final;

	vtkSMSourceProxy* GetThisProxy();

	void Extract(vtkSMSourceProxy*, int index);
	void Copy(vtkSMSourceProxy*, int index);
	void ClearExtractAndCopy();

	vtkStringArray* GetHierarchyBlocks(std::string type); // types: "COPY", "REFERENCE"

	// files
	vtkSmartPointer<vtkStringArray> Files;
	char* FileName;
	std::set<std::string> FilesList;
	std::set<std::string> FileNamesLoaded;

	std::set<std::string> selectorNotLoaded; // load state, load selector before files :(

	// multi-processor
	vtkMultiProcessController *Controller;

	// treeview / selector for load data
	std::set<std::string> selectors;

	// Tag for property visibility
	int AssemblyTag;
	int ExtractTag;

	bool extractExist;
	vtkSmartPointer<vtkStringArray> DataSetList;
	std::map<std::string, bool> DataSetListSelection;

	vtkSmartPointer<vtkStringArray> DataSetListForCopy;
	std::map<std::string, bool> DataSetListSelectionForCopy;

	// Wellbores Properties
	bool MarkerOrientation;
	int MarkerSize;

	// Resqml
	ResqmlDataRepositoryToVtkPartitionedDataSetCollection repository;

	bool colorApplyLoading;

};
#endif

