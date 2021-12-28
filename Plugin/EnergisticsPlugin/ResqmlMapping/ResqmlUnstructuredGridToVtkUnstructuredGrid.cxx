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
#include "ResqmlMapping/ResqmlUnstructuredGridToVtkUnstructuredGrid.h"

// SYSTEM
#include <sstream>

// VTK
#include <vtkSmartPointer.h>
#include <vtkCellData.h>
#include <vtkPointData.h>
#include <vtkDataArray.h>
#include <vtkDoubleArray.h>
#include <vtkCellArray.h>
#include <vtkPolyhedron.h>
#include <vtkTetra.h>
#include <vtkHexagonalPrism.h>
#include <vtkPentagonalPrism.h>
#include <vtkHexahedron.h>
#include <vtkPyramid.h>
#include <vtkWedge.h>
#include <vtkUnstructuredGrid.h>

// FESAPI
#include <fesapi/resqml2/UnstructuredGridRepresentation.h>
//#include <fesapi/resqml2/SubRepresentation.h>
#include <fesapi/resqml2/AbstractLocal3dCrs.h>

// FESPP
#include "ResqmlMapping/ResqmlAbstractRepresentationToVtkDataset.h"

//----------------------------------------------------------------------------
ResqmlUnstructuredGridToVtkUnstructuredGrid::ResqmlUnstructuredGridToVtkUnstructuredGrid(RESQML2_NS::UnstructuredGridRepresentation *unstructuredGrid, int proc_number, int max_proc)
	: ResqmlAbstractRepresentationToVtkDataset(unstructuredGrid,
											   proc_number - 1,
											   max_proc - 1),
	  resqmlData(unstructuredGrid)
{
	this->pointCount = unstructuredGrid->getXyzPointCountOfAllPatches();

	this->vtkData = vtkSmartPointer<vtkPartitionedDataSet>::New();

	this->loadVtkObject();

	this->vtkData->Modified();
}

