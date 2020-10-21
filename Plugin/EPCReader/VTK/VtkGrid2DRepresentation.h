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
#ifndef __VtkGrid2DRepresentation_h
#define __VtkGrid2DRepresentation_h

#include <vtkDataArray.h>

#include "VtkResqml2MultiBlockDataSet.h"
#include "VtkGrid2DRepresentationPoints.h"

/** @brief	The grid2D representation.
 */

class VtkGrid2DRepresentation : public VtkResqml2MultiBlockDataSet
{
public:
	/**
	* Constructor
	*/
	VtkGrid2DRepresentation(const std::string & fileName, const std::string & name, const std::string & uuid, const std::string & uuidParent, COMMON_NS::DataObjectRepository const * pckRep, COMMON_NS::DataObjectRepository const * pckSubRep);
	
	/**
	* Destructor
	*/
	~VtkGrid2DRepresentation();

	/**
	* create link between uuid
	*
	* @param 	uuid	uuid source
	* @param	parent	uuid parent
	* @param	name	name/title of object uuid's
	* @param	resqmlType	type of object uuid's
	*/
	void createTreeVtk(const std::string & uuid, const std::string & parent, const std::string & name, VtkEpcCommon::Resqml2Type resqmlType);

	/**
	* add all Vtk representation to the epc VtkMultiblockDataset
	*/
	void attach();
	
	/**
	 * load grid2D or property in VtkMultiBlockDataSet 
	 *
	 * @param 	uuid	uuid to load
	 */
	void visualize(const std::string & uuid);
	
	/**
	 * unload grid2D or property in VtkMultiBlockDataSet 
	 *
	 * @param 	uuid	uuid to unload
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
	* method : createOutput
	* variable : std::string uuid 
	* create grid2D points and grid2D cells.
	*/
	int createOutput(const std::string & uuid);
	
private:
	// EPC DOCUMENT
	COMMON_NS::DataObjectRepository const * repositoryRepresentation;
	COMMON_NS::DataObjectRepository const * repositorySubRepresentation;

	VtkGrid2DRepresentationPoints grid2DPoints;
};
#endif
