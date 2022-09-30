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
#ifndef __EnergisticsPluginSource_h
#define __EnergisticsPluginSource_h

// include system
#include <string>
#include <set>

#include <vtkPartitionedDataSetCollectionAlgorithm.h>
#include <vtkSmartPointer.h>
#include <vtkStringArray.h>

#include "EnergisticsPluginModule.h"
#include "ResqmlMapping/ResqmlDataRepositoryToVtkPartitionedDataSetCollection.h"

class vtkDataAssembly;
class vtkMultiProcessController;

/**
 * A VTK reader for EPC document.
 */
class ENERGISTICSPLUGIN_EXPORT EnergisticsPluginSource : public vtkPartitionedDataSetCollectionAlgorithm
{
public:
	static EnergisticsPluginSource *New();
	vtkTypeMacro(EnergisticsPluginSource, vtkPartitionedDataSetCollectionAlgorithm);
	void PrintSelf(ostream &os, vtkIndent indent) final;

	// --------------- PART Properties------ -------------

	///@{
	/**
	 * Set the Ip for ETP connection.
	 */
	void setETPUrlConnection(char * etp_url);
	///@}
		
	///@{
	/**
	 * Set the Port for ETP connection.
	 */
	void setDataPartition(char * data_partition);
	///@}
	
	 void setAuthType(int auth_type);

	///@{
	/**
	 * Set the Auth for ETP connection.
	 */
	void setAuthPwd(char* auth_connection);
	///@}

	void confirmConnectionClicked();

	///@{
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

	///@{

protected:
	EnergisticsPluginSource();
	~EnergisticsPluginSource() override;

	int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
	int RequestInformation(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

private:
	EnergisticsPluginSource(const EnergisticsPluginSource&) = delete;
	void operator=(const EnergisticsPluginSource&) = delete;

	std::string ETPUrl;
	std::string DataPartition;
	std::string Authentification;
	std::string AuthPwd;

	// treeview
	std::set<std::string> selectors;
	int AssemblyTag;

	// Properties
	bool MarkerOrientation;
	int MarkerSize;

	ResqmlDataRepositoryToVtkPartitionedDataSetCollection repository;
};
#endif
