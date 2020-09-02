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
#include "VtkUnstructuredGridRepresentation.h"

//SYSTEM
#include <sstream>

// VTK
#include <vtkSmartPointer.h>
#include <vtkCellData.h>
#include <vtkPointData.h>
#include <vtkCellArray.h>
#include <vtkPolyhedron.h>
#include <vtkTetra.h>
#include <vtkHexagonalPrism.h>
#include <vtkPentagonalPrism.h>
#include <vtkHexahedron.h>
#include <vtkPyramid.h>
#include <vtkWedge.h>

// FESAPI
#include <fesapi/resqml2/UnstructuredGridRepresentation.h>

// FESPP
#include "VtkProperty.h"

//----------------------------------------------------------------------------
VtkUnstructuredGridRepresentation::VtkUnstructuredGridRepresentation(const std::string & fileName, const std::string & name, const std::string & uuid, const std::string & uuidParent,
	COMMON_NS::DataObjectRepository const * repoRepresentation, COMMON_NS::DataObjectRepository const * repoSubRepresentation, int idProc, int maxProc) :
	VtkResqml2UnstructuredGrid(fileName, name, uuid, uuidParent, repoRepresentation, repoSubRepresentation, idProc, maxProc)
{
}

//----------------------------------------------------------------------------
void VtkUnstructuredGridRepresentation::visualize(const std::string & uuid)
{
	if (!subRepresentation)	{
		RESQML2_NS::UnstructuredGridRepresentation* unstructuredGridRep = epcPackageRepresentation->getDataObjectByUuid<RESQML2_NS::UnstructuredGridRepresentation>(getUuid());

		if (vtkOutput == nullptr) {
			vtkOutput = vtkSmartPointer<vtkUnstructuredGrid>::New();
			vtkOutput->Allocate(unstructuredGridRep->getCellCount());

			// POINTS
			const ULONG64 pointCount = unstructuredGridRep->getXyzPointCountOfAllPatches();
			double* allXyzPoints = new double[pointCount * 3];  // Will be deleted by VTK;
			unstructuredGridRep->getXyzPointsOfAllPatchesInGlobalCrs(allXyzPoints);
			createVtkPoints(pointCount, allXyzPoints, unstructuredGridRep->getLocalCrs(0));
			vtkOutput->SetPoints(points);
			points = nullptr;

			unstructuredGridRep->loadGeometry();
			// CELLS
			const unsigned int nodeCount = unstructuredGridRep->getNodeCount();
			std::unique_ptr<vtkIdType[]> pointIds(new vtkIdType[nodeCount]);
			for (unsigned int i = 0; i < nodeCount; ++i) {
				pointIds[i] = i;
			}

			const ULONG64 cellCount = unstructuredGridRep->getCellCount();
			ULONG64 const * cumulativeFaceCountPerCell = unstructuredGridRep->isFaceCountOfCellsConstant()
				? nullptr
				: unstructuredGridRep->getCumulativeFaceCountPerCell(); // This pointer is owned and managed by FESAPI
			std::unique_ptr<unsigned char[]> cellFaceNormalOutwardlyDirected(new unsigned char[cumulativeFaceCountPerCell == nullptr
				? cellCount * unstructuredGridRep->getConstantFaceCountOfCells()
				: cumulativeFaceCountPerCell[cellCount - 1]]);
			unstructuredGridRep->getCellFaceIsRightHanded(cellFaceNormalOutwardlyDirected.get());

			auto maxCellIndex = (getIdProc()+1) * cellCount/getMaxProc();

			for (ULONG64 cellIndex = getIdProc() * cellCount / getMaxProc(); cellIndex < maxCellIndex; ++cellIndex) {
				bool isOptimizedCell = false;

				const ULONG64 localFaceCount = unstructuredGridRep->getFaceCountOfCell(cellIndex);
				if (localFaceCount == 4) { // VTK_TETRA
					cellVtkTetra(unstructuredGridRep, cumulativeFaceCountPerCell, cellFaceNormalOutwardlyDirected.get(), cellIndex);
					isOptimizedCell = true;
				}
				else if (localFaceCount == 5) { // VTK_WEDGE or VTK_PYRAMID
					cellVtkWedgeOrPyramid(unstructuredGridRep, cumulativeFaceCountPerCell, cellFaceNormalOutwardlyDirected.get(), cellIndex);
					isOptimizedCell = true;
				}
				else if (localFaceCount == 6) { // VTK_HEXAHEDRON
					isOptimizedCell = cellVtkHexahedron(unstructuredGridRep, cumulativeFaceCountPerCell, cellFaceNormalOutwardlyDirected.get(), cellIndex);
				}
				else if (localFaceCount == 7) { // VTK_PENTAGONAL_PRISM
					isOptimizedCell = cellVtkPentagonalPrism(unstructuredGridRep, cellIndex);
				}
				else if (localFaceCount == 8) { // VTK_HEXAGONAL_PRISM
					isOptimizedCell = cellVtkHexagonalPrism(unstructuredGridRep, cellIndex);
				}

				if (!isOptimizedCell) {
					vtkSmartPointer<vtkCellArray> faces = vtkSmartPointer<vtkCellArray>::New();
					for (ULONG64 localFaceIndex = 0; localFaceIndex < localFaceCount; ++localFaceIndex) {
						const unsigned int localNodeCount = unstructuredGridRep->getNodeCountOfFaceOfCell(cellIndex, localFaceIndex);
						std::unique_ptr<vtkIdType[]> nodes(new vtkIdType[localNodeCount]);
						ULONG64 const * nodeIndices = unstructuredGridRep->getNodeIndicesOfFaceOfCell(cellIndex, localFaceIndex);
						for (unsigned int i = 0; i < localNodeCount; ++i) {
							nodes[i] = nodeIndices[i];
						}
						faces->InsertNextCell(localNodeCount, nodes.get());
					}
					vtkOutput->InsertNextCell(VTK_POLYHEDRON, nodeCount, pointIds.get(), unstructuredGridRep->getFaceCountOfCell(cellIndex), faces->GetPointer());

					faces = nullptr;
				}

			}
			unstructuredGridRep->unloadGeometry();
		}
		// PROPERTY(IES)
		if (uuid != getUuid()) {
			vtkDataArray* arrayProperty = uuidToVtkProperty[uuid]->visualize(uuid, unstructuredGridRep);
			addProperty(uuid, arrayProperty);
		}
	}
}

