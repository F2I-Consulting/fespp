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
#ifndef __VtkEpcDocumentSet_h
#define __VtkEpcDocumentSet_h

#include "VtkEpcCommon.h"

// include system
#include <unordered_map>
#include <vector>

// include VTK
#include <vtkSmartPointer.h>
#include <vtkMultiBlockDataSet.h>

class VtkEpcDocument;

class VtkEpcDocumentSet
{
public:
	/**
	* Constructor
	*/
	VtkEpcDocumentSet (int idProc=0, int maxProc=0, VtkEpcCommon::modeVtkEpc mode = VtkEpcCommon::modeVtkEpc::Both);

	/**
	* Destructor
	*/
	~VtkEpcDocumentSet();

	/**
	* method : visualize
	* variable : std::string uuid 
	* create uuid representation.
	*/
	std::string visualize(const std::string & uuid);
	void visualizeFull();
	void visualizeFullWell(std::string fileName);
	
	/**
	* method : remove
	* variable : std::string uuid 
	* delete uuid representation.
	*/
	void unvisualize(const std::string & uuid);
	void unvisualizeFullWell(std::string fileName);

	VtkEpcCommon::Resqml2Type getType(std::string uuid);
	VtkEpcCommon getInfoUuid(std::string);

	void toggleMarkerOrientation(const bool orientation);

	/**
	* method : getOutput
	* variable : --
	* return the vtkMultiBlockDataSet for each epcdocument.
	*/
	vtkSmartPointer<vtkMultiBlockDataSet> getVisualization() const;
	std::vector<VtkEpcCommon const *> getAllVtkEpcCommons() const;

	std::string addEpcDocument(const std::string & fileName);

	const std::vector<VtkEpcDocument*>& getAllVtkEpcDocuments() const { return vtkEpcList; }
	VtkEpcDocument* getVtkEpcDocument(const std::string& uuid);

	/**
	* Get the datatype of a particular dataobject according to its uuid
	*
	* @param uuid	The UUID of the dataobject we want to know it datatype
	* @return	An unknown datatype is returned if the dataobject is not contained on this set
	*/
	VtkEpcCommon::Resqml2Type getTypeInEpcDocument(const std::string & uuid);

private:

	// The list of EPC documents contained into this set
	std::vector<VtkEpcDocument*> vtkEpcList;
	// A list of uuid which cannot be visualized for some reasons (generally FESAPI exception)
	std::vector<std::string> badUuid;
	// link from a dataobject uuid to the VtkEpcDocument it belongs to
	std::unordered_map<std::string, VtkEpcDocument*> uuidToVtkEpc;

	vtkSmartPointer<vtkMultiBlockDataSet> vtkOutput;

	int procRank;
	int nbProc;

	bool treeViewMode;
	bool representationMode;

};
#endif
