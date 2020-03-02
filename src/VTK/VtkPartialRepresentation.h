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
#ifndef __VtkPartialRepresentation_h
#define __VtkPartialRepresentation_h

// SYSTEM
#include <unordered_map>
#include <string>

#include <fesapi/nsDefinitions.h>

// FESPP
#include "VtkAbstractObject.h"

class VtkEpcDocument;
class VtkProperty;

namespace COMMON_NS
{
	class DataObjectRepository;
}

class VtkPartialRepresentation
{
public:
	/**
	* Constructor
	*/
	VtkPartialRepresentation(const std::string & fileName, const std::string & uuid, VtkEpcDocument* vtkEpcDowumentWithCompleteRep, COMMON_NS::DataObjectRepository* repo);

	/**
	* Destructor
	*/
	~VtkPartialRepresentation();

	/**
	* method : visualize
	* variable : std::string uuid 
	*/
	void visualize(const std::string & uuid);
	
	/**
	* method : createTreeVtk
	* variable : std::string uuid, std::string parent, std::string name, Resqml2Type resqmlTypeParent
	*/
	void createTreeVtk(const std::string & uuid, const std::string & parent, const std::string & name, VtkEpcCommon::Resqml2Type resqmlTypeParent);
	
	/**
	* method : remove
	* variable : std::string uuid
	* delete the vtkDataArray.
	*/
	void remove(const std::string & uuid);

	VtkEpcCommon::Resqml2Type getType();
	VtkEpcCommon getInfoUuid();

	COMMON_NS::DataObjectRepository * getEpcSource();

	long getAttachmentPropertyCount(const std::string & uuid, VtkEpcCommon::FesppAttachmentProperty propertyUnit);

private:
	std::unordered_map<std::string, VtkProperty*> uuidToVtkProperty;

	// EPC DOCUMENT
	COMMON_NS::DataObjectRepository* repository;

	VtkEpcDocument* vtkEpcDocumentSource;
	std::string vtkPartialReprUuid;
	std::string fileName;
};
#endif
