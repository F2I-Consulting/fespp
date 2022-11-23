﻿/*-----------------------------------------------------------------------
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
#include "ResqmlMapping/ResqmlUnstructuredGridSubRepToVtkUnstructuredGrid.h"

#include <array>

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
#include <vtkUnstructuredGrid.h>
#include <vtkPolyData.h>
#include <vtkPolygon.h>
#include <vtkTriangle.h>
#include <vtkQuad.h>

// FESAPI
#include <fesapi/resqml2/UnstructuredGridRepresentation.h>
#include <fesapi/resqml2/SubRepresentation.h>
#include <fesapi/resqml2/AbstractLocal3dCrs.h>

// FESPP
#include "ResqmlAbstractRepresentationToVtkPartitionedDataSet.h"
#include "ResqmlUnstructuredGridToVtkUnstructuredGrid.h"

//----------------------------------------------------------------------------
ResqmlUnstructuredGridSubRepToVtkUnstructuredGrid::ResqmlUnstructuredGridSubRepToVtkUnstructuredGrid(RESQML2_NS::SubRepresentation *subRep, ResqmlUnstructuredGridToVtkUnstructuredGrid *support, int proc_number, int max_proc)
	: ResqmlAbstractRepresentationToVtkPartitionedDataSet(subRep,
														  proc_number,
														  max_proc),
	  resqmlData(subRep),
	  mapperUnstructuredGrid(support)
{
	this->iCellCount = subRep->getElementCountOfPatch(0);
	this->pointCount = subRep->getSupportingRepresentation(0)->getXyzPointCountOfAllPatches();

	this->vtkData = vtkSmartPointer<vtkPartitionedDataSet>::New();
}

//----------------------------------------------------------------------------
void ResqmlUnstructuredGridSubRepToVtkUnstructuredGrid::loadVtkObject()
{
	if (this->resqmlData->areElementIndicesPairwise(0))
	{
		vtkOutputWindowDisplayErrorText(("not supported: the element indices of a particular patch are pairwise for SubRepresentation " + this->resqmlData->getUuid()).c_str());
	}
	else
	{
		auto *supportingGrid = dynamic_cast<RESQML2_NS::UnstructuredGridRepresentation *>(this->resqmlData->getSupportingRepresentation(0));
		if (supportingGrid != nullptr)
		{
			if (this->resqmlData->getElementKindOfPatch(0, 0) == gsoap_eml2_3::resqml22__IndexableElement::cells)
			{
				vtkSmartPointer<vtkUnstructuredGrid> vtk_unstructuredGrid = vtkSmartPointer<vtkUnstructuredGrid>::New();
				vtk_unstructuredGrid->Allocate(this->resqmlData->getElementCountOfPatch(0));

				vtk_unstructuredGrid->SetPoints(this->getMapperVtkPoint());

				supportingGrid->loadGeometry();
				// CELLS
				const unsigned int nodeCount = supportingGrid->getNodeCount();
				std::unique_ptr<vtkIdType[]> pointIds(new vtkIdType[nodeCount]);
				for (unsigned int i = 0; i < nodeCount; ++i)
				{
					pointIds[i] = i;
				}

				const ULONG64 cellCount = this->resqmlData->getElementCountOfPatch(0);
				ULONG64 const *cumulativeFaceCountPerCell = supportingGrid->isFaceCountOfCellsConstant()
																? nullptr
																: supportingGrid->getCumulativeFaceCountPerCell(); // This pointer is owned and managed by FESAPI
				std::unique_ptr<unsigned char[]> cellFaceNormalOutwardlyDirected(new unsigned char[cumulativeFaceCountPerCell == nullptr
																									   ? supportingGrid->getCellCount() * supportingGrid->getConstantFaceCountOfCells()
																									   : cumulativeFaceCountPerCell[supportingGrid->getCellCount() - 1]]);

				supportingGrid->getCellFaceIsRightHanded(cellFaceNormalOutwardlyDirected.get());
				auto maxCellIndex = (this->procNumber + 1) * cellCount / this->maxProc;

				std::unique_ptr<uint64_t[]> elementIndices(new uint64_t[cellCount]);
				resqmlData->getElementIndicesOfPatch(0, 0, elementIndices.get());

				for (ULONG64 cellIndex = this->procNumber * cellCount / this->maxProc; cellIndex < maxCellIndex; ++cellIndex)
				{
					bool isOptimizedCell = false;

					const ULONG64 localFaceCount = supportingGrid->getFaceCountOfCell(elementIndices[cellIndex]);

					// Following https://kitware.github.io/vtk-examples/site/VTKBook/05Chapter5/#Figure%205-2
					if (localFaceCount == 4)
					{ // VTK_TETRA
						this->mapperUnstructuredGrid->cellVtkTetra(vtk_unstructuredGrid, cumulativeFaceCountPerCell, cellFaceNormalOutwardlyDirected.get(), elementIndices[cellIndex]);
						isOptimizedCell = true;
					}
					else if (localFaceCount == 5)
					{ // VTK_WEDGE or VTK_PYRAMID
						this->mapperUnstructuredGrid->cellVtkWedgeOrPyramid(vtk_unstructuredGrid, cumulativeFaceCountPerCell, cellFaceNormalOutwardlyDirected.get(), elementIndices[cellIndex]);
						isOptimizedCell = true;
					}
					else if (localFaceCount == 6)
					{ // VTK_HEXAHEDRON
						isOptimizedCell = this->mapperUnstructuredGrid->cellVtkHexahedron(vtk_unstructuredGrid, cumulativeFaceCountPerCell, cellFaceNormalOutwardlyDirected.get(), elementIndices[cellIndex]);
					}
					else if (localFaceCount == 7)
					{ // VTK_PENTAGONAL_PRISM
						isOptimizedCell = this->mapperUnstructuredGrid->cellVtkPentagonalPrism(vtk_unstructuredGrid, elementIndices[cellIndex]);
					}
					else if (localFaceCount == 8)
					{ // VTK_HEXAGONAL_PRISM
						isOptimizedCell = this->mapperUnstructuredGrid->cellVtkHexagonalPrism(vtk_unstructuredGrid, elementIndices[cellIndex]);
					}

					if (!isOptimizedCell)
					{
						vtkSmartPointer<vtkIdList> idList = vtkSmartPointer<vtkIdList>::New();

						// For polyhedron cell, a special ptIds input format is required : (numCellFaces, numFace0Pts, id1, id2, id3, numFace1Pts, id1, id2, id3, ...)
						idList->InsertNextId(localFaceCount);
						for (ULONG64 localFaceIndex = 0; localFaceIndex < localFaceCount; ++localFaceIndex)
						{
							const unsigned int localNodeCount = supportingGrid->getNodeCountOfFaceOfCell(elementIndices[cellIndex], localFaceIndex);
							idList->InsertNextId(localNodeCount);
							ULONG64 const *nodeIndices = supportingGrid->getNodeIndicesOfFaceOfCell(elementIndices[cellIndex], localFaceIndex);
							for (unsigned int i = 0; i < localNodeCount; ++i)
							{
								idList->InsertNextId(nodeIndices[i]);
							}
						}

						vtk_unstructuredGrid->InsertNextCell(VTK_POLYHEDRON, idList);
					}
				}

				supportingGrid->unloadGeometry();

				this->vtkData->SetPartition(0, vtk_unstructuredGrid);
				this->vtkData->Modified();
			}
			else if (this->resqmlData->getElementKindOfPatch(0, 0) == gsoap_eml2_3::resqml22__IndexableElement::faces)
			{
				vtkSmartPointer<vtkPolyData> vtk_polydata = vtkSmartPointer<vtkPolyData>::New();
				vtk_polydata->Allocate(this->resqmlData->getElementCountOfPatch(0));

				vtk_polydata->SetPoints(this->getMapperVtkPoint());

				supportingGrid->loadGeometry();

				// FACES
				const unsigned int nodeCount = supportingGrid->getNodeCount();
				std::unique_ptr<vtkIdType[]> pointIds(new vtkIdType[nodeCount]);
				for (unsigned int i = 0; i < nodeCount; ++i)
				{
					pointIds[i] = i;
				}

				const ULONG64 gridFaceCount = supportingGrid->getFaceCount();
					std::unique_ptr<uint64_t[]> nodeCountOfFaces(new uint64_t[gridFaceCount]);
					supportingGrid->getCumulativeNodeCountPerFace(nodeCountOfFaces.get());
				
					std::unique_ptr<uint64_t[]> nodeIndices(new uint64_t[nodeCountOfFaces[gridFaceCount-1]]);
					supportingGrid->getNodeIndicesOfFaces(nodeIndices.get());

					const ULONG64 subFaceCount = this->resqmlData->getElementCountOfPatch(0);
				std::unique_ptr<uint64_t[]> elementIndices(new uint64_t[subFaceCount]);
				resqmlData->getElementIndicesOfPatch(0, 0, elementIndices.get());

				vtkSmartPointer<vtkCellArray> polys = vtkSmartPointer<vtkCellArray>::New();
				for (ULONG64 subFaceIndex =0; subFaceIndex < subFaceCount; ++subFaceIndex)
				{
					ULONG64 faceIndex = elementIndices[subFaceIndex];
					auto indiceValue = nodeCountOfFaces[faceIndex];
					auto first_indiceValue = faceIndex == 0 ? 0 : nodeCountOfFaces[faceIndex - 1];
					ULONG64 nodeCount_OfFaceIndex = indiceValue - first_indiceValue;

					if (nodeCount_OfFaceIndex == 3) // vtkTriangle
					{
						vtkSmartPointer<vtkTriangle> triangle = vtkSmartPointer<vtkTriangle>::New();
						auto indice1 = nodeIndices[first_indiceValue];
						auto indice2 = nodeIndices[first_indiceValue +1];
						auto indice3 = nodeIndices[first_indiceValue +2];
						triangle->GetPointIds()->SetId(0, indice1);
						triangle->GetPointIds()->SetId(1, indice2);
						triangle->GetPointIds()->SetId(2, indice3);
						polys->InsertNextCell(triangle);
					}
					else if (nodeCount_OfFaceIndex == 4) // vtkQuad
					{
						vtkSmartPointer<vtkQuad> quad = vtkSmartPointer<vtkQuad>::New();
						auto indice1 = nodeIndices[first_indiceValue];
						auto indice2 = nodeIndices[first_indiceValue + 1];
						auto indice3 = nodeIndices[first_indiceValue + 2];
						auto indice4 = nodeIndices[first_indiceValue + 3];
						quad->GetPointIds()->SetId(0, indice1);
						quad->GetPointIds()->SetId(1, indice2);
						quad->GetPointIds()->SetId(2, indice3);
						quad->GetPointIds()->SetId(3, indice4);
						polys->InsertNextCell(quad);
					}
					else
					{
						vtkSmartPointer<vtkPolygon> polygon = vtkSmartPointer<vtkPolygon>::New();
						for (ULONG64 nodeIndex = 0; nodeIndex < nodeCount_OfFaceIndex; ++nodeIndex)
						{
							auto index = first_indiceValue + nodeIndex;
							polygon->GetPointIds()->SetId(nodeIndex, nodeIndices[index]);
						}
						polys->InsertNextCell(polygon);
					}
				}

				vtk_polydata->SetPolys(polys);

				supportingGrid->unloadGeometry();

				this->vtkData->SetPartition(0, vtk_polydata);
				this->vtkData->Modified();
			}
			else
			{
				vtkOutputWindowDisplayErrorText(("not supported: SubRepresentation (" + this->resqmlData->getUuid() + ") with indexable element different from cell or nodes").c_str());
			}
		}
		else
		{
			// TODO msg d'erreur
		}
	}
}

//----------------------------------------------------------------------------
std::string ResqmlUnstructuredGridSubRepToVtkUnstructuredGrid::unregisterToMapperSupportingGrid()
{
	this->mapperUnstructuredGrid->unregisterSubRep();
	return this->mapperUnstructuredGrid->getUuid();
}

//----------------------------------------------------------------------------
vtkSmartPointer<vtkPoints> ResqmlUnstructuredGridSubRepToVtkUnstructuredGrid::getMapperVtkPoint()
{
	this->mapperUnstructuredGrid->registerSubRep();
	return this->mapperUnstructuredGrid->getVtkPoints();
}