//----------------------------------------------------------------------------
void VtkUnstructuredGridRepresentation::cellVtkTetra(const RESQML2_NS::UnstructuredGridRepresentation* unstructuredGridRep,
	ULONG64 const * cumulativeFaceCountPerCell, unsigned char const * cellFaceNormalOutwardlyDirected,
	ULONG64 cellIndex) {
	unsigned int nodes[4];

	// Face 0
	ULONG64 const * nodeIndices = unstructuredGridRep->getNodeIndicesOfFaceOfCell(cellIndex, 0);
	size_t cellFaceIndex = unstructuredGridRep->isFaceCountOfCellsConstant() || cellIndex == 0
		? cellIndex * 4
		: cumulativeFaceCountPerCell[cellIndex - 1];
	if (cellFaceNormalOutwardlyDirected[cellFaceIndex] == 0) { // The RESQML orientation of face 0 honors the VTK orientation of face 0 i.e. the face 0 normal defined using a right hand rule is inwardly directed.
		nodes[0] = nodeIndices[0];
		nodes[1] = nodeIndices[1];
		nodes[2] = nodeIndices[2];
	}
	else { // The RESQML orientation of face 0 does not honor the VTK orientation of face 0
		nodes[0] = nodeIndices[2];
		nodes[1] = nodeIndices[1];
		nodes[2] = nodeIndices[0];
	}

	// Face 1
	nodeIndices = unstructuredGridRep->getNodeIndicesOfFaceOfCell(cellIndex, 1);

	for (size_t index = 0; index < 3; ++index) {
		if (std::find(nodes, nodes + 3, nodeIndices[index]) == nodes + 3) {
			nodes[3] = nodeIndices[index];
			break;
		}
	}

	vtkSmartPointer<vtkTetra> tetra = vtkSmartPointer<vtkTetra>::New();
	for (vtkIdType pointId = 0; pointId < 4; ++pointId) {
		tetra->GetPointIds()->SetId(pointId, nodes[pointId]);
	}
	vtkOutput->InsertNextCell(tetra->GetCellType(), tetra->GetPointIds());
}