//----------------------------------------------------------------------------
void ResqmlUnstructuredGridToVtkUnstructuredGrid::loadVtkObject()
{
	vtkSmartPointer<vtkUnstructuredGrid> vtk_unstructuredGrid = vtkSmartPointer<vtkUnstructuredGrid>::New();
	// vtk_unstructuredGrid->Allocate(unstructuredGridRep->getCellCount());

	// POINTS
	double *allXyzPoints = new double[this->pointCount * 3]; // Will be deleted by VTK;
	this->resqmlData->getXyzPointsOfAllPatchesInGlobalCrs(allXyzPoints);

	vtkSmartPointer<vtkPoints> vtkPts = vtkSmartPointer<vtkPoints>::New();
	const size_t coordCount = this->pointCount * 3;
	if (this->resqmlData->getLocalCrs(0)->isDepthOriented())
	{
		for (size_t zCoordIndex = 2; zCoordIndex < coordCount; zCoordIndex += 3)
		{
			allXyzPoints[zCoordIndex] *= -1;
		}
	}

	vtkSmartPointer<vtkDoubleArray> vtkUnderlyingArray = vtkSmartPointer<vtkDoubleArray>::New();
	vtkUnderlyingArray->SetNumberOfComponents(3);
	// Take ownership of the underlying C array
	vtkUnderlyingArray->SetArray(allXyzPoints, coordCount, vtkAbstractArray::VTK_DATA_ARRAY_DELETE);
	vtkPts->SetData(vtkUnderlyingArray);

	vtk_unstructuredGrid->SetPoints(vtkPts);

	this->resqmlData->loadGeometry();
	// CELLS
	const unsigned int nodeCount = this->resqmlData->getNodeCount();
	std::unique_ptr<vtkIdType[]> pointIds(new vtkIdType[nodeCount]);
	for (unsigned int i = 0; i < nodeCount; ++i)
	{
		pointIds[i] = i;
	}

	const ULONG64 cellCount = this->resqmlData->getCellCount();
	ULONG64 const *cumulativeFaceCountPerCell = this->resqmlData->isFaceCountOfCellsConstant()
													? nullptr
													: this->resqmlData->getCumulativeFaceCountPerCell(); // This pointer is owned and managed by FESAPI
	std::unique_ptr<unsigned char[]> cellFaceNormalOutwardlyDirected(new unsigned char[cumulativeFaceCountPerCell == nullptr
																						   ? cellCount * this->resqmlData->getConstantFaceCountOfCells()
																						   : cumulativeFaceCountPerCell[cellCount - 1]]);
	this->resqmlData->getCellFaceIsRightHanded(cellFaceNormalOutwardlyDirected.get());

	auto maxCellIndex = (this->procNumber + 1) * cellCount / this->maxProc;

	for (ULONG64 cellIndex = this->procNumber * cellCount / this->maxProc; cellIndex < maxCellIndex; ++cellIndex)
	{
		bool isOptimizedCell = false;

		const ULONG64 localFaceCount = this->resqmlData->getFaceCountOfCell(cellIndex);
		if (localFaceCount == 4)
		{ // VTK_TETRA
			cellVtkTetra(vtk_unstructuredGrid, this->resqmlData, cumulativeFaceCountPerCell, cellFaceNormalOutwardlyDirected.get(), cellIndex);
			isOptimizedCell = true;
		}
		else if (localFaceCount == 5)
		{ // VTK_WEDGE or VTK_PYRAMID
			cellVtkWedgeOrPyramid(vtk_unstructuredGrid, this->resqmlData, cumulativeFaceCountPerCell, cellFaceNormalOutwardlyDirected.get(), cellIndex);
			isOptimizedCell = true;
		}
		else if (localFaceCount == 6)
		{ // VTK_HEXAHEDRON
			isOptimizedCell = cellVtkHexahedron(vtk_unstructuredGrid, this->resqmlData, cumulativeFaceCountPerCell, cellFaceNormalOutwardlyDirected.get(), cellIndex);
		}
		else if (localFaceCount == 7)
		{ // VTK_PENTAGONAL_PRISM
			isOptimizedCell = cellVtkPentagonalPrism(vtk_unstructuredGrid, this->resqmlData, cellIndex);
		}
		else if (localFaceCount == 8)
		{ // VTK_HEXAGONAL_PRISM
			isOptimizedCell = cellVtkHexagonalPrism(vtk_unstructuredGrid, this->resqmlData, cellIndex);
		}

		if (!isOptimizedCell)
		{
			vtkSmartPointer<vtkIdList> idList = vtkSmartPointer<vtkIdList>::New();

			// For polyhedron cell, a special ptIds input format is required : (numCellFaces, numFace0Pts, id1, id2, id3, numFace1Pts, id1, id2, id3, ...)
			idList->InsertNextId(localFaceCount);
			for (ULONG64 localFaceIndex = 0; localFaceIndex < localFaceCount; ++localFaceIndex)
			{
				const unsigned int localNodeCount = this->resqmlData->getNodeCountOfFaceOfCell(cellIndex, localFaceIndex);
				idList->InsertNextId(localNodeCount);
				ULONG64 const *nodeIndices = this->resqmlData->getNodeIndicesOfFaceOfCell(cellIndex, localFaceIndex);
				for (unsigned int i = 0; i < localNodeCount; ++i)
				{
					idList->InsertNextId(nodeIndices[i]);
				}
			}

			vtk_unstructuredGrid->InsertNextCell(VTK_POLYHEDRON, idList);
		}
	}
	this->resqmlData->unloadGeometry();

	this->vtkData->SetPartition(0, vtk_unstructuredGrid);
	this->vtkData->Modified();
}

