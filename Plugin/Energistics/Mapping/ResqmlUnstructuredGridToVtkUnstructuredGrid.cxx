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
#include "Mapping/ResqmlUnstructuredGridToVtkUnstructuredGrid.h"

// VTK
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
#include <vtkUnstructuredGrid.h>
#include <vtkPyramid.h>
#include <vtkWedge.h>
#include <vtkIdList.h>

// FESAPI
#include <fesapi/resqml2/UnstructuredGridRepresentation.h>
#include <fesapi/resqml2/AbstractLocal3dCrs.h>

//----------------------------------------------------------------------------
ResqmlUnstructuredGridToVtkUnstructuredGrid::ResqmlUnstructuredGridToVtkUnstructuredGrid(RESQML2_NS::UnstructuredGridRepresentation *unstructuredGrid, int proc_number, int max_proc)
	: ResqmlAbstractRepresentationToVtkPartitionedDataSet(unstructuredGrid,
											   proc_number,
											   max_proc),
	points(vtkSmartPointer<vtkPoints>::New())
{
	this->pointCount = unstructuredGrid->getXyzPointCountOfAllPatches();
	this->iCellCount = unstructuredGrid->getCellCount();

	this->vtkData = vtkSmartPointer<vtkPartitionedDataSet>::New();

	this->vtkData->Modified();
}

//----------------------------------------------------------------------------
RESQML2_NS::UnstructuredGridRepresentation * ResqmlUnstructuredGridToVtkUnstructuredGrid::getResqmlData() const
{
	return static_cast<RESQML2_NS::UnstructuredGridRepresentation *>(resqmlData);
}

//----------------------------------------------------------------------------
void ResqmlUnstructuredGridToVtkUnstructuredGrid::loadVtkObject()
{
	vtkSmartPointer<vtkUnstructuredGrid> vtk_unstructuredGrid = vtkSmartPointer<vtkUnstructuredGrid>::New();
	RESQML2_NS::UnstructuredGridRepresentation * unstructuredGrid = getResqmlData();

	vtk_unstructuredGrid->AllocateExact(unstructuredGrid->getCellCount(), unstructuredGrid->getXyzPointCountOfAllPatches());

	// POINTS
	vtk_unstructuredGrid->SetPoints(this->getVtkPoints());

	unstructuredGrid->loadGeometry();

	const uint64_t cellCount = unstructuredGrid->getCellCount();
	uint64_t const* cumulativeFaceCountPerCell = unstructuredGrid->isFaceCountOfCellsConstant()
		? nullptr
		: unstructuredGrid->getCumulativeFaceCountPerCell(); // This pointer is owned and managed by FESAPI
	const uint64_t faceCount = cumulativeFaceCountPerCell == nullptr
		? cellCount * unstructuredGrid->getConstantFaceCountOfCells()
		: cumulativeFaceCountPerCell[cellCount - 1];
	std::unique_ptr<unsigned char[]> cellFaceNormalOutwardlyDirected(new unsigned char[faceCount]);

	unstructuredGrid->getCellFaceIsRightHanded(cellFaceNormalOutwardlyDirected.get());
	auto* crs = unstructuredGrid->getLocalCrs(0);
	if (!crs->isPartial() && crs->isDepthOriented())
	{
		for (size_t i = 0; i < faceCount; ++i) {
			cellFaceNormalOutwardlyDirected[i] = !cellFaceNormalOutwardlyDirected[i];
		}
	}

	auto maxCellIndex = (this->procNumber + 1) * cellCount / this->maxProc;

	for (uint64_t  cellIndex = this->procNumber * cellCount / this->maxProc; cellIndex < maxCellIndex; ++cellIndex)
	{
		bool isOptimizedCell = false;

		const uint64_t localFaceCount = unstructuredGrid->getFaceCountOfCell(cellIndex);

		// Following https://kitware.github.io/vtk-examples/site/VTKBook/05Chapter5/#Figure%205-2
		if (localFaceCount == 4)
		{ // VTK_TETRA
			cellVtkTetra(vtk_unstructuredGrid, cumulativeFaceCountPerCell, cellFaceNormalOutwardlyDirected.get(), cellIndex);
			isOptimizedCell = true;
		}
		else if (localFaceCount == 5)
		{ // VTK_WEDGE or VTK_PYRAMID
			cellVtkWedgeOrPyramid(vtk_unstructuredGrid, cumulativeFaceCountPerCell, cellFaceNormalOutwardlyDirected.get(), cellIndex);
			isOptimizedCell = true;
		}
		else if (localFaceCount == 6)
		{ // VTK_HEXAHEDRON
			isOptimizedCell = cellVtkHexahedron(vtk_unstructuredGrid, cumulativeFaceCountPerCell, cellFaceNormalOutwardlyDirected.get(), cellIndex);
		}
		else if (localFaceCount == 7)
		{ // VTK_PENTAGONAL_PRISM
			isOptimizedCell = cellVtkPentagonalPrism(vtk_unstructuredGrid, cumulativeFaceCountPerCell, cellFaceNormalOutwardlyDirected.get(), cellIndex);
		}
		else if (localFaceCount == 8)
		{ // VTK_HEXAGONAL_PRISM
			isOptimizedCell = cellVtkHexagonalPrism(vtk_unstructuredGrid, cumulativeFaceCountPerCell, cellFaceNormalOutwardlyDirected.get(), cellIndex);
		}

		if (!isOptimizedCell)
		{
			vtkSmartPointer<vtkIdList> idList = vtkSmartPointer<vtkIdList>::New();

			// For polyhedron cell, a special ptIds input format is required : (numCellFaces, numFace0Pts, id1, id2, id3, numFace1Pts, id1, id2, id3, ...)
			idList->InsertNextId(localFaceCount);
			for (uint64_t localFaceIndex = 0; localFaceIndex < localFaceCount; ++localFaceIndex)
			{
				const uint64_t localNodeCount = unstructuredGrid->getNodeCountOfFaceOfCell(cellIndex, localFaceIndex);
				idList->InsertNextId(localNodeCount);
				uint64_t const *nodeIndices = unstructuredGrid->getNodeIndicesOfFaceOfCell(cellIndex, localFaceIndex);
				for (size_t i = 0; i < localNodeCount; ++i)
				{
					idList->InsertNextId(nodeIndices[i]);
				}
			}

			vtk_unstructuredGrid->InsertNextCell(VTK_POLYHEDRON, idList);
		}
	}

	unstructuredGrid->unloadGeometry();

	this->vtkData->SetPartition(0, vtk_unstructuredGrid);
	this->vtkData->Modified();
}
//----------------------------------------------------------------------------
vtkSmartPointer<vtkPoints> ResqmlUnstructuredGridToVtkUnstructuredGrid::getVtkPoints()
{
	if (this->points->GetNumberOfPoints() < 1)
	{
		createPoints();
	}
	return this->points;
}

