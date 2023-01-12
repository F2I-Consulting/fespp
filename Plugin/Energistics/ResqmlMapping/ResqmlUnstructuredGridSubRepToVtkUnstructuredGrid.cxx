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
	  mapperUnstructuredGrid(support)
{
	this->iCellCount = subRep->getElementCountOfPatch(0);
	this->pointCount = subRep->getSupportingRepresentation(0)->getXyzPointCountOfAllPatches();

	this->vtkData = vtkSmartPointer<vtkPartitionedDataSet>::New();
}

//----------------------------------------------------------------------------
RESQML2_NS::SubRepresentation const* ResqmlUnstructuredGridSubRepToVtkUnstructuredGrid::getResqmlData() const
{
	return static_cast<RESQML2_NS::SubRepresentation const*>(resqmlData);
}

//----------------------------------------------------------------------------
void ResqmlUnstructuredGridSubRepToVtkUnstructuredGrid::loadVtkObject()
{
	RESQML2_NS::SubRepresentation const* subRep = getResqmlData();

	if (subRep->areElementIndicesPairwise(0))
	{
		vtkOutputWindowDisplayErrorText(("not supported: the element indices of a particular patch are pairwise for SubRepresentation " + subRep->getUuid()).c_str());
	}
	else
	{
		auto *supportingGrid = dynamic_cast<RESQML2_NS::UnstructuredGridRepresentation *>(subRep->getSupportingRepresentation(0));
		if (supportingGrid != nullptr)
		{
			gsoap_eml2_3::resqml22__IndexableElement indexable_element = subRep->getElementKindOfPatch(0, 0);
			if (indexable_element == gsoap_eml2_3::resqml22__IndexableElement::cells)
			{
				vtkSmartPointer<vtkUnstructuredGrid> vtk_unstructuredGrid = vtkSmartPointer<vtkUnstructuredGrid>::New();
				vtk_unstructuredGrid->Allocate(subRep->getElementCountOfPatch(0));
				vtk_unstructuredGrid->SetPoints(this->getMapperVtkPoint());

				supportingGrid->loadGeometry();
				// CELLS
				const ULONG64 cellCount = subRep->getElementCountOfPatch(0);
				ULONG64 const *cumulativeFaceCountPerCell = supportingGrid->isFaceCountOfCellsConstant()
																? nullptr
																: supportingGrid->getCumulativeFaceCountPerCell(); // This pointer is owned and managed by FESAPI
				std::unique_ptr<unsigned char[]> cellFaceNormalOutwardlyDirected(new unsigned char[cumulativeFaceCountPerCell == nullptr
																									   ? supportingGrid->getCellCount() * supportingGrid->getConstantFaceCountOfCells()
																									   : cumulativeFaceCountPerCell[supportingGrid->getCellCount() - 1]]);

				supportingGrid->getCellFaceIsRightHanded(cellFaceNormalOutwardlyDirected.get());
				auto maxCellIndex = (this->procNumber + 1) * cellCount / this->maxProc;

				std::unique_ptr<uint64_t[]> elementIndices(new uint64_t[cellCount]);
				subRep->getElementIndicesOfPatch(0, 0, elementIndices.get());

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
			else if (indexable_element == gsoap_eml2_3::resqml22__IndexableElement::faces)
			{
				vtkSmartPointer<vtkPolyData> vtk_polydata = vtkSmartPointer<vtkPolyData>::New();
				vtk_polydata->SetPoints(this->getMapperVtkPoint());

				// FACES
				const ULONG64 gridFaceCount = supportingGrid->getFaceCount();
				std::unique_ptr<uint64_t[]> nodeCountOfFaces(new uint64_t[gridFaceCount]);
				supportingGrid->getCumulativeNodeCountPerFace(nodeCountOfFaces.get());
				
				std::unique_ptr<uint64_t[]> nodeIndices(new uint64_t[nodeCountOfFaces[gridFaceCount-1]]);
				supportingGrid->getNodeIndicesOfFaces(nodeIndices.get());

				const ULONG64 subFaceCount = subRep->getElementCountOfPatch(0);
				std::unique_ptr<uint64_t[]> elementIndices(new uint64_t[subFaceCount]);
				subRep->getElementIndicesOfPatch(0, 0, elementIndices.get());

				vtkSmartPointer<vtkCellArray> polys = vtkSmartPointer<vtkCellArray>::New();
				polys->AllocateEstimate(subFaceCount, 4);

				for (ULONG64 subFaceIndex =0; subFaceIndex < subFaceCount; ++subFaceIndex)
				{
					ULONG64 faceIndex = elementIndices[subFaceIndex];
					auto first_indiceValue = faceIndex == 0 ? 0 : nodeCountOfFaces[faceIndex - 1];
					ULONG64 nodeCount_OfFaceIndex = nodeCountOfFaces[faceIndex] - first_indiceValue;

					vtkSmartPointer<vtkIdList> nodes = vtkSmartPointer<vtkIdList>::New();
					for (ULONG64 nodeIndex = 0; nodeIndex < nodeCount_OfFaceIndex; ++nodeIndex, ++first_indiceValue)
					{
						nodes->InsertId(nodeIndex, nodeIndices[first_indiceValue]);
					}
					polys->InsertNextCell(nodes);
				}

				vtk_polydata->SetPolys(polys);

				this->vtkData->SetPartition(0, vtk_polydata);
				this->vtkData->Modified();
			}
			else
			{
				vtkOutputWindowDisplayErrorText(("not supported: SubRepresentation (" + subRep->getUuid() + ") with indexable element different from cell or nodes").c_str());
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