//----------------------------------------------------------------------------
void VtkUnstructuredGridRepresentation::cellVtkWedgeOrPyramid(const RESQML2_NS::UnstructuredGridRepresentation* unstructuredGridRep,
	ULONG64 const * cumulativeFaceCountPerCell, unsigned char const * cellFaceNormalOutwardlyDirected,
	ULONG64 cellIndex) {
	std::vector<unsigned int> localFaceIndexWith4Nodes;
	for (unsigned int localFaceIndex = 0; localFaceIndex < 5; ++localFaceIndex) {
		if (unstructuredGridRep->getNodeCountOfFaceOfCell(cellIndex, localFaceIndex) == 4) {
			localFaceIndexWith4Nodes.push_back(localFaceIndex);
		}
	}
	if (localFaceIndexWith4Nodes.size() == 3) { // VTK_WEDGE
		ULONG64 nodes[6];
		unsigned int faceTo3Nodes = 0;
		for (unsigned int localFaceIndex = 0; localFaceIndex < 5; ++localFaceIndex) {
			const unsigned int localNodeCount = unstructuredGridRep->getNodeCountOfFaceOfCell(cellIndex, localFaceIndex);
			if (localNodeCount == 3) {
				ULONG64 const* nodeIndices = unstructuredGridRep->getNodeIndicesOfFaceOfCell(cellIndex, localFaceIndex);
				for (unsigned int i = 0; i < localNodeCount; ++i) {
					nodes[faceTo3Nodes * 3 + i] = nodeIndices[i];
				}
				++faceTo3Nodes;
			}
		}

		vtkSmartPointer<vtkWedge> wedge = vtkSmartPointer<vtkWedge>::New();
		for (int nodesIndex = 0; nodesIndex < 6; ++nodesIndex) {
			wedge->GetPointIds()->SetId(nodesIndex, nodes[nodesIndex]);
		}
		vtkOutput->InsertNextCell(wedge->GetCellType(), wedge->GetPointIds());
	}
	else if (localFaceIndexWith4Nodes.size() == 1) { // VTK_PYRAMID
		ULONG64 nodes[5];

		ULONG64 const* nodeIndices = unstructuredGridRep->getNodeIndicesOfFaceOfCell(cellIndex, localFaceIndexWith4Nodes[0]);
		size_t cellFaceIndex = (unstructuredGridRep->isFaceCountOfCellsConstant() || cellIndex == 0
			? cellIndex * 5
			: cumulativeFaceCountPerCell[cellIndex - 1]) + localFaceIndexWith4Nodes[0];
		if (cellFaceNormalOutwardlyDirected[cellFaceIndex] == 0) { // The RESQML orientation of the face honors the VTK orientation of face 0 i.e. the face 0 normal defined using a right hand rule is inwardly directed.
			nodes[0] = nodeIndices[0];
			nodes[1] = nodeIndices[1];
			nodes[2] = nodeIndices[2];
			nodes[3] = nodeIndices[3];
		}
		else { // The RESQML orientation of the face does not honor the VTK orientation of face 0
			nodes[0] = nodeIndices[3];
			nodes[1] = nodeIndices[2];
			nodes[2] = nodeIndices[1];
			nodes[3] = nodeIndices[0];
		}

		// Face with 3 points
		nodeIndices = unstructuredGridRep->getNodeIndicesOfFaceOfCell(cellIndex, localFaceIndexWith4Nodes[0] == 0 ? 1 : 0);

		for (size_t index = 0; index < 3; ++index) {
			if (std::find(nodes, nodes + 4, nodeIndices[index]) == nodes + 4) {
				nodes[4] = nodeIndices[index];
				break;
			}
		}

		vtkSmartPointer<vtkPyramid> pyramid = vtkSmartPointer<vtkPyramid>::New();
		for (int nodesIndex = 0; nodesIndex < 5; ++nodesIndex) {
			pyramid->GetPointIds()->SetId(nodesIndex, nodes[nodesIndex]);
		}
		vtkOutput->InsertNextCell(pyramid->GetCellType(), pyramid->GetPointIds());
	}
	else {
		throw std::invalid_argument("The cell index " + std::to_string(cellIndex) + " is malformed : 5 faces but not a pyramid, not a wedge.");
	}
}