//----------------------------------------------------------------------------
void ResqmlUnstructuredGridToVtkUnstructuredGrid::createPoints()
{
	RESQML2_NS::UnstructuredGridRepresentation * unstructuredGrid = getResqmlData();

	// POINTS
	double *allXyzPoints = new double[this->pointCount * 3]; // Will be deleted by VTK;
	bool partialCRS = false;
	const uint64_t patchCount = unstructuredGrid->getPatchCount();
	for (uint_fast64_t  patchIndex = 0; patchIndex < patchCount; ++patchIndex) {
		if (unstructuredGrid->getLocalCrs(patchIndex)->isPartial()) {
			partialCRS = true;
			break;
		}
	}
	if (partialCRS) {
		vtkOutputWindowDisplayWarningText(("At least one of the local CRS of  : " + unstructuredGrid->getUuid() + " is partial. Get coordinates in local CRS instead.\n").c_str());
		unstructuredGrid->getXyzPointsOfAllPatches(allXyzPoints);
	}
	else {
		unstructuredGrid->getXyzPointsOfAllPatchesInGlobalCrs(allXyzPoints);
	}

	const uint64_t coordCount = pointCount * 3;
	if (!partialCRS && unstructuredGrid->getLocalCrs(0)->isDepthOriented())
	{
		for (uint_fast64_t  zCoordIndex = 2; zCoordIndex < coordCount; zCoordIndex += 3)
		{
			allXyzPoints[zCoordIndex] *= -1;
		}
	}
	vtkSmartPointer<vtkDoubleArray> vtkUnderlyingArray = vtkSmartPointer<vtkDoubleArray>::New();
	vtkUnderlyingArray->SetNumberOfComponents(3);
	// Take ownership of the underlying C array
	vtkUnderlyingArray->SetArray(allXyzPoints, coordCount, vtkAbstractArray::VTK_DATA_ARRAY_DELETE);
	this->points->SetData(vtkUnderlyingArray);
}

