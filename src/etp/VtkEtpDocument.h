﻿/*-----------------------------------------------------------------------
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
#ifndef __vtkEtpDocument_h
#define __vtkEtpDocument_h

// include system
#include <string>
#include <vector>
#include <list>
#include <unordered_map>

#include <vtkSmartPointer.h>

#include <fesapi/etp/ProtocolHandlers/DiscoveryHandlers.h>

// include VTK
#include "VTK/VtkResqml2MultiBlockDataSet.h"
#include "VTK/VtkEpcCommon.h"

class EtpClientSession;

class VtkIjkGridRepresentation;
class VtkUnstructuredGridRepresentation;
class VtkPartialRepresentation;
class VtkGrid2DRepresentation;
class VtkPolylineRepresentation;
class VtkTriangulatedRepresentation;
class VtkSetPatch;
class VtkWellboreTrajectoryRepresentation;

class VtkEtpDocument  : public VtkResqml2MultiBlockDataSet
{
public:

	/**
	 * Constructor
	 */
	VtkEtpDocument (const std::string & ipAddress, const std::string & port, const VtkEpcCommon::modeVtkEpc & mode);
	/**
	 * Destructor
	 */
	~VtkEtpDocument();

	/**
	 * method : remove
	 * variable : std::string uuid
	 * delete uuid representation.
	 */
	void remove(const std::string & uuid);

	void receive_resources_tree(const std::string & rec_uri, const std::string & rec_name, const std::string & dataobjectType, int32_t sourceCount);
	void receive_nbresources_tree(size_t);

	EtpClientSession* getClientSession();
	void setClientSession(EtpClientSession * session) {client_session = session;}

	void createTree();
	void attach();

	/**
	 * method : get TreeView
	 * variable :
	 */
	std::vector<VtkEpcCommon> getTreeView() const;

	/**
	* method : inloading treeView
	* variable :
	*/
	bool isLoading() const;

	/**
	 * method : visualize
	 * variable : std::string uuid
	 * create uuid representation.
	 */
	void visualize(const std::string & uuid);

	/**
	 * method : remove
	 * variable : std::string uuid
	 * delete uuid representation.
	 */
	void unvisualize(const std::string & uuid);

	/**
	 * method : getOutput
	 * variable : --
	 * return the vtkMultiBlockDataSet for each epcdocument.
	 */
	vtkSmartPointer<vtkMultiBlockDataSet> getVisualization() const;

	long getAttachmentPropertyCount(const std::string & uuid, const VtkEpcCommon::FesppAttachmentProperty propertyUnit) ;

private:

	int64_t push_command(const std::string & command);

	void addPropertyTreeVtk(const std::string & uuid, const std::string & parent, const std::string & name);

	void createTreeVtk(const std::string & uuid, const std::string & parent, const std::string & name, const VtkEpcCommon::Resqml2Type & resqmlType);

	std::list<int> number_response_wait_queue;
	std::list<VtkEpcCommon> response_queue;
	std::list<std::string> command_queue;

	EtpClientSession * client_session;

	bool treeViewMode;
	bool representationMode;

	std::vector<VtkEpcCommon> treeView; // Tree

	int64_t last_id;

	bool loading;

	std::unordered_map<std::string, VtkIjkGridRepresentation*> uuidToVtkIjkGridRepresentation;
};
#endif
