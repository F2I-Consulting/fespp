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

namespace common
{
	class DataObjectRepository;
}

namespace resqml2
{
	class AbstractRepresentation;
}

class ResqmlAbstractRepresentationToVtkPartitionedDataSet;

/**
 * @brief	class description.
 */
class ResqmlDataRepositoryToVtkPartitionedDataSetCollection
{
public:
	ResqmlDataRepositoryToVtkPartitionedDataSetCollection();
	~ResqmlDataRepositoryToVtkPartitionedDataSetCollection();
	// --------------- PART: TreeView ---------------------
	vtkDataAssembly *GetAssembly() { return output->GetDataAssembly(); }

	//---------------------------------

	// for EPC reader
	std::string addFile(const char *file);

	// for ETP source
	std::string addDataspace(const char *dataspace);

	// for ETP connection
	std::vector<std::string> connect(const std::string etp_url, const std::string data_partition, const std::string auth_connection);
	void disconnect();

	// Wellbore Options
	void setMarkerOrientation(bool orientation);
	void setMarkerSize(int size);

	vtkPartitionedDataSetCollection *getVtkPartitionedDatasSetCollection(const double time);

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

	void initMapper(const std::string &uuid);
	void loadMapper(const std::string &uuid, double time);

	bool markerOrientation;
	int markerSize;

	common::DataObjectRepository *repository;

	vtkSmartPointer<vtkPartitionedDataSetCollection> output;

	std::map<int, ResqmlAbstractRepresentationToVtkPartitionedDataSet *> nodeId_to_resqml; // index of VtkDataAssembly to ResqmlAbstractRepresentationToVtkPartitionedDataSet

	//\/          uuid             title            index        prop_uuid
	std::map<std::string, std::map<std::string, std::map<double, std::string>>> timeSeries_uuid_and_title_to_index_and_properties_uuid;

	std::set<int> current_selection;
	std::set<int> old_selection;

	// time step values
	std::vector<double> times_step;
#ifdef WITH_ETP_SSL
	std::shared_ptr<ETP_NS::AbstractSession> session;
#endif
};
#endif