//----------------------------------------------------------------------------
void ResqmlUnstructuredGridToVtkUnstructuredGrid::cellVtkTetra(vtkSmartPointer<vtkUnstructuredGrid> vtk_unstructuredGrid,
															   const RESQML2_NS::UnstructuredGridRepresentation *unstructuredGridRep,
															   ULONG64 const *cumulativeFaceCountPerCell, unsigned char const *cellFaceNormalOutwardlyDirected,
															   ULONG64 cellIndex)
{
	unsigned int nodes[4];

	// Face 0
	ULONG64 const *nodeIndices = unstructuredGridRep->getNodeIndicesOfFaceOfCell(cellIndex, 0);
	size_t cellFaceIndex = unstructuredGridRep->isFaceCountOfCellsConstant() || cellIndex == 0
							   ? cellIndex * 4
							   : cumulativeFaceCountPerCell[cellIndex - 1];
	if (cellFaceNormalOutwardlyDirected[cellFaceIndex] == 0)
	{ // The RESQML orientation of face 0 honors the VTK orientation of face 0 i.e. the face 0 normal defined using a right hand rule is inwardly directed.
		nodes[0] = nodeIndices[0];
		nodes[1] = nodeIndices[1];
		nodes[2] = nodeIndices[2];
	}
	else
	{ // The RESQML orientation of face 0 does not honor the VTK orientation of face 0
		nodes[0] = nodeIndices[2];
		nodes[1] = nodeIndices[1];
		nodes[2] = nodeIndices[0];
	}

	// Face 1
	nodeIndices = unstructuredGridRep->getNodeIndicesOfFaceOfCell(cellIndex, 1);

	for (size_t index = 0; index < 3; ++index)
	{
		if (std::find(nodes, nodes + 3, nodeIndices[index]) == nodes + 3)
		{
			nodes[3] = nodeIndices[index];
			break;
		}
	}

	vtkSmartPointer<vtkTetra> tetra = vtkSmartPointer<vtkTetra>::New();
	for (vtkIdType pointId = 0; pointId < 4; ++pointId)
	{
		tetra->GetPointIds()->SetId(pointId, nodes[pointId]);
	}
	vtk_unstructuredGrid->InsertNextCell(tetra->GetCellType(), tetra->GetPointIds());
}

//----------------------------------------------------------------------------
void ResqmlUnstructuredGridToVtkUnstructuredGrid::cellVtkWedgeOrPyramid(vtkSmartPointer<vtkUnstructuredGrid> vtk_unstructuredGrid,
																		const RESQML2_NS::UnstructuredGridRepresentation *unstructuredGridRep,
																		ULONG64 const *cumulativeFaceCountPerCell, unsigned char const *cellFaceNormalOutwardlyDirected,
																		ULONG64 cellIndex)
{
	std::vector<unsigned int> localFaceIndexWith4Nodes;
	for (unsigned int localFaceIndex = 0; localFaceIndex < 5; ++localFaceIndex)
	{
		if (unstructuredGridRep->getNodeCountOfFaceOfCell(cellIndex, localFaceIndex) == 4)
		{
			localFaceIndexWith4Nodes.push_back(localFaceIndex);
		}
	}
	if (localFaceIndexWith4Nodes.size() == 3)
	{ // VTK_WEDGE
		ULONG64 nodes[6];
		unsigned int faceTo3Nodes = 0;
		for (unsigned int localFaceIndex = 0; localFaceIndex < 5; ++localFaceIndex)
		{
			const unsigned int localNodeCount = unstructuredGridRep->getNodeCountOfFaceOfCell(cellIndex, localFaceIndex);
			if (localNodeCount == 3)
			{
				ULONG64 const *nodeIndices = unstructuredGridRep->getNodeIndicesOfFaceOfCell(cellIndex, localFaceIndex);
				for (unsigned int i = 0; i < localNodeCount; ++i)
				{
					nodes[faceTo3Nodes * 3 + i] = nodeIndices[i];
				}
				++faceTo3Nodes;
			}
		}

		vtkSmartPointer<vtkWedge> wedge = vtkSmartPointer<vtkWedge>::New();
		for (int nodesIndex = 0; nodesIndex < 6; ++nodesIndex)
		{
			wedge->GetPointIds()->SetId(nodesIndex, nodes[nodesIndex]);
		}
		vtk_unstructuredGrid->InsertNextCell(wedge->GetCellType(), wedge->GetPointIds());
	}
	else if (localFaceIndexWith4Nodes.size() == 1)
	{ // VTK_PYRAMID
		ULONG64 nodes[5];

		ULONG64 const *nodeIndices = unstructuredGridRep->getNodeIndicesOfFaceOfCell(cellIndex, localFaceIndexWith4Nodes[0]);
		size_t cellFaceIndex = (unstructuredGridRep->isFaceCountOfCellsConstant() || cellIndex == 0
									? cellIndex * 5
									: cumulativeFaceCountPerCell[cellIndex - 1]) +
							   localFaceIndexWith4Nodes[0];
		if (cellFaceNormalOutwardlyDirected[cellFaceIndex] == 0)
		{ // The RESQML orientation of the face honors the VTK orientation of face 0 i.e. the face 0 normal defined using a right hand rule is inwardly directed.
			nodes[0] = nodeIndices[0];
			nodes[1] = nodeIndices[1];
			nodes[2] = nodeIndices[2];
			nodes[3] = nodeIndices[3];
		}
		else
		{ // The RESQML orientation of the face does not honor the VTK orientation of face 0
			nodes[0] = nodeIndices[3];
			nodes[1] = nodeIndices[2];
			nodes[2] = nodeIndices[1];
			nodes[3] = nodeIndices[0];
		}

		// Face with 3 points
		nodeIndices = unstructuredGridRep->getNodeIndicesOfFaceOfCell(cellIndex, localFaceIndexWith4Nodes[0] == 0 ? 1 : 0);

		for (size_t index = 0; index < 3; ++index)
		{
			if (std::find(nodes, nodes + 4, nodeIndices[index]) == nodes + 4)
			{
				nodes[4] = nodeIndices[index];
				break;
			}
		}

		vtkSmartPointer<vtkPyramid> pyramid = vtkSmartPointer<vtkPyramid>::New();
		for (int nodesIndex = 0; nodesIndex < 5; ++nodesIndex)
		{
			pyramid->GetPointIds()->SetId(nodesIndex, nodes[nodesIndex]);
		}
		vtk_unstructuredGrid->InsertNextCell(pyramid->GetCellType(), pyramid->GetPointIds());
	}
	else
	{
		throw std::invalid_argument("The cell index " + std::to_string(cellIndex) + " is malformed : 5 faces but not a pyramid, not a wedge.");
	}
}