//----------------------------------------------------------------------------
void ResqmlUnstructuredGridToVtkUnstructuredGrid::cellVtkTetra(vtkSmartPointer<vtkUnstructuredGrid> vtk_unstructuredGrid,
															   uint64_t const *cumulativeFaceCountPerCell, unsigned char const *cellFaceNormalOutwardlyDirected,
															   uint64_t cellIndex)
{
	RESQML2_NS::UnstructuredGridRepresentation const* unstructuredGrid = getResqmlData();

	// Face 0
	uint64_t const *nodeIndices = unstructuredGrid->getNodeIndicesOfFaceOfCell(cellIndex, 0);
	size_t cellFaceIndex = unstructuredGrid->isFaceCountOfCellsConstant() || cellIndex == 0
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
	nodeIndices = unstructuredGrid->getNodeIndicesOfFaceOfCell(cellIndex, 1);

	for (size_t index = 0; index < 3; ++index)
	{
		if (std::find(nodes.data(), nodes.data() + 3, nodeIndices[index]) == nodes.data() + 3)
		{
			nodes[3] = nodeIndices[index];
			break;
		}
	}

	vtk_unstructuredGrid->InsertNextCell(VTK_TETRA, 4, nodes.data());
}

//----------------------------------------------------------------------------
void ResqmlUnstructuredGridToVtkUnstructuredGrid::cellVtkWedgeOrPyramid(vtkSmartPointer<vtkUnstructuredGrid> vtk_unstructuredGrid,
	uint64_t const* cumulativeFaceCountPerCell, unsigned char const* cellFaceNormalOutwardlyDirected,
	uint64_t cellIndex)
{
	RESQML2_NS::UnstructuredGridRepresentation const* unstructuredGrid = getResqmlData();

	// The global index of the first face of the polyhedron in the cellFaceNormalOutwardlyDirected array
	const size_t globalFirstFaceIndex = unstructuredGrid->isFaceCountOfCellsConstant() || cellIndex == 0
		? cellIndex * 5
		: cumulativeFaceCountPerCell[cellIndex - 1];

	std::vector<unsigned int> localFaceIndexWith4Nodes;
	for (unsigned int localFaceIndex = 0; localFaceIndex < 5; ++localFaceIndex)
	{
		if (unstructuredGrid->getNodeCountOfFaceOfCell(cellIndex, localFaceIndex) == 4)
		{
			localFaceIndexWith4Nodes.push_back(localFaceIndex);
		}
	}
	if (localFaceIndexWith4Nodes.size() == 3)
	{ // VTK_WEDGE
		// Set the triangle base of the wedge
		unsigned int triangleIndex = 0;
		for (; triangleIndex < 5; ++triangleIndex)
		{
			const unsigned int localNodeCount = unstructuredGrid->getNodeCountOfFaceOfCell(cellIndex, triangleIndex);
			if (localNodeCount == 3)
			{
				uint64_t const* nodeIndices = unstructuredGrid->getNodeIndicesOfFaceOfCell(cellIndex, triangleIndex);
				if (cellFaceNormalOutwardlyDirected[globalFirstFaceIndex + triangleIndex] == 0)
				{
					for (size_t i = 0; i < 3; ++i)
					{
						nodes[i] = nodeIndices[2 - i];
					}
				}
				else
				{
					// The RESQML orientation of face 0 honors the VTK orientation of face 0 i.e. the face 0 normal defined using a right hand rule is outwardly directed.
					for (size_t i = 0; i < 3; ++i)
					{
						nodes[i] = nodeIndices[i];
					}
				}
				++triangleIndex;
				break;
			}
		}
		// Find the index of the vertex at the opposite triangle regarding the triangle base
		for (unsigned int localFaceIndex = 0; localFaceIndex < 5; ++localFaceIndex)
		{
			const unsigned int localNodeCount = unstructuredGrid->getNodeCountOfFaceOfCell(cellIndex, localFaceIndex);
			if (localNodeCount == 4)
			{
				uint64_t const* nodeIndices = unstructuredGrid->getNodeIndicesOfFaceOfCell(cellIndex, localFaceIndex);
				if (nodeIndices[0] == nodes[0]) {
					nodes[3] = nodeIndices[1] == nodes[1] || nodeIndices[1] == nodes[2]
						? nodeIndices[3] : nodeIndices[1];
					break;
				}
				else if (nodeIndices[1] == nodes[0]) {
					nodes[3] = nodeIndices[2] == nodes[1] || nodeIndices[2] == nodes[2]
						? nodeIndices[0] : nodeIndices[2];
					break;
				}
				else if (nodeIndices[2] == nodes[0]) {
					nodes[3] = nodeIndices[3] == nodes[1] || nodeIndices[3] == nodes[2]
						? nodeIndices[1] : nodeIndices[3];
					break;
				}
				else if (nodeIndices[3] == nodes[0]) {
					nodes[3] = nodeIndices[0] == nodes[1] || nodeIndices[0] == nodes[2]
						? nodeIndices[2] : nodeIndices[0];
					break;
				}
			}
		}
		// Set the other triangle of the wedge
		for (; triangleIndex < 5; ++triangleIndex)
		{
			const unsigned int localNodeCount = unstructuredGrid->getNodeCountOfFaceOfCell(cellIndex, triangleIndex);
			if (localNodeCount == 3)
			{
				uint64_t const* nodeIndices = unstructuredGrid->getNodeIndicesOfFaceOfCell(cellIndex, triangleIndex);
				if (nodeIndices[0] == nodes[3])
				{
					if (cellFaceNormalOutwardlyDirected[globalFirstFaceIndex + triangleIndex] == 0)
					{
						nodes[4] = nodeIndices[1];
						nodes[5] = nodeIndices[2];
					}
					else
					{
						nodes[4] = nodeIndices[2];
						nodes[5] = nodeIndices[1];
					}
				}
				else if (nodeIndices[1] == nodes[3])
				{
					if (cellFaceNormalOutwardlyDirected[globalFirstFaceIndex + triangleIndex] == 0)
					{
						nodes[4] = nodeIndices[2];
						nodes[5] = nodeIndices[0];
					}
					else
					{
						nodes[4] = nodeIndices[0];
						nodes[5] = nodeIndices[2];
					}
				}
				else if (nodeIndices[2] == nodes[3])
				{
					if (cellFaceNormalOutwardlyDirected[globalFirstFaceIndex + triangleIndex] == 0)
					{
						nodes[4] = nodeIndices[0];
						nodes[5] = nodeIndices[1];
					}
					else
					{
						nodes[4] = nodeIndices[1];
						nodes[5] = nodeIndices[0];
					}
				}
				break;
			}
		}

		vtk_unstructuredGrid->InsertNextCell(VTK_WEDGE, 6, nodes.data());
	}
	else if (localFaceIndexWith4Nodes.size() == 1)
	{ // VTK_PYRAMID
	uint64_t const* nodeIndices = unstructuredGrid->getNodeIndicesOfFaceOfCell(cellIndex, localFaceIndexWith4Nodes[0]);
		size_t cellFaceIndex = (unstructuredGrid->isFaceCountOfCellsConstant() || cellIndex == 0
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
		nodeIndices = unstructuredGrid->getNodeIndicesOfFaceOfCell(cellIndex, localFaceIndexWith4Nodes[0] == 0 ? 1 : 0);

		for (size_t index = 0; index < 3; ++index)
		{
			if (std::find(nodes.data(), nodes.data() + 4, nodeIndices[index]) == nodes.data() + 4)
			{
				nodes[4] = nodeIndices[index];
				break;
			}
		}

		vtk_unstructuredGrid->InsertNextCell(VTK_PYRAMID, 5, nodes.data());
	}
	else
	{
		throw std::invalid_argument("The cell index " + std::to_string(cellIndex) + " is malformed : 5 faces but not a pyramid, not a wedge.");
	}
}

//----------------------------------------------------------------------------
bool ResqmlUnstructuredGridToVtkUnstructuredGrid::cellVtkHexahedron(vtkSmartPointer<vtkUnstructuredGrid> vtk_unstructuredGrid,
	uint64_t const *cumulativeFaceCountPerCell, unsigned char const *cellFaceNormalOutwardlyDirected,
	uint64_t cellIndex)
{
	RESQML2_NS::UnstructuredGridRepresentation const* unstructuredGrid = getResqmlData();

	for (unsigned int localFaceIndex = 0; localFaceIndex < 6; ++localFaceIndex)
	{
		if (unstructuredGrid->getNodeCountOfFaceOfCell(cellIndex, localFaceIndex) != 4)
		{
			return false;
		}
	}

	uint64_t const *nodeIndices = unstructuredGrid->getNodeIndicesOfFaceOfCell(cellIndex, 0);
	const size_t cellFaceIndex = unstructuredGrid->isFaceCountOfCellsConstant() || cellIndex == 0
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
		nodeIndices = unstructuredGrid->getNodeIndicesOfFaceOfCell(cellIndex, localFaceIndex);
		for (size_t index = 0; index < 4; ++index)
		{																	// Loop on face nodes
			vtkIdType* itr = std::find(nodes.data(), nodes.data() + 4, nodeIndices[index]); // Locate a node on face 0
			if (itr != nodes.data() + 4)
			{
				// A top neighbor node can be found
				const size_t topNeigborIdx = std::distance(nodes.data(), itr);
				if (!alreadyTreated[topNeigborIdx])
				{
					const size_t previousIndex = index == 0 ? 3 : index - 1;
					nodes[topNeigborIdx + 4] = std::find(nodes.data(), nodes.data() + 4, nodeIndices[previousIndex]) != nodes.data() + 4 // If previous index is also in face 0
												   ? nodeIndices[index == 3 ? 0 : index + 1]						// Put next index
												   : nodeIndices[previousIndex];									// Put previous index
					alreadyTreated[topNeigborIdx] = true;
				}
			}
		}
	}

	vtk_unstructuredGrid->InsertNextCell(VTK_HEXAHEDRON, 8, nodes.data());
	return true;
}

