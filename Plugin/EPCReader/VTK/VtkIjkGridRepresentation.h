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
#ifndef __VtkIjkGridRepresentation_h
#define __VtkIjkGridRepresentation_h

#include "VtkResqml2UnstructuredGrid.h"

namespace COMMON_NS
{
	class AbstractObject;
}

namespace RESQML2_NS
{
	class AbstractIjkGridRepresentation;
}

/** @brief	The ijkGrid representation.
 */

class VtkIjkGridRepresentation : public VtkResqml2UnstructuredGrid
{
public:
	/**
	* Constructor
	*/
	VtkIjkGridRepresentation(const std::string & fileName, const std::string & name, const std::string & uuid, const std::string & uuidParent, COMMON_NS::DataObjectRepository const * pckRep, COMMON_NS::DataObjectRepository const * pckSubRep, int idProc=0, int maxProc=0);

	/**
	* Destructor
	*/
	~VtkIjkGridRepresentation() {}
	
	/**
	 * load ijkgrid or property in VtkUnstructuredGrid
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
	long getAttachmentPropertyCount(const std::string & uuid, VtkEpcCommon::FesppAttachmentProperty propertyUnit) ;

	/**
	* get the ICell count for a representation or property uuid
	*
	* @param 	uuid	property or representation uuid
	*/
	int getICellCount(const std::string & uuid) const;

	/**
	* get the ICell count for ijkGrid
	*
	* @param 	uuid	property or representation uuid
	*/
	int getJCellCount(const std::string & uuid) const;

	/**
	* get the ICell count for ijkGrid
	*
	* @param 	uuid	property or representation uuid
	*/
	int getKCellCount(const std::string & uuid) const;

	/**
	* get the ICell count for ijkGrid
	*
	* @param 	uuid	property or representation uuid
	*/
	int getInitKIndex(const std::string & uuid) const;

	/**
	* set the ICell count for ijkGrid
	*
	* @param 	value	ICell count
	*/
	void setICellCount(int value) {	iCellCount = value; }

	/**
	* set the JCell count for ijkGrid
	*
		* @param 	value	JCell count
	*/
	void setJCellCount(int value) { jCellCount = value; }

	/**
	* set the KCell count for ijkGrid
	*
	* @param 	value	KCell count
	*/
	void setKCellCount(int value) { kCellCount = value; }

	/**
	* for multiprocess set the init Klayer index
	*
	* @param 	value	init klayer index
	*/
	void setInitKIndex(int value) { initKIndex = value; }

	/**
	* for multiprocess set the max Klayer index
	*
	* @param 	value	max klayer index
	*/
	void setMaxKIndex(int value) { maxKIndex = value; }

private:

	/**
	* Create the VTK points from the RESQML points of the RESQML IJK grid representation.
	*/
	vtkSmartPointer<vtkPoints> createPoints();

	/**
	* Create the VTK unstructured grid.
	*
	* @param ijkOrSubrep			The corresponding RESQML2 representation which can be eith an ijk grid or a subrepresenation
	*/
	void createVtkUnstructuredGrid(RESQML2_NS::AbstractRepresentation* ijkOrSubrep);

	/**
	* method : checkHyperslabingCapacity
	* variable : ijkGridRepresentation
	* check if an ijkgrid is Hyperslabed
	*/	
	void checkHyperslabingCapacity(RESQML2_NS::AbstractIjkGridRepresentation* ijkGridRepresentation);

	std::string lastProperty;

	int iCellCount;
	int jCellCount;
	int kCellCount;
	int initKIndex;
	int maxKIndex;

	bool isHyperslabed;

	vtkSmartPointer<vtkPoints> points;
};
#endif