//----------------------------------------------------------------------------
bool ResqmlUnstructuredGridToVtkUnstructuredGrid::cellVtkHexahedron(vtkSmartPointer<vtkUnstructuredGrid> vtk_unstructuredGrid,
																	const RESQML2_NS::UnstructuredGridRepresentation *unstructuredGridRep,
																	ULONG64 const *cumulativeFaceCountPerCell, unsigned char const *cellFaceNormalOutwardlyDirected,
																	ULONG64 cellIndex)
{
	for (ULONG64 localFaceIndex = 0; localFaceIndex < 6; ++localFaceIndex)
	{
		if (unstructuredGridRep->getNodeCountOfFaceOfCell(cellIndex, localFaceIndex) != 4)
		{
			return false;
		}
	}

	ULONG64 nodes[8];

	ULONG64 const *nodeIndices = unstructuredGridRep->getNodeIndicesOfFaceOfCell(cellIndex, 0);
	const size_t cellFaceIndex = unstructuredGridRep->isFaceCountOfCellsConstant() || cellIndex == 0
									 ? cellIndex * 6
									 : cumulativeFaceCountPerCell[cellIndex - 1];
	if (cellFaceNormalOutwardlyDirected[cellFaceIndex] == 0)
	{ // The RESQML orientation of the face honors the VTK orientation of face 0 i.e. the face 0 normal defined using a right hand rule is inwardly directed.
		nodes[0] = nodeIndices[0];
		nodes[1] = nodeIndices[1];
		nodes[2] = nodeIndices[2];
		nodes[3] = nodeIndices[3];
	}
	else
	{ // The RESQML orientation of the face does not honor the VTK orientation of face 0
		nodes[0] = nodeIndices[3];
		nodes[1] = nodeIndices[2];
		nodes[2] = nodeIndices[1];
		nodes[3] = nodeIndices[0];
	}

	// Find the opposite neighbors of the nodes already got
	bool alreadyTreated[4] = {false, false, false, false};
	for (unsigned int localFaceIndex = 1; localFaceIndex < 6; ++localFaceIndex)
	{
		nodeIndices = unstructuredGridRep->getNodeIndicesOfFaceOfCell(cellIndex, localFaceIndex);
		for (size_t index = 0; index < 4; ++index)
		{																	// Loop on face nodes
			ULONG64 *itr = std::find(nodes, nodes + 4, nodeIndices[index]); // Locate a node on face 0
			if (itr != nodes + 4)
			{
				// A top neighbor node can be found
				const size_t topNeigborIdx = std::distance(nodes, itr);
				if (!alreadyTreated[topNeigborIdx])
				{
					const size_t previousIndex = index == 0 ? 3 : index - 1;
					nodes[topNeigborIdx + 4] = std::find(nodes, nodes + 4, nodeIndices[previousIndex]) != nodes + 4 // If previous index is also in face 0
												   ? nodeIndices[index == 3 ? 0 : index + 1]						// Put next index
												   : nodeIndices[previousIndex];									// Put previous index
					alreadyTreated[topNeigborIdx] = true;
				}
			}
		}
		if (localFaceIndex > 2 && std::find(alreadyTreated, alreadyTreated + 4, false) == alreadyTreated + 4)
		{
			// All top neighbor nodes have been found. No need to continue
			// A minimum of four faces is necessary in order to find all top neighbor nodes.
			break;
		}
	}

	vtkSmartPointer<vtkHexahedron> hexahedron = vtkSmartPointer<vtkHexahedron>::New();
	for (int nodesIndex = 0; nodesIndex < 8; ++nodesIndex)
	{
		hexahedron->GetPointIds()->SetId(nodesIndex, nodes[nodesIndex]);
	}
	vtk_unstructuredGrid->InsertNextCell(hexahedron->GetCellType(), hexahedron->GetPointIds());
	return true;
}