//----------------------------------------------------------------------------
bool ResqmlUnstructuredGridToVtkUnstructuredGrid::cellVtkPentagonalPrism(vtkSmartPointer<vtkUnstructuredGrid> vtk_unstructuredGrid,
	uint64_t const *cumulativeFaceCountPerCell, unsigned char const *cellFaceNormalOutwardlyDirected, uint64_t cellIndex)
{
	RESQML2_NS::UnstructuredGridRepresentation const* unstructuredGrid = getResqmlData();

	// Found the base 5 nodes face
	for (uint32_t localFaceIndex = 0; localFaceIndex < 7; ++localFaceIndex)
	{
		const uint64_t localNodeCount = unstructuredGrid->getNodeCountOfFaceOfCell(cellIndex, localFaceIndex);
		if (localNodeCount == 5)
		{
			const size_t cellFaceIndex = (unstructuredGrid->isFaceCountOfCellsConstant() || cellIndex == 0
				? cellIndex * 7
				: cumulativeFaceCountPerCell[cellIndex - 1]) + localFaceIndex;

			uint64_t const *nodeIndices = unstructuredGrid->getNodeIndicesOfFaceOfCell(cellIndex, localFaceIndex);
			if (cellFaceNormalOutwardlyDirected[cellFaceIndex] == 0)
			{ // The RESQML orientation of the face honors the VTK orientation of face 0 i.e. the face 0 normal defined using a right hand rule is inwardly directed.
				std::copy(nodeIndices, nodeIndices + 5, nodes.data());
			}
			else
			{ // The RESQML orientation of the face does not honor the VTK orientation of face 0
				std::reverse_copy(nodeIndices, nodeIndices + 5, nodes.data());
			}
			break; // We have found the base face
		}
	}

	// Find the other nodes from the 4 nodes faces
	uint_fast8_t faceWith5Nodes = 0;
	uint_fast8_t faceWith4Nodes = 0;
	std::array<bool, 5> alreadyTreated = { false, false, false, false, false };
	for (uint32_t localFaceIndex = 0; localFaceIndex < 7; ++localFaceIndex)
	{
		const uint64_t localNodeCount = unstructuredGrid->getNodeCountOfFaceOfCell(cellIndex, localFaceIndex);
		if (localNodeCount == 4)
		{
			uint64_t const *nodeIndices = unstructuredGrid->getNodeIndicesOfFaceOfCell(cellIndex, localFaceIndex);
			for (size_t index = 0; index < 4; ++index) // Loop on face nodes
			{
				vtkIdType* itr = std::find(nodes.data(), nodes.data() + 5, nodeIndices[index]); // Locate a node on base face
				if (itr != nodes.data() + 5)
				{
					// A top neighbor node can be found
					const size_t topNeigborIdx = std::distance(nodes.data(), itr);
					if (!alreadyTreated[topNeigborIdx])
					{
						const size_t previousIndex = index == 0 ? 3 : index - 1;
						nodes[topNeigborIdx + 5] = std::find(nodes.data(), nodes.data() + 5, nodeIndices[previousIndex]) != nodes.data() + 5 // If previous index is also in face 0
							? nodeIndices[index == 3 ? 0 : index + 1]						// Put next index
							: nodeIndices[previousIndex];									// Put previous index
						alreadyTreated[topNeigborIdx] = true;
					}
				}
			}
			++faceWith4Nodes;
		}
		else if (localNodeCount == 5) {
			++faceWith5Nodes;
		}
	}

	if (faceWith5Nodes == 2 && faceWith4Nodes == 5)
	{
		vtk_unstructuredGrid->InsertNextCell(VTK_PENTAGONAL_PRISM, 10, nodes.data());
		return true;
	}
	return false;
}

