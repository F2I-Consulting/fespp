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
#include "Mapping/ResqmlUnstructuredGridSubRepToVtkUnstructuredGrid.h"

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
ResqmlUnstructuredGridSubRepToVtkUnstructuredGrid::ResqmlUnstructuredGridSubRepToVtkUnstructuredGrid(const RESQML2_NS::SubRepresentation *subRep, ResqmlUnstructuredGridToVtkUnstructuredGrid *support, uint32_t p_procNumber, uint32_t p_maxProc)
	: ResqmlAbstractRepresentationToVtkPartitionedDataSet(subRep,
		p_procNumber,
		p_maxProc),
	mapperUnstructuredGrid(support)
{
	_iCellCount = subRep->getElementCountOfPatch(0);
	_pointCount = subRep->getSupportingRepresentation(0)->getXyzPointCountOfAllPatches();

	_vtkData = vtkSmartPointer<vtkPartitionedDataSet>::New();
}

//----------------------------------------------------------------------------
const RESQML2_NS::SubRepresentation* ResqmlUnstructuredGridSubRepToVtkUnstructuredGrid::getResqmlData() const
{
	return static_cast<const RESQML2_NS::SubRepresentation*>(_resqmlData);
}

//----------------------------------------------------------------------------
void ResqmlUnstructuredGridSubRepToVtkUnstructuredGrid::loadVtkObject()
{
	RESQML2_NS::SubRepresentation const* subRep = getResqmlData();

	if (subRep->areElementIndicesPairwise(0))
	{
		vtkOutputWindowDisplayErrorText(("not supported: the element indices of a particular patch are pairwise for SubRepresentation " + subRep->getUuid()).c_str());
		return;
	}
	auto* supportingGrid = dynamic_cast<RESQML2_NS::UnstructuredGridRepresentation*>(subRep->getSupportingRepresentation(0));
	if (supportingGrid == nullptr)
	{
		vtkOutputWindowDisplayWarningText(("SubRepresentation (" + subRep->getUuid() + ") has no support grid").c_str());
		return;
	}

	gsoap_eml2_3::eml23__IndexableElement indexable_element = subRep->getElementKindOfPatch(0, 0);
	if (indexable_element == gsoap_eml2_3::eml23__IndexableElement::cells)
	{
    vtkSmartPointer<vtkUnstructuredGrid> vtk_unstructuredGrid = vtkSmartPointer<vtkUnstructuredGrid>::New();
		vtk_unstructuredGrid->Allocate(subRep->getElementCountOfPatch(0));
		vtk_unstructuredGrid->SetPoints(this->getMapperVtkPoint());

		supportingGrid->loadGeometry();
		// CELLS
		const uint64_t cellCount = subRep->getElementCountOfPatch(0);
		uint64_t const *cumulativeFaceCountPerCell = supportingGrid->isFaceCountOfCellsConstant()
						? nullptr
						: supportingGrid->getCumulativeFaceCountPerCell(); // This pointer is owned and managed by FESAPI
		std::unique_ptr<unsigned char[]> cellFaceNormalOutwardlyDirected(new unsigned char[cumulativeFaceCountPerCell == nullptr
				   ? supportingGrid->getCellCount() * supportingGrid->getConstantFaceCountOfCells()
				   : cumulativeFaceCountPerCell[supportingGrid->getCellCount() - 1]]);

		supportingGrid->getCellFaceIsRightHanded(cellFaceNormalOutwardlyDirected.get());
		auto maxCellIndex = (_procNumber + 1) * cellCount / _maxProc;

		std::unique_ptr<uint64_t[]> elementIndices(new uint64_t[cellCount]);
		subRep->getElementIndicesOfPatch(0, 0, elementIndices.get());

		for (uint64_t cellIndex = _procNumber * cellCount / _maxProc; cellIndex < maxCellIndex; ++cellIndex)
		{
			bool isOptimizedCell = false;

			const uint64_t localFaceCount = supportingGrid->getFaceCountOfCell(elementIndices[cellIndex]);

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
				isOptimizedCell = this->mapperUnstructuredGrid->cellVtkPentagonalPrism(vtk_unstructuredGrid, cumulativeFaceCountPerCell, cellFaceNormalOutwardlyDirected.get(), elementIndices[cellIndex]);
			}
			else if (localFaceCount == 8)
			{ // VTK_HEXAGONAL_PRISM
				isOptimizedCell = this->mapperUnstructuredGrid->cellVtkHexagonalPrism(vtk_unstructuredGrid, cumulativeFaceCountPerCell, cellFaceNormalOutwardlyDirected.get(), elementIndices[cellIndex]);
			}

			if (!isOptimizedCell)
			{
        vtkSmartPointer<vtkIdList> idList = vtkSmartPointer<vtkIdList>::New();

				// For polyhedron cell, a special ptIds input format is required : (numCellFaces, numFace0Pts, id1, id2, id3, numFace1Pts, id1, id2, id3, ...)
				idList->InsertNextId(localFaceCount);
				for (uint64_t localFaceIndex = 0; localFaceIndex < localFaceCount; ++localFaceIndex)
				{
					const unsigned int localNodeCount = supportingGrid->getNodeCountOfFaceOfCell(elementIndices[cellIndex], localFaceIndex);
					idList->InsertNextId(localNodeCount);
					uint64_t const *nodeIndices = supportingGrid->getNodeIndicesOfFaceOfCell(elementIndices[cellIndex], localFaceIndex);
					for (unsigned int i = 0; i < localNodeCount; ++i)
					{
						idList->InsertNextId(nodeIndices[i]);
					}
				}

				vtk_unstructuredGrid->InsertNextCell(VTK_POLYHEDRON, idList);
			}
		}
		supportingGrid->unloadGeometry();

		_vtkData->SetPartition(0, vtk_unstructuredGrid);
		_vtkData->Modified();
	}
	else if (indexable_element == gsoap_eml2_3::eml23__IndexableElement::faces)
	{
		vtkSmartPointer<vtkPolyData> vtk_polydata = vtkSmartPointer<vtkPolyData>::New();
		vtk_polydata->SetPoints(this->getMapperVtkPoint());

    // FACES
		const uint64_t gridFaceCount = supportingGrid->getFaceCount();
		std::unique_ptr<uint64_t[]> nodeCountOfFaces(new uint64_t[gridFaceCount]);
		supportingGrid->getCumulativeNodeCountPerFace(nodeCountOfFaces.get());
    
    std::unique_ptr<uint64_t[]> nodeIndices(new uint64_t[nodeCountOfFaces[gridFaceCount - 1]]);
		supportingGrid->getNodeIndicesOfFaces(nodeIndices.get());
    
		const uint64_t subFaceCount = subRep->getElementCountOfPatch(0);
		std::unique_ptr<uint64_t[]> elementIndices(new uint64_t[subFaceCount]);
		subRep->getElementIndicesOfPatch(0, 0, elementIndices.get());

    vtkSmartPointer<vtkCellArray> polys = vtkSmartPointer<vtkCellArray>::New();
		polys->AllocateEstimate(subFaceCount, 4);

		for (uint64_t subFaceIndex = 0; subFaceIndex < subFaceCount; ++subFaceIndex)
		{
			uint64_t faceIndex = elementIndices[subFaceIndex];
			auto first_indiceValue = faceIndex == 0 ? 0 : nodeCountOfFaces[faceIndex - 1];
			uint64_t nodeCount_OfFaceIndex = nodeCountOfFaces[faceIndex] - first_indiceValue;

      vtkSmartPointer<vtkIdList> nodes = vtkSmartPointer<vtkIdList>::New();
			for (uint64_t nodeIndex = 0; nodeIndex < nodeCount_OfFaceIndex; ++nodeIndex, ++first_indiceValue)
			{
				nodes->InsertId(nodeIndex, nodeIndices[first_indiceValue]);
			}
			polys->InsertNextCell(nodes);
		}

		vtk_polydata->SetPolys(polys);

		_vtkData->SetPartition(0, vtk_polydata);
		_vtkData->Modified();
	}
	else
	{
		vtkOutputWindowDisplayWarningText(("not supported: SubRepresentation (" + subRep->getUuid() + ") with indexable element different from cell or faces\n").c_str());
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
