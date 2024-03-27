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
	class RepresentationSetRepresentation;
	class AbstractObject;
	class PropertySet;
	class WellboreTrajectoryRepresentation;
	class WellboreFeature;
}

namespace resqml2_0_1
{
	class PropertySet;
}

class ResqmlAbstractRepresentationToVtkPartitionedDataSet;
class CommonAbstractObjectSetToVtkPartitionedDataSetSet;
class CommonAbstractObjectToVtkPartitionedDataSet;

/**
 * @brief	transform a fesapi data repository to VtkPartitionedDataSetCollection
 */
class ResqmlDataRepositoryToVtkPartitionedDataSetCollection
{
public:
	ResqmlDataRepositoryToVtkPartitionedDataSetCollection();
	~ResqmlDataRepositoryToVtkPartitionedDataSetCollection();
	// --------------- PART: TreeView ---------------------
	vtkDataAssembly* GetAssembly() { return _output->GetDataAssembly(); };

	//---------------------------------

	// for EPC reader
	std::string addFile(const char *p_file);
	// for EPC reader
	void closeHdfProxies();

	// for ETP source
	std::string addDataspace(const char *p_dataspace);

	// for ETP connection
	std::vector<std::string> connect(const std::string &p_etpUrl, const std::string &p_dataPartition, const std::string &p_authConnection);
	void disconnect();

	// Wellbore Options
	void setMarkerOrientation(bool p_orientation);
	void setMarkerSize(uint32_t p_size);

	vtkPartitionedDataSetCollection *getVtkPartitionedDatasSetCollection(const double p_time, const uint32_t p_nbProcess = 1, const uint32_t p_processId = 0);

	std::vector<double> getTimes() { return _timesStep; };

	/**
	 * @return selection parent
	 */
	std::string selectNodeId(int p_nodeId);
	void clearSelection();

private:
	std::string buildDataAssemblyFromDataObjectRepo(const char *p_fileName);

	std::string searchWellboreTrajectory(const std::string &p_fileName);												  // traj
	std::string searchWellboreFrame(const resqml2::WellboreTrajectoryRepresentation *w_wellboreTrajectory, int p_nodeId); // frame/markerFrame + chanel + marker
	std::string searchWellboreCompletion(const resqml2::WellboreFeature *w_wellboreTrajectory, int p_nodeId);			  // completion + perforation
	std::string searchRepresentations(resqml2::AbstractRepresentation const *p_representation, int p_nodeId = 0 /* 0 is root's id*/);
	int searchRepresentationSetRepresentation(resqml2::RepresentationSetRepresentation const *p_rsr, int p_nodeId = 0 /* 0 is root's id*/);
	std::string searchSubRepresentation(resqml2::AbstractRepresentation const *p_representation, int p_nodeParent);
	std::string searchTimeSeries(const std::string &p_fileName);
	int searchPropertySet(resqml2_0_1::PropertySet const *p_propSet, int p_nodeId);
	std::string searchProperties(resqml2::AbstractRepresentation const *p_representation, int p_nodeParent);

	void selectNodeIdParent(int p_nodeId);
	void selectNodeIdChildren(int p_nodeId);

	/**
	 * delete _oldSelection mapper
	 */
	void deleteMapper(double p_time);
	/**
	 * initialize _nodeIdToMapperSet
	 */
	void initMapperSet(const TreeViewNodeType p_type, const int p_nodeId, const uint32_t p_nbProcess, const uint32_t p_processId);
	/**
	 * initialize and load _nodeIdToMapper
	 */
	void loadMapper(const TreeViewNodeType p_type, const int p_nodeId,const uint32_t p_nbProcess, const uint32_t p_processId);
	void loadRepresentationMapper(const int p_nodeId, const uint32_t p_nbProcess, const uint32_t p_processId);
	void loadWellboreTrajectoryMapper(const int p_nodeId);
	/**
 * add data to parent nodeId
 */
	void addDataToParent(const TreeViewNodeType p_type, const int p_nodeId, const uint32_t p_nbProcess, const uint32_t p_processId, const double p_time);

	// This function replaces the VTK function vtkDataAssembly::MakeValidNodeName(),
	// which has a bug in the sorted_valid_chars array. The '.' character is placed
	// before the '-' character, which is incorrect. This function uses a valid_chars
	// array that correctly sorts the characters. The function checks if each character
	// in the input string is valid, and adds it to the _output string if it is valid.
	// If the first character of the _output string is not valid, an underscore is added
	// to the beginning of the _output string. This function is designed to create a valid
	// node name from a given string.
	std::string MakeValidNodeName(const char *p_name);

	bool _markerOrientation;
	uint32_t _markerSize;

	common::DataObjectRepository *_repository;

	vtkSmartPointer<vtkPartitionedDataSetCollection> _output;

	std::map<int, CommonAbstractObjectToVtkPartitionedDataSet *> _nodeIdToMapper;		   // index of VtkDataAssembly to CommonAbstractObjectToVtkPartitionedDataSet
	std::map<int, CommonAbstractObjectSetToVtkPartitionedDataSetSet *> _nodeIdToMapperSet; // index of VtkDataAssembly to CommonAbstractObjectSetToVtkPartitionedDataSetSet

	//\/          uuid             title            index        prop_uuid
	std::map<std::string, std::map<std::string, std::map<double, std::string>>> _timeSeriesUuidAndTitleToIndexAndPropertiesUuid;

	std::set<int> _currentSelection;
	std::set<int> _oldSelection;

	std::set<std::string> _files;

	// time step values
	std::vector<double> _timesStep;
#ifdef WITH_ETP_SSL
	std::shared_ptr<ETP_NS::ClientSession> _session;
#endif
};
#endif
