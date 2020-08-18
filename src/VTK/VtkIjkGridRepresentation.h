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
	* method : visualize
	* variable : uuid (ijk Grid representation or property) 
	* description : 
	*    - if ijk uuid : create the vtk objects for represent ijk Grid 
	*    - if property uuid : add property to ijk Grid 
	*/
	void visualize(const std::string & uuid);
	
	/**
	* method : addProperty
	* variable : uuid,  dataProperty
	* description :
	* add property to ijk Grid
	*/
	void addProperty(const std::string & uuidProperty, vtkDataArray* dataProperty);

	/**
	* method : getAttachmentPropertyCount
	* variable : uuid,  support property (CELLS or POINTS)
	* return : count 
	*/
	long getAttachmentPropertyCount(const std::string & uuid, VtkEpcCommon::FesppAttachmentProperty propertyUnit) ;

	int getICellCount(const std::string & uuid) const;

	int getJCellCount(const std::string & uuid) const;

	int getKCellCount(const std::string & uuid) const;

	int getInitKIndex(const std::string & uuid) const;

	void setICellCount(int value) {	iCellCount = value; }

	void setJCellCount(int value) { jCellCount = value; }

	void setKCellCount(int value) { kCellCount = value; }

	void setInitKIndex(int value) { initKIndex = value; }

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

};
#endif
