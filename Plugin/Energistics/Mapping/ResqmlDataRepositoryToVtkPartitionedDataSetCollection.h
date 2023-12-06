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
#include <set>

#include <vtkSmartPointer.h>
#include <vtkPartitionedDataSetCollection.h>
#include <vtkMultiProcessController.h>

#ifdef WITH_ETP_SSL
#include <fetpapi/etp/ClientSessionLaunchers.h>
#endif

#include "../Tools/enum.h"

namespace common
{
	class DataObjectRepository;
}

namespace resqml2
{
	class AbstractRepresentation;
	class AbstractObject;
}

class ResqmlAbstractRepresentationToVtkPartitionedDataSet;
class CommonAbstractObjectSetToVtkPartitionedDataSetSet;
class CommonAbstractObjectToVtkPartitionedDataSet;

/**
 * @brief	class description.
 */
class ResqmlDataRepositoryToVtkPartitionedDataSetCollection
{
public:
	ResqmlDataRepositoryToVtkPartitionedDataSetCollection();
	~ResqmlDataRepositoryToVtkPartitionedDataSetCollection();
	// --------------- PART: TreeView ---------------------
	vtkDataAssembly *GetAssembly() { return _output->GetDataAssembly(); }

	//---------------------------------

	// for EPC reader
	std::string addFile(const char *file);

	// for ETP source
	std::string addDataspace(const char *dataspace);

	// for ETP connection
	std::vector<std::string> connect(const std::string &etp_url, const std::string &data_partition, const std::string &auth_connection);
	void disconnect();

	// Wellbore Options
	void setMarkerOrientation(bool orientation);
	void setMarkerSize(int size);

	vtkPartitionedDataSetCollection *getVtkPartitionedDatasSetCollection(const double time, const int nbProcess = 1, const int processId = 0);

	std::vector<double> getTimes() { return times_step; }

	/**
	 * @return selection parent
	 */
	std::string selectNodeId(int node);
	void clearSelection();

private:
	std::string buildDataAssemblyFromDataObjectRepo(const char *fileName);

	std::string searchWellboreTrajectory(const std::string &fileName);
	std::string searchRepresentations(resqml2::AbstractRepresentation const *representation, int idNode = 0 /* 0 is root's id*/);

	std::string searchSubRepresentation(resqml2::AbstractRepresentation const *representation, vtkDataAssembly *assembly, int node_parent);
	std::string searchTimeSeries(const std::string &fileName);

	std::string searchProperties(resqml2::AbstractRepresentation const *representation, vtkDataAssembly *assembly, int node_parent);

	void selectNodeIdParent(int node);
	void selectNodeIdChildren(int node);

	/**
	* delete _oldSelection mapper
	*/
	void deleteMapper(double p_time);
	/**
	* initialize _currentSelection mapper
	*/
	void initMapper(const TreeViewNodeType type, const int node_id, const int nbProcess, const int processId);
	/**
	* load vtkObject mapper
	*/
	void loadMapper(const TreeViewNodeType type, const int node_id, double time);
	/**
	* Attach vtkObject to _output
	*/
	void attachMapper();

	// This function replaces the VTK function vtkDataAssembly::MakeValidNodeName(),
	// which has a bug in the sorted_valid_chars array. The '.' character is placed
	// before the '-' character, which is incorrect. This function uses a valid_chars
	// array that correctly sorts the characters. The function checks if each character
	// in the input string is valid, and adds it to the _output string if it is valid.
	// If the first character of the _output string is not valid, an underscore is added
	// to the beginning of the _output string. This function is designed to create a valid
	// node name from a given string.
	std::string MakeValidNodeName(const char* name);

	bool markerOrientation;
	int markerSize;

	common::DataObjectRepository *repository;

	vtkSmartPointer<vtkPartitionedDataSetCollection> _output;

	std::map<int, CommonAbstractObjectToVtkPartitionedDataSet*> _nodeIdToMapper; // index of VtkDataAssembly to CommonAbstractObjectToVtkPartitionedDataSet
	std::map<int, CommonAbstractObjectSetToVtkPartitionedDataSetSet*> _nodeIdToMapperSet; // index of VtkDataAssembly to CommonAbstractObjectSetToVtkPartitionedDataSetSet

	//\/          uuid             title            index        prop_uuid
	std::map<std::string, std::map<std::string, std::map<double, std::string>>> _timeSeriesUuidAndTitleToIndexAndPropertiesUuid;

	std::set<int> _currentSelection;
	std::set<int> _oldSelection;

	// time step values
	std::vector<double> times_step;
#ifdef WITH_ETP_SSL
	std::shared_ptr<ETP_NS::AbstractSession> session;
#endif
};
#endif
