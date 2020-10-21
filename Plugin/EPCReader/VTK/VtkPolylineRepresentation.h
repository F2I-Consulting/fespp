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
#ifndef __VtkPolylineRepresentation_h
#define __VtkPolylineRepresentation_h

#include "VtkResqml2PolyData.h"

/** @brief	The fespp polyline representation (set or simple)
 */

class VtkPolylineRepresentation : public VtkResqml2PolyData 
{
public:
	/**
	* Constructor
	*/
	VtkPolylineRepresentation(const std::string & fileName, const std::string & name, const std::string & uuid, const std::string & uuidParent, unsigned int patchNo, COMMON_NS::DataObjectRepository *repoRepresentation, COMMON_NS::DataObjectRepository *repoSubRepresentation);
	
	/**
	* Destructor
	*/
	~VtkPolylineRepresentation() = default;

	/**
	 * load polyline  or property in vtkPolyData
	 *
	 * @param 	uuid	uuid to load
	 */
	void visualize(const std::string & uuid);

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

private:
	unsigned int patchIndex;
	
	std::string lastProperty;
};
#endif
