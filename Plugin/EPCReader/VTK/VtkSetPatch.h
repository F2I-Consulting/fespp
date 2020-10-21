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

/** @brief	The fespp patch representation for polylineSet/TriangulatedSet in VtkMultiBlockDataSet.
 */

class VtkSetPatch : public VtkResqml2MultiBlockDataSet
{

public:
	/**
	* create a polyline or triangulated representation for each patch.
	* The patch name = "Patch"+patch_index
	*/
	VtkSetPatch (const std::string & fileName, const std::string & name, const std:: string & uuid, const std::string & uuidParent, COMMON_NS::DataObjectRepository *pckEPC, int idProc=0, int maxProc=0);
	
	/**
	* Destructor
	*/
	virtual ~VtkSetPatch();

	/**
	* Add to trees structure a patch or a property.
	*
	* @param uuid		uuid property.
	* @param parent		uuid property parent.
	* @param name		name (i.e. title) property
	* @param resqmlType		verification of property type
	*
	*/
	void createTreeVtk(const std::string & uuid, const std::string & parent, const std::string & name, VtkEpcCommon::Resqml2Type resqmlType);

	/**
	 * load patch or property for patch in vtkPolyData
	 *
	 * @param 	uuid	uuid to load
	 */
	void visualize(const std::string & uuid);

	/**
	* Remove a property or detach of tree the set 
	*/
	void remove(const std::string & uuid);

	/**
	* apply data for a property
	*
	* @param 	uuidProperty	the property uuid
	* @param 	uuid	the data
	*/
	void addProperty(const std::string & uuidProperty, vtkDataArray* dataProperty);
	
	/**
	* get the element count for a unit (points/cells) on which property values attached 
	*
	* @param 	uuid	property or representation uuid
	* @param 	propertyUnit	unit	
	*/
	long getAttachmentPropertyCount(const std::string & uuid, VtkEpcCommon::FesppAttachmentProperty propertyUnit);

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
