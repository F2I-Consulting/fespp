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
void VtkUnstructuredGridRepresentation::createOutput(const std::string & uuid)
{
	if (!subRepresentation)	{
		RESQML2_NS::UnstructuredGridRepresentation* unstructuredGridRep = epcPackageRepresentation->getDataObjectByUuid<RESQML2_NS::UnstructuredGridRepresentation>(getUuid());

		if (!vtkOutput) {
			vtkOutput = vtkSmartPointer<vtkUnstructuredGrid>::New();
			// POINTS
			const ULONG64 pointCount = unstructuredGridRep->getXyzPointCountOfAllPatches();
			std::unique_ptr<double[]> allXyzPoints(new double[pointCount * 3]);
			unstructuredGridRep->getXyzPointsOfAllPatchesInGlobalCrs(allXyzPoints.get());
			createVtkPoints(pointCount, allXyzPoints.get(), unstructuredGridRep->getLocalCrs(0));
			vtkOutput->SetPoints(points);
			points = nullptr;

			unstructuredGridRep->loadGeometry();
			if (unstructuredGridRep->isFaceCountOfCellsConstant() && unstructuredGridRep->isNodeCountOfFacesConstant() &&
				unstructuredGridRep->getConstantFaceCountOfCells() == 4 && unstructuredGridRep->getConstantNodeCountOfFaces() == 3) {
				vtkOutput->SetCells(VTK_TETRA, createOutputVtkTetra(unstructuredGridRep));
			}
			else {
				// CELLS
				const unsigned int nodeCount = unstructuredGridRep->getNodeCount();
				std::unique_ptr<vtkIdType[]> pointIds(new vtkIdType[nodeCount]);
				for (unsigned int i = 0; i < nodeCount; ++i) {
					pointIds[i] = i;
				}

				const ULONG64 cellCount = unstructuredGridRep->getCellCount();
				auto initCellIndex = getIdProc() * (cellCount/getMaxProc());
				auto maxCellIndex = (getIdProc()+1) * (cellCount/getMaxProc());

				cout << "unstructuredGrid " << getIdProc() << "-" << getMaxProc() << " : " << initCellIndex << " to " << maxCellIndex << endl;
				for (ULONG64 cellIndex = initCellIndex; cellIndex < maxCellIndex; ++cellIndex) {
					bool isOptimizedCell = false;

					const ULONG64 localFaceCount = unstructuredGridRep->getFaceCountOfCell(cellIndex);
					if (localFaceCount == 4) { // VTK_TETRA
						isOptimizedCell = cellVtkTetra(unstructuredGridRep, cellIndex);
					}
					else if (localFaceCount == 5) { // VTK_WEDGE or VTK_PYRAMID
						isOptimizedCell = cellVtkWedgeOrPyramid(unstructuredGridRep, cellIndex);
					}
					else if (localFaceCount == 6) { // VTK_HEXAHEDRON
						isOptimizedCell = cellVtkHexahedron(unstructuredGridRep, cellIndex);
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
							ULONG64* nodeIndices = unstructuredGridRep->getNodeIndicesOfFaceOfCell(cellIndex, localFaceIndex);
							for (unsigned int i = 0; i < localNodeCount; ++i) {
								nodes[i] = nodeIndices[i];
							}
							faces->InsertNextCell(localNodeCount, nodes.get());
						}
						vtkOutput->InsertNextCell(VTK_POLYHEDRON, nodeCount, pointIds.get(), unstructuredGridRep->getFaceCountOfCell(cellIndex), faces->GetPointer());

						faces = nullptr;
					}

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
vtkSmartPointer<vtkCellArray> VtkUnstructuredGridRepresentation::createOutputVtkTetra(const RESQML2_NS::UnstructuredGridRepresentation* unstructuredGridRep)
{
	vtkSmartPointer<vtkCellArray> cellArray = vtkSmartPointer<vtkCellArray>::New();

	const ULONG64 cellCount = unstructuredGridRep->getCellCount();

	auto initCellIndex = getIdProc() * (cellCount/getMaxProc());
	auto maxCellIndex = (getIdProc()+1) * (cellCount/getMaxProc());

	cout << "unstructuredGrid " << getIdProc() << "-" << getMaxProc() << " : " << initCellIndex << " to " << maxCellIndex << "\n";

	unsigned int nodes[4];
	for (ULONG64 cellIndex = 0; cellIndex < cellCount; ++cellIndex) {

		vtkSmartPointer<vtkTetra> tetra = vtkSmartPointer<vtkTetra>::New();

		// Face 1
		ULONG64* nodeIndices = unstructuredGridRep->getNodeIndicesOfFaceOfCell(cellIndex, 0);
		nodes[0] = nodeIndices[0];
		nodes[1] = nodeIndices[1];
		nodes[2] = nodeIndices[2];

		// Face 2
		nodeIndices = unstructuredGridRep->getNodeIndicesOfFaceOfCell(cellIndex, 1);

		if (!(nodeIndices[0] == nodes[0] || nodeIndices[0] == nodes[1] || nodeIndices[0] == nodes[2]))
			nodes[3] = nodeIndices[0];
		else if (!(nodeIndices[1] == nodes[0] || nodeIndices[1] == nodes[1] || nodeIndices[1] == nodes[2]))
			nodes[3] = nodeIndices[1];
		else if (!(nodeIndices[2] == nodes[0] || nodeIndices[2] == nodes[1] || nodeIndices[2] == nodes[2]))
			nodes[3] = nodeIndices[2];
		else if (!(nodeIndices[3] == nodes[0] || nodeIndices[3] == nodes[1] || nodeIndices[3] == nodes[2]))
			nodes[3] = nodeIndices[3];


		tetra->GetPointIds()->SetId(0, nodes[0]);
		tetra->GetPointIds()->SetId(1, nodes[1]);
		tetra->GetPointIds()->SetId(2, nodes[2]);
		tetra->GetPointIds()->SetId(3, nodes[3]);

		cellArray->InsertNextCell(tetra);
	}
	return cellArray;
}

//----------------------------------------------------------------------------
bool VtkUnstructuredGridRepresentation::cellVtkTetra(const RESQML2_NS::UnstructuredGridRepresentation* unstructuredGridRep, ULONG64 cellIndex) {
	unsigned int faceTo3Nodes = 0;
	ULONG64 nodes[4];
	unsigned int nodeIndex = 0;
	for (ULONG64 localFaceIndex = 0; localFaceIndex < 4; ++localFaceIndex) {
		const unsigned int localNodeCount = unstructuredGridRep->getNodeCountOfFaceOfCell(cellIndex, localFaceIndex);
		if (localNodeCount == 3) {
			ULONG64* nodeIndices = unstructuredGridRep->getNodeIndicesOfFaceOfCell(cellIndex, localFaceIndex);
			for (unsigned int i = 0; i < localNodeCount; ++i) {
				bool alreadyNode = false;
				for (unsigned int j = 0; j < nodeIndex; ++j) {
					if (nodes[j] == nodeIndices[i]) {
						alreadyNode = true;
						break;
					}
				}
				if(!alreadyNode && nodeIndex < 4) {
					nodes[nodeIndex++] = nodeIndices[i];
				}
			}
			++faceTo3Nodes;
		}
	}
	if (faceTo3Nodes != 4) {
		throw std::invalid_argument("The cell index " + std::to_string(cellIndex) + " is malformed : 4 faces but not 3 points per face.");
	}

	vtkSmartPointer<vtkTetra> tetra = vtkSmartPointer<vtkTetra>::New();
	for (int nodesIndex = 0; nodesIndex < 4; ++nodesIndex) {
		tetra->GetPointIds()->SetId(nodesIndex, nodes[nodesIndex]);
	}
	vtkOutput->InsertNextCell(tetra->GetCellType(), tetra->GetPointIds());
	tetra = nullptr;
	return true;
}

//----------------------------------------------------------------------------
bool VtkUnstructuredGridRepresentation::cellVtkWedgeOrPyramid(const RESQML2_NS::UnstructuredGridRepresentation* unstructuredGridRep, ULONG64 cellIndex) {
	unsigned int faceTo4Nodes = 0;
	for (ULONG64 localFaceIndex = 0; localFaceIndex < 5; ++localFaceIndex) {
		const unsigned int localNodeCount = unstructuredGridRep->getNodeCountOfFaceOfCell(cellIndex, localFaceIndex);
		if (localNodeCount == 4) {
			++faceTo4Nodes;
		}
	}
	if (faceTo4Nodes == 3) { // VTK_WEDGE
		ULONG64 nodes[6];
		unsigned int faceTo3Nodes = 0;
		for (ULONG64 localFaceIndex = 0; localFaceIndex < 5; ++localFaceIndex) {
			const unsigned int localNodeCount = unstructuredGridRep->getNodeCountOfFaceOfCell(cellIndex, localFaceIndex);
			if (localNodeCount == 3) {
				ULONG64* nodeIndices = unstructuredGridRep->getNodeIndicesOfFaceOfCell(cellIndex, localFaceIndex);
				for (unsigned int i = 0; i < localNodeCount; ++i) {
					nodes[faceTo3Nodes*3+i] = nodeIndices[i];
				}
				++faceTo3Nodes;
			}
		}

		vtkSmartPointer<vtkWedge> wedge = vtkSmartPointer<vtkWedge>::New();
		for (int nodesIndex = 0; nodesIndex < 6; ++nodesIndex) {
			wedge->GetPointIds()->SetId(nodesIndex, nodes[nodesIndex]);
		}
		vtkOutput->InsertNextCell(wedge->GetCellType(), wedge->GetPointIds());
		wedge = nullptr;
	}
	else if (faceTo4Nodes == 1) { // VTK_PYRAMID
		ULONG64 nodes[6];
		unsigned int nodeIndex = 0;
		unsigned int faceTo3Nodes = 0;
		for (ULONG64 localFaceIndex = 0; localFaceIndex < 5; ++localFaceIndex) {
			const unsigned int localNodeCount = unstructuredGridRep->getNodeCountOfFaceOfCell(cellIndex, localFaceIndex);
			if (localNodeCount == 3) {
				ULONG64* nodeIndices = unstructuredGridRep->getNodeIndicesOfFaceOfCell(cellIndex, localFaceIndex);
				for (unsigned int i = 0; i < localNodeCount; ++i) {
					bool alreadyNode = false;
					for (unsigned int j = 0; j < nodeIndex; ++j) {
						if (nodes[j] == nodeIndices[i]) {
							alreadyNode = true;
							break;
						}
					}
					if(!alreadyNode && nodeIndex < 5) {
						nodes[nodeIndex++] = nodeIndices[i];
					}
				}
				++faceTo3Nodes;
			}
		}

		vtkSmartPointer<vtkPyramid> pyramid = vtkSmartPointer<vtkPyramid>::New();
		for (int nodesIndex = 0; nodesIndex < 5; ++nodesIndex) {
			pyramid->GetPointIds()->SetId(nodesIndex, nodes[nodesIndex]);
		}
		vtkOutput->InsertNextCell(pyramid->GetCellType(), pyramid->GetPointIds());
		pyramid = nullptr;
	}
	else {
		throw std::invalid_argument("The cell index " + std::to_string(cellIndex) + " is malformed : 5 faces but not a pyramid, not a wedge.");
	}

	return true;
}

//----------------------------------------------------------------------------
bool VtkUnstructuredGridRepresentation::cellVtkHexahedron(const RESQML2_NS::UnstructuredGridRepresentation* unstructuredGridRep, ULONG64 cellIndex) {
	unsigned int faceTo4Nodes = 0;
	ULONG64 nodes[8];
	unsigned int nodeIndex = 0;
	for (ULONG64 localFaceIndex = 0; localFaceIndex < 6; ++localFaceIndex) {
		const unsigned int localNodeCount = unstructuredGridRep->getNodeCountOfFaceOfCell(cellIndex, localFaceIndex);
		if (localNodeCount == 4) {
			ULONG64* nodeIndices = unstructuredGridRep->getNodeIndicesOfFaceOfCell(cellIndex, localFaceIndex);
			for (unsigned int i = 0; i < localNodeCount; ++i) {
				bool alreadyNode = false;
				for (unsigned int j = 0; j < nodeIndex; ++j) {
					if (nodes[j] == nodeIndices[i]) {
						alreadyNode = true;
						break;
					}
				}
				if(!alreadyNode && nodeIndex < 8) {
					nodes[++nodeIndex] = nodeIndices[i];
				}
			}
			++faceTo4Nodes;
		}
	}

	if (faceTo4Nodes == 6) {
		vtkSmartPointer<vtkHexahedron> hexahedron = vtkSmartPointer<vtkHexahedron>::New();
		for (int nodesIndex = 0; nodesIndex < 8; ++nodesIndex) {
			hexahedron->GetPointIds()->SetId(nodesIndex, nodes[nodesIndex]);
		}
		vtkOutput->InsertNextCell(hexahedron->GetCellType(), hexahedron->GetPointIds());
		hexahedron = nullptr;
		return true;
	}

	return false;
}

//----------------------------------------------------------------------------
bool VtkUnstructuredGridRepresentation::cellVtkPentagonalPrism(const RESQML2_NS::UnstructuredGridRepresentation* unstructuredGridRep, ULONG64 cellIndex) {
	unsigned int faceTo5Nodes = 0;
	ULONG64 nodes[10];
	for (ULONG64 localFaceIndex = 0; localFaceIndex < 7; ++localFaceIndex) {
		const unsigned int localNodeCount = unstructuredGridRep->getNodeCountOfFaceOfCell(cellIndex, localFaceIndex);
		if (localNodeCount == 5) {
			ULONG64* nodeIndices = unstructuredGridRep->getNodeIndicesOfFaceOfCell(cellIndex, localFaceIndex);
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
			ULONG64* nodeIndices = unstructuredGridRep->getNodeIndicesOfFaceOfCell(cellIndex, localFaceIndex);
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
	lastProperty = uuidProperty;
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
