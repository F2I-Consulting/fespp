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
#ifndef __VtkUnstructuredGridRepresentation_h
#define __VtkUnstructuredGridRepresentation_h

#include "VtkResqml2UnstructuredGrid.h"

class VtkProperty;

namespace COMMON_NS
{
	class DataObjectRepository;
}

namespace RESQML2_NS
{
	class UnstructuredGridRepresentation;
}

class VtkUnstructuredGridRepresentation : public VtkResqml2UnstructuredGrid
{
public:
	/**
	* Constructor
	*/
	VtkUnstructuredGridRepresentation(const std::string & fileName, const std::string & name, const std::string & uuid, const std::string & uuidParent,
		COMMON_NS::DataObjectRepository const * repoRepresentation, COMMON_NS::DataObjectRepository const * repoSubRepresentation, int idProc=0, int maxProc=0);

	/**
	* Destructor
	*/
	~VtkUnstructuredGridRepresentation() = default;

	/**
	* method : visualize
	* variable : std::string uuid (Unstructured Grid UUID)
	* create the vtk objects for represent Unstructured grid.
	*/
	void visualize(const std::string & uuid) final;

	void addProperty(const std::string & uuidProperty, vtkDataArray* dataProperty);

	long getAttachmentPropertyCount(const std::string & uuid, VtkEpcCommon::FesppAttachmentProperty propertyUnit) final;

private:
	/**
	* Insert a new VTK tetrahedron corresponding to a particular RESQML cell
	*
	* @param unstructuredGridRep				The RESQML UnstructuredGridRepresentation which contains the cell to map and to insert in the VTK UnstructuredGrid
	* @param cumulativeFaceCountPerCell			The cumulative count of faces for each cell of the RESQML UnstructuredGridRepresentation.
	* @param cellFaceNormalOutwardlyDirected	Indicates for each cell face of the RESQML UnstructuredGridRepresentation if its normal using the right hand rule is outwardly directed.
	* @param cellIndex							The index of the RESQML cell in the RESQML UnstructuredGridRepresentation to be mapped and inserted in the VTK UnstructuredGrid.
	*/
	void cellVtkTetra(const RESQML2_NS::UnstructuredGridRepresentation* unstructuredGridRep,
		ULONG64 const * cumulativeFaceCountPerCell,
		unsigned char const * cellFaceNormalOutwardlyDirected,
		ULONG64 cellIndex);

	/**
	* Insert a new VTK wedge or pyramid corresponding to a particular RESQML cell
	*
	* @param unstructuredGridRep				The RESQML UnstructuredGridRepresentation which contains the cell to map and to insert in the VTK UnstructuredGrid
	* @param cumulativeFaceCountPerCell			The cumulative count of faces for each cell of the RESQML UnstructuredGridRepresentation.
	* @param cellFaceNormalOutwardlyDirected	Indicates for each cell face of the RESQML UnstructuredGridRepresentation if its normal using the right hand rule is outwardly directed.
	* @param cellIndex							The index of the RESQML cell in the RESQML UnstructuredGridRepresentation to be mapped and inserted in the VTK UnstructuredGrid.
	*/
	void cellVtkWedgeOrPyramid(const RESQML2_NS::UnstructuredGridRepresentation* unstructuredGridRep,
		ULONG64 const * cumulativeFaceCountPerCell,
		unsigned char const * cellFaceNormalOutwardlyDirected,
		ULONG64 cellIndex);

	/**
	* Insert a new VTK hexahedron corresponding to a particular RESQML cell only if the RESQML cell is Quadrilaterally-faced hexahedron.
	*
	* @param unstructuredGridRep				The RESQML UnstructuredGridRepresentation which contains the cell to map and to insert in the VTK UnstructuredGrid
	* @param cumulativeFaceCountPerCell			The cumulative count of faces for each cell of the RESQML UnstructuredGridRepresentation.
	* @param cellFaceNormalOutwardlyDirected	Indicates for each cell face of the RESQML UnstructuredGridRepresentation if its normal using the right hand rule is outwardly directed.
	* @param cellIndex							The index of the RESQML cell in the RESQML UnstructuredGridRepresentation to be mapped and inserted in the VTK UnstructuredGrid.
	*
	* @return true if the hexahedron is a Quadrilaterally-faced one, false otherwise
	*/
	bool cellVtkHexahedron(const RESQML2_NS::UnstructuredGridRepresentation* unstructuredGridRep,
		ULONG64 const * cumulativeFaceCountPerCell,
		unsigned char const * cellFaceNormalOutwardlyDirected,
		ULONG64 cellIndex);

	// verify and add cell if is  VTK_PENTAGONAL_PRISM
	bool cellVtkPentagonalPrism(const RESQML2_NS::UnstructuredGridRepresentation*, ULONG64 cellIndex);
	// verify and add cell if is  VTK_HEXAGONAL_PRISM
	bool cellVtkHexagonalPrism(const RESQML2_NS::UnstructuredGridRepresentation*, ULONG64 cellIndex);
};
#endif