//----------------------------------------------------------------------------
bool ResqmlUnstructuredGridToVtkUnstructuredGrid::cellVtkHexagonalPrism(vtkSmartPointer<vtkUnstructuredGrid> vtk_unstructuredGrid,
	uint64_t const *cumulativeFaceCountPerCell, unsigned char const *cellFaceNormalOutwardlyDirected, uint64_t cellIndex)
{
	RESQML2_NS::UnstructuredGridRepresentation * unstructuredGrid = getResqmlData();

	// Found the base 5 nodes face
	for (uint32_t localFaceIndex = 0; localFaceIndex < 8; ++localFaceIndex)
	{
		const uint64_t localNodeCount = unstructuredGrid->getNodeCountOfFaceOfCell(cellIndex, localFaceIndex);
		if (localNodeCount == 6)
		{
			const size_t cellFaceIndex = (unstructuredGrid->isFaceCountOfCellsConstant() || cellIndex == 0
				? cellIndex * 7
				: cumulativeFaceCountPerCell[cellIndex - 1]) + localFaceIndex;

			uint64_t const *nodeIndices = unstructuredGrid->getNodeIndicesOfFaceOfCell(cellIndex, localFaceIndex);
			if (cellFaceNormalOutwardlyDirected[cellFaceIndex] == 0)
			{ // The RESQML orientation of the face honors the VTK orientation of face 0 i.e. the face 0 normal defined using a right hand rule is inwardly directed.
				std::copy(nodeIndices, nodeIndices + 6, nodes.data());
			}
			else
			{ // The RESQML orientation of the face does not honor the VTK orientation of face 0
				std::reverse_copy(nodeIndices, nodeIndices + 6, nodes.data());
			}
			break; // We have found the base face
		}
	}

	// Find the other nodes from the 4 nodes faces
	uint_fast8_t faceWith6Nodes = 0;
	uint_fast8_t faceWith4Nodes = 0;
	std::array<bool, 6> alreadyTreated = { false, false, false, false, false, false };
	for (uint32_t localFaceIndex = 0; localFaceIndex < 8; ++localFaceIndex)
	{
		const uint64_t localNodeCount = unstructuredGrid->getNodeCountOfFaceOfCell(cellIndex, localFaceIndex);
		if (localNodeCount == 4)
		{
			uint64_t const *nodeIndices = unstructuredGrid->getNodeIndicesOfFaceOfCell(cellIndex, localFaceIndex);
			for (size_t index = 0; index < 4; ++index) // Loop on face nodes
			{
				vtkIdType* itr = std::find(nodes.data(), nodes.data() + 6, nodeIndices[index]); // Locate a node on base face
				if (itr != nodes.data() + 6)
				{
					// A top neighbor node can be found
					const size_t topNeigborIdx = std::distance(nodes.data(), itr);
					if (!alreadyTreated[topNeigborIdx])
					{
						const size_t previousIndex = index == 0 ? 3 : index - 1;
						nodes[topNeigborIdx + 6] = std::find(nodes.data(), nodes.data() + 6, nodeIndices[previousIndex]) != nodes.data() + 6 // If previous index is also in face 0
							? nodeIndices[index == 3 ? 0 : index + 1]						// Put next index
							: nodeIndices[previousIndex];									// Put previous index
						alreadyTreated[topNeigborIdx] = true;
					}
				}
			}
			++faceWith4Nodes;
		}
		else if (localNodeCount == 6) {
			++faceWith6Nodes;
		}
	}

	if (faceWith6Nodes == 2 && faceWith4Nodes == 6)
	{
		vtk_unstructuredGrid->InsertNextCell(VTK_HEXAGONAL_PRISM, 12, nodes.data());
		return true;
	}
	return false;
}
