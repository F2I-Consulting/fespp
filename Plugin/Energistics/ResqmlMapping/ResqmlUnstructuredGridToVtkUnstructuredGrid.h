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
#ifndef __ResqmlUnstructuredGridToVtkUnstructuredGrid_h
#define __ResqmlUnstructuredGridToVtkUnstructuredGrid_h

#include "ResqmlAbstractRepresentationToVtkDataset.h"

// include VTK
#include <vtkSmartPointer.h>
#include <vtkUnstructuredGrid.h>

namespace RESQML2_NS
{
	class UnstructuredGridRepresentation;
}

class ResqmlUnstructuredGridToVtkUnstructuredGrid : public ResqmlAbstractRepresentationToVtkDataset
{
public:
	/**
	 * Constructor
	 */
	ResqmlUnstructuredGridToVtkUnstructuredGrid(RESQML2_NS::UnstructuredGridRepresentation *unstructuredGrid, int proc_number = 1, int max_proc = 1);

	/**
	 * load vtkDataSet with resqml data
	 */
	void loadVtkObject();

	/**
	 * Create the VTK points from the RESQML points of the RESQML IJK grid representation.
	 */
	vtkSmartPointer<vtkPoints> createPoints(/* RESQML2_NS::AbstractIjkGridRepresentation *ijkGrid */);
	/**
	 *	Return The vtkPoints
	 */
	vtkSmartPointer<vtkPoints> getVtkPoints();

protected:
	RESQML2_NS::UnstructuredGridRepresentation *resqmlData;

private:
	/**
	 * Insert a new VTK tetrahedron corresponding to a particular RESQML cell
	 *
	 * @param unstructuredGridRep				The RESQML UnstructuredGridRepresentation which contains the cell to map and to insert in the VTK UnstructuredGrid
	 * @param cumulativeFaceCountPerCell			The cumulative count of faces for each cell of the RESQML UnstructuredGridRepresentation.
	 * @param cellFaceNormalOutwardlyDirected	Indicates for each cell face of the RESQML UnstructuredGridRepresentation if its normal using the right hand rule is outwardly directed.
	 * @param cellIndex							The index of the RESQML cell in the RESQML UnstructuredGridRepresentation to be mapped and inserted in the VTK UnstructuredGrid.
	 */
	void cellVtkTetra(vtkSmartPointer<vtkUnstructuredGrid> vtk_unstructuredGrid,
					  const RESQML2_NS::UnstructuredGridRepresentation *unstructuredGridRep,
					  ULONG64 const *cumulativeFaceCountPerCell,
					  unsigned char const *cellFaceNormalOutwardlyDirected,
					  ULONG64 cellIndex);

	/**
	 * Insert a new VTK wedge or pyramid corresponding to a particular RESQML cell
	 *
	 * @param unstructuredGridRep				The RESQML UnstructuredGridRepresentation which contains the cell to map and to insert in the VTK UnstructuredGrid
	 * @param cumulativeFaceCountPerCell			The cumulative count of faces for each cell of the RESQML UnstructuredGridRepresentation.
	 * @param cellFaceNormalOutwardlyDirected	Indicates for each cell face of the RESQML UnstructuredGridRepresentation if its normal using the right hand rule is outwardly directed.
	 * @param cellIndex							The index of the RESQML cell in the RESQML UnstructuredGridRepresentation to be mapped and inserted in the VTK UnstructuredGrid.
	 */
	void cellVtkWedgeOrPyramid(vtkSmartPointer<vtkUnstructuredGrid> vtk_unstructuredGrid,
							   const RESQML2_NS::UnstructuredGridRepresentation *unstructuredGridRep,
							   ULONG64 const *cumulativeFaceCountPerCell,
							   unsigned char const *cellFaceNormalOutwardlyDirected,
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
	bool cellVtkHexahedron(vtkSmartPointer<vtkUnstructuredGrid> vtk_unstructuredGrid,
						   const RESQML2_NS::UnstructuredGridRepresentation *unstructuredGridRep,
						   ULONG64 const *cumulativeFaceCountPerCell,
						   unsigned char const *cellFaceNormalOutwardlyDirected,
						   ULONG64 cellIndex);

	// verify and add cell if is  VTK_PENTAGONAL_PRISM
	bool cellVtkPentagonalPrism(vtkSmartPointer<vtkUnstructuredGrid> vtk_unstructuredGrid,
								const RESQML2_NS::UnstructuredGridRepresentation *, ULONG64 cellIndex);
	// verify and add cell if is  VTK_HEXAGONAL_PRISM
	bool cellVtkHexagonalPrism(vtkSmartPointer<vtkUnstructuredGrid> vtk_unstructuredGrid,
							   const RESQML2_NS::UnstructuredGridRepresentation *, ULONG64 cellIndex);
};
#endif