//----------------------------------------------------------------------------
bool ResqmlUnstructuredGridToVtkUnstructuredGrid::cellVtkPentagonalPrism(vtkSmartPointer<vtkUnstructuredGrid> vtk_unstructuredGrid,
																		 const RESQML2_NS::UnstructuredGridRepresentation *unstructuredGridRep, ULONG64 cellIndex)
{
	unsigned int faceTo5Nodes = 0;
	ULONG64 nodes[10];
	for (ULONG64 localFaceIndex = 0; localFaceIndex < 7; ++localFaceIndex)
	{
		const unsigned int localNodeCount = unstructuredGridRep->getNodeCountOfFaceOfCell(cellIndex, localFaceIndex);
		if (localNodeCount == 5)
		{
			ULONG64 const *nodeIndices = unstructuredGridRep->getNodeIndicesOfFaceOfCell(cellIndex, localFaceIndex);
			for (unsigned int i = 0; i < localNodeCount; ++i)
			{
				nodes[faceTo5Nodes * 5 + i] = nodeIndices[i];
			}
			++faceTo5Nodes;
		}
	}
	if (faceTo5Nodes == 2)
	{
		vtkSmartPointer<vtkPentagonalPrism> pentagonalPrism = vtkSmartPointer<vtkPentagonalPrism>::New();
		for (int nodesIndex = 0; nodesIndex < 10; ++nodesIndex)
		{
			pentagonalPrism->GetPointIds()->SetId(nodesIndex, nodes[nodesIndex]);
		}
		vtk_unstructuredGrid->InsertNextCell(pentagonalPrism->GetCellType(), pentagonalPrism->GetPointIds());
		pentagonalPrism = nullptr;
		return true;
	}
	return false;
}

//----------------------------------------------------------------------------
bool ResqmlUnstructuredGridToVtkUnstructuredGrid::cellVtkHexagonalPrism(vtkSmartPointer<vtkUnstructuredGrid> vtk_unstructuredGrid,
																		const RESQML2_NS::UnstructuredGridRepresentation *unstructuredGridRep, ULONG64 cellIndex)
{
	unsigned int faceTo6Nodes = 0;
	ULONG64 nodes[12];
	for (ULONG64 localFaceIndex = 0; localFaceIndex < 8; ++localFaceIndex)
	{
		const unsigned int localNodeCount = unstructuredGridRep->getNodeCountOfFaceOfCell(cellIndex, localFaceIndex);
		if (localNodeCount == 6)
		{
			ULONG64 const *nodeIndices = unstructuredGridRep->getNodeIndicesOfFaceOfCell(cellIndex, localFaceIndex);
			for (unsigned int i = 0; i < localNodeCount; ++i)
			{
				nodes[faceTo6Nodes * 6 + i] = nodeIndices[i];
			}
			++faceTo6Nodes;
		}
	}
	if (faceTo6Nodes == 2)
	{
		vtkSmartPointer<vtkHexagonalPrism> hexagonalPrism = vtkSmartPointer<vtkHexagonalPrism>::New();
		for (int nodesIndex = 0; nodesIndex < 12; ++nodesIndex)
		{
			hexagonalPrism->GetPointIds()->SetId(nodesIndex, nodes[nodesIndex]);
		}
		vtk_unstructuredGrid->InsertNextCell(hexagonalPrism->GetCellType(), hexagonalPrism->GetPointIds());
		hexagonalPrism = nullptr;
		return true;
	}
	return false;
}