//----------------------------------------------------------------------------
bool VtkUnstructuredGridRepresentation::cellVtkHexahedron(const RESQML2_NS::UnstructuredGridRepresentation* unstructuredGridRep,
	ULONG64 const * cumulativeFaceCountPerCell, unsigned char const * cellFaceNormalOutwardlyDirected,
	ULONG64 cellIndex)
{
	for (ULONG64 localFaceIndex = 0; localFaceIndex < 6; ++localFaceIndex) {
		if (unstructuredGridRep->getNodeCountOfFaceOfCell(cellIndex, localFaceIndex) != 4) {
			return false;
		}
	}

	ULONG64 nodes[8];

	ULONG64 const* nodeIndices = unstructuredGridRep->getNodeIndicesOfFaceOfCell(cellIndex, 0);
	const size_t cellFaceIndex = unstructuredGridRep->isFaceCountOfCellsConstant() || cellIndex == 0
		? cellIndex * 6
		: cumulativeFaceCountPerCell[cellIndex - 1];
	if (cellFaceNormalOutwardlyDirected[cellFaceIndex] == 0) { // The RESQML orientation of the face honors the VTK orientation of face 0 i.e. the face 0 normal defined using a right hand rule is inwardly directed.
		nodes[0] = nodeIndices[0];
		nodes[1] = nodeIndices[1];
		nodes[2] = nodeIndices[2];
		nodes[3] = nodeIndices[3];
	}
	else { // The RESQML orientation of the face does not honor the VTK orientation of face 0
		nodes[0] = nodeIndices[3];
		nodes[1] = nodeIndices[2];
		nodes[2] = nodeIndices[1];
		nodes[3] = nodeIndices[0];
	}

	// Find the opposite neighbors of the nodes already got
	bool alreadyTreated[4] = { false, false, false, false };
	for (unsigned int localFaceIndex = 1; localFaceIndex < 6; ++localFaceIndex) {
		nodeIndices = unstructuredGridRep->getNodeIndicesOfFaceOfCell(cellIndex, localFaceIndex);
		for (size_t index = 0; index < 4; ++index) { // Loop on face nodes
			ULONG64* itr = std::find(nodes, nodes + 4, nodeIndices[index]); // Locate a node on face 0
			if (itr != nodes + 4) {
				// A top neighbor node can be found
				const size_t topNeigborIdx = std::distance(nodes, itr);
				if (!alreadyTreated[topNeigborIdx]) {
					const size_t previousIndex = index == 0 ? 3 : index - 1;
					nodes[topNeigborIdx + 4] = std::find(nodes, nodes + 4, nodeIndices[previousIndex]) != nodes + 4 // If previous index is also in face 0
						? nodeIndices[index == 3 ? 0 : index + 1] // Put next index
						: nodeIndices[previousIndex]; // Put previous index
					alreadyTreated[topNeigborIdx] = true;
				}
			}
		}
		if (localFaceIndex > 2 && std::find(alreadyTreated, alreadyTreated + 4, false) == alreadyTreated + 4) {
			// All top neighbor nodes have been found. No need to continue
			// A minimum of four faces is necessary in order to find all top neighbor nodes.
			break;
		}
	}

	vtkSmartPointer<vtkHexahedron> hexahedron = vtkSmartPointer<vtkHexahedron>::New();
	for (int nodesIndex = 0; nodesIndex < 8; ++nodesIndex) {
		hexahedron->GetPointIds()->SetId(nodesIndex, nodes[nodesIndex]);
	}
	vtkOutput->InsertNextCell(hexahedron->GetCellType(), hexahedron->GetPointIds());
	return true;
}

