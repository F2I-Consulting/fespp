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

/** @brief	link the partial object with the object source.
 */

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
	 * load a property to object source
	 * loading a full representation is not possible, the source will be taken first
	 *
	 * @param 	uuid	property to load on the source
	 */
	void visualize(const std::string & uuid);
	
	/**
	* create link between uuid
	*
	* @param 	uuid	uuid source
	* @param	parent	uuid parent
	* @param	name	name/title of object uuid's
	* @param	resqmlType	type of object uuid's
	*/
	void createTreeVtk(const std::string & uuid, const std::string & parent, const std::string & name, VtkEpcCommon::Resqml2Type resqmlTypeParent);
	
	/**
	 * unload a property to object source
	 * unload a full representation is not possible, the source will be taken first
	 *
	 * @param 	uuid	property to unload at the source
	 */
	void remove(const std::string & uuid);

	/**
	* get type for a uuid
	*
	* @param 	uuid	
	*/
	VtkEpcCommon::Resqml2Type getType();
	
	/**
	* get all info (VtkEpcCommon) for a uuid
	*
	* @param 	uuid	
	*/
	VtkEpcCommon getInfoUuid();

	/**
	* return the vtkEpcDocument which contain source uuid
	*/
	const COMMON_NS::DataObjectRepository* getEpcSource() const;

	/**
	* get the element count for a unit (points/cells) on which property values attached 
	*
	* @param 	uuid	property or representation uuid
	* @param 	propertyUnit	unit	
	*/
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
