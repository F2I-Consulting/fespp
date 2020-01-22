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
#ifndef __VtkSetPatch_h
#define __VtkSetPatch_h

// include system
#include <unordered_map>

#include <vtkDataArray.h>

// Fesapi namespaces
#include <fesapi/nsDefinitions.h>

#include "VtkResqml2MultiBlockDataSet.h"

class VtkPolylineRepresentation;
class VtkTriangulatedRepresentation;
class VtkProperty;

namespace COMMON_NS
{
	class DataObjectRepository;
}

class VtkSetPatch : public VtkResqml2MultiBlockDataSet
{

public:
	/**
	* Constructor
	*/
	VtkSetPatch (const std::string & fileName, const std::string & name, const std:: string & uuid, const std::string & uuidParent, COMMON_NS::DataObjectRepository *pckEPC, int idProc=0, int maxProc=0);
	
	/**
	* Destructor
	*/
	virtual ~VtkSetPatch();

	/**
	* method : createTreeVtk
	* variable : std::string uuid, std::string parent, std::string name, Resqml2Type resqml Type
	* prepare the vtk object for represent the set representation.
	*/
	void createTreeVtk(const std::string & uuid, const std::string & parent, const std::string & name, VtkEpcCommon::Resqml2Type resqmlType);

	/**
	* method : visualize
	* variable : std::string uuid 
	* create the vtk objects.
	*/
	void visualize(const std::string & uuid);

	/**
	* method : remove
	* variable : std::string uuid 
	* delete this object.
	*/
	void remove(const std::string & uuid);

	void addProperty(const std::string & uuidProperty, vtkDataArray* dataProperty);
	
	long getAttachmentPropertyCount(const std::string & uuid, VtkEpcCommon::FesppAttachmentProperty propertyUnit) ;
protected:
	
	/**
	* method : attach
	* variable : --
	* attach to this object the differents representations.
	*/
	void attach();
	
private:
	// EPC DOCUMENT
	COMMON_NS::DataObjectRepository *epcPackage;
	
	// All representation
	std::unordered_map<std::string, std::vector<VtkPolylineRepresentation*>> uuidToVtkPolylineRepresentation;
	std::unordered_map<std::string, std::vector<VtkTriangulatedRepresentation*>> uuidToVtkTriangulatedRepresentation;
	std::unordered_map<std::string, VtkProperty*> uuidToVtkProperty;
};
#endif