//----------------------------------------------------------------------------
bool VtkUnstructuredGridRepresentation::cellVtkPentagonalPrism(const RESQML2_NS::UnstructuredGridRepresentation* unstructuredGridRep, ULONG64 cellIndex) {
	unsigned int faceTo5Nodes = 0;
	ULONG64 nodes[10];
	for (ULONG64 localFaceIndex = 0; localFaceIndex < 7; ++localFaceIndex) {
		const unsigned int localNodeCount = unstructuredGridRep->getNodeCountOfFaceOfCell(cellIndex, localFaceIndex);
		if (localNodeCount == 5) {
			ULONG64 const* nodeIndices = unstructuredGridRep->getNodeIndicesOfFaceOfCell(cellIndex, localFaceIndex);
			for (unsigned int i = 0; i < localNodeCount; ++i) {
				nodes[faceTo5Nodes*5+i] = nodeIndices[i];
			}
			++faceTo5Nodes;
		}
	}
	if (faceTo5Nodes == 2) {
		vtkSmartPointer<vtkPentagonalPrism> pentagonalPrism = vtkSmartPointer<vtkPentagonalPrism>::New();
		for (int nodesIndex = 0; nodesIndex < 10; ++nodesIndex) {
			pentagonalPrism->GetPointIds()->SetId(nodesIndex, nodes[nodesIndex]);
		}
		vtkOutput->InsertNextCell(pentagonalPrism->GetCellType(), pentagonalPrism->GetPointIds());
		pentagonalPrism = nullptr;
		return true;
	}
	return false;
}

//----------------------------------------------------------------------------
bool VtkUnstructuredGridRepresentation::cellVtkHexagonalPrism(const RESQML2_NS::UnstructuredGridRepresentation* unstructuredGridRep, ULONG64 cellIndex) {
	unsigned int faceTo6Nodes = 0;
	ULONG64 nodes[12];
	for (ULONG64 localFaceIndex = 0; localFaceIndex < 8; ++localFaceIndex) {
		const unsigned int localNodeCount = unstructuredGridRep->getNodeCountOfFaceOfCell(cellIndex, localFaceIndex);
		if (localNodeCount == 6) {
			ULONG64 const* nodeIndices = unstructuredGridRep->getNodeIndicesOfFaceOfCell(cellIndex, localFaceIndex);
			for (unsigned int i = 0; i < localNodeCount; ++i) {
				nodes[faceTo6Nodes*6+i] = nodeIndices[i];
			}
			++faceTo6Nodes;
		}
	}
	if (faceTo6Nodes == 2) {
		vtkSmartPointer<vtkHexagonalPrism> hexagonalPrism = vtkSmartPointer<vtkHexagonalPrism>::New();
		for (int nodesIndex = 0; nodesIndex < 12; ++nodesIndex) {
			hexagonalPrism->GetPointIds()->SetId(nodesIndex, nodes[nodesIndex]);
		}
		vtkOutput->InsertNextCell(hexagonalPrism->GetCellType(), hexagonalPrism->GetPointIds());
		hexagonalPrism = nullptr;
		return true;
	}
	return false;
}

//----------------------------------------------------------------------------
void VtkUnstructuredGridRepresentation::addProperty(const std::string & uuidProperty, vtkDataArray* dataProperty)
{
	vtkOutput->Modified();
	const unsigned int propertyAttachmentKind = uuidToVtkProperty[uuidProperty]->getSupport();
	if (propertyAttachmentKind == VtkProperty::typeSupport::CELLS) {
		vtkOutput->GetCellData()->AddArray(dataProperty);
	}
	else if (propertyAttachmentKind == VtkProperty::typeSupport::POINTS) {
		vtkOutput->GetPointData()->AddArray(dataProperty);
	}
}

//----------------------------------------------------------------------------
long VtkUnstructuredGridRepresentation::getAttachmentPropertyCount(const std::string &, VtkEpcCommon::FesppAttachmentProperty propertyUnit)
{
	long result = 0;
	RESQML2_NS::UnstructuredGridRepresentation* unstructuredGridRep = epcPackageRepresentation->getDataObjectByUuid<RESQML2_NS::UnstructuredGridRepresentation>(getUuid());
	if (unstructuredGridRep != nullptr) {
		if (propertyUnit == VtkEpcCommon::FesppAttachmentProperty::POINTS) {
			result = unstructuredGridRep->getXyzPointCountOfAllPatches();
		}
		else if (propertyUnit== VtkEpcCommon::FesppAttachmentProperty::CELLS) {
			result = unstructuredGridRep->getCellCount();
		}
	}
	return result;
}
