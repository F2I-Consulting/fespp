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
#include "Mapping/ResqmlBlockedWellboreToVtkUnstructuredGrid.h"

#include <algorithm>
#include <array>
#include <memory>
#include <vector>

// include VTK library
#include <vtkSmartPointer.h>
#include <vtkCellType.h>
#include <vtkIdList.h>
#include <vtkUnstructuredGrid.h>

// include FESAPI
#include <fesapi/resqml2/BlockedWellboreRepresentation.h>
#include <fesapi/resqml2/AbstractIjkGridRepresentation.h>
#include <fesapi/resqml2/UnstructuredGridRepresentation.h>

// include FESPP
#include "ResqmlIjkGridToVtkExplicitStructuredGrid.h"
#include "ResqmlUnstructuredGridToVtkUnstructuredGrid.h"

//----------------------------------------------------------------------------
ResqmlBlockedWellboreToVtkUnstructuredGrid::ResqmlBlockedWellboreToVtkUnstructuredGrid(const RESQML2_NS::BlockedWellboreRepresentation* p_blockedWellbore, ResqmlAbstractRepresentationToVtkPartitionedDataSet* p_supportGridMapper, uint32_t p_procNumber, uint32_t p_maxProc)

	: ResqmlAbstractRepresentationToVtkPartitionedDataSet(p_blockedWellbore,
		p_procNumber,
		p_maxProc),
	mapperSupportingGrid(p_supportGridMapper)
{
	_iCellCount = p_blockedWellbore->getCellCount();

	_vtkData = vtkSmartPointer<vtkPartitionedDataSet>::New();
}

//----------------------------------------------------------------------------
const RESQML2_NS::BlockedWellboreRepresentation* ResqmlBlockedWellboreToVtkUnstructuredGrid::getResqmlData() const
{
	return static_cast<const RESQML2_NS::BlockedWellboreRepresentation*>(_resqmlData);
}

//----------------------------------------------------------------------------
void ResqmlBlockedWellboreToVtkUnstructuredGrid::loadVtkObject()
{
	RESQML2_NS::BlockedWellboreRepresentation const* w_blockedWellbore = getResqmlData();
	RESQML2_NS::AbstractGridRepresentation* w_grid = w_blockedWellbore->getSupportingGridRepresentation(0);
	if (w_grid == nullptr)
	{
		vtkOutputWindowDisplayWarningText(("BlockedWellbore (" + w_blockedWellbore->getUuid() + ") has no supporting grid\n").c_str());
		return;
	}

	// Collect the flat indices of the blocked cells. FESAPI fills a per-interval
	// array (size = getMdValuesCount(), null entries = interval crosses no cell);
	// we keep the getCellCount() non-null ones, then sort ascending so the
	// flat-order cell scan below matches them in order (same trick as the SubRep
	// mappers, whose element indices are already ascending).
	const uint64_t w_mdCount = w_blockedWellbore->getMdValuesCount();
	const uint64_t w_cellCount = w_blockedWellbore->getCellCount();
	std::vector<int64_t> w_rawCells(w_mdCount);
	const int64_t w_cellNull = w_blockedWellbore->getCellIndices(w_rawCells.data());
	std::vector<int64_t> w_blockedCells;
	w_blockedCells.reserve(w_cellCount);
	for (uint64_t i = 0; i < w_mdCount && w_blockedCells.size() < w_cellCount; ++i)
	{
		if (w_rawCells[i] != w_cellNull)
		{
			w_blockedCells.push_back(w_rawCells[i]);
		}
	}
	std::sort(w_blockedCells.begin(), w_blockedCells.end());
	if (w_blockedCells.empty())
	{
		return;
	}

	vtkSmartPointer<vtkUnstructuredGrid> w_vtkUnstructuredGrid = vtkSmartPointer<vtkUnstructuredGrid>::New();

	// --- IjkGrid support: build a hexahedron per blocked (i,j,k) cell, reusing
	// the supporting grid's points (cf. ResqmlIjkGridSubRepToVtkExplicitStructuredGrid).
	if (auto* w_ijkGrid = dynamic_cast<RESQML2_NS::AbstractIjkGridRepresentation*>(w_grid); w_ijkGrid != nullptr)
	{
		auto* w_ijkMapper = dynamic_cast<ResqmlIjkGridToVtkExplicitStructuredGrid*>(mapperSupportingGrid);
		if (w_ijkMapper == nullptr)
		{
			vtkOutputWindowDisplayWarningText(("BlockedWellbore (" + w_blockedWellbore->getUuid() + "): supporting IjkGrid mapper missing\n").c_str());
			return;
		}
		mapperSupportingGrid->registerSubRep();
		// One hexahedron per blocked cell, 8 ids each — exact one-shot allocation.
		w_vtkUnstructuredGrid->AllocateExact(static_cast<vtkIdType>(w_blockedCells.size()),
			static_cast<vtkIdType>(w_blockedCells.size()) * 8);
		w_vtkUnstructuredGrid->SetPoints(w_ijkMapper->getVtkPoints());

		// hexahedron node ordering per ParaView convention (handedness-aware)
		std::array<unsigned int, 8> w_corner = { 0, 1, 2, 3, 4, 5, 6, 7 };
		if (w_ijkGrid->isRightHanded())
		{
			w_corner = { 4, 5, 6, 7, 0, 1, 2, 3 };
		}

		w_ijkGrid->loadSplitInformation();
		_iCellCount = w_ijkGrid->getICellCount();
		_jCellCount = w_ijkGrid->getJCellCount();
		_kCellCount = w_ijkGrid->getKCellCount();

		uint64_t w_cellIndex = 0;
		size_t w_indice = 0;
		for (uint_fast32_t w_k = 0; w_k < _kCellCount; ++w_k)
		{
			for (uint_fast32_t w_j = 0; w_j < _jCellCount; ++w_j)
			{
				for (uint_fast32_t w_i = 0; w_i < _iCellCount; ++w_i)
				{
					if (w_indice < w_blockedCells.size() && w_blockedCells[w_indice] == static_cast<int64_t>(w_cellIndex))
					{
						vtkIdType w_hexPointIds[8];
						for (uint_fast8_t w_c = 0; w_c < 8; ++w_c)
						{
							w_hexPointIds[w_c] = static_cast<vtkIdType>(
								w_ijkGrid->getXyzPointIndexFromCellCorner(w_i, w_j, w_k, w_corner[w_c]));
						}
						w_vtkUnstructuredGrid->InsertNextCell(VTK_HEXAHEDRON, 8, w_hexPointIds);
						++w_indice;
					}
					++w_cellIndex;
				}
			}
		}
		w_ijkGrid->unloadSplitInformation();
	}
	// --- UnstructuredGrid support: build each blocked cell from the supporting
	// grid's face topology (cf. ResqmlUnstructuredGridSubRepToVtkUnstructuredGrid).
	else if (auto* w_unstructuredGrid = dynamic_cast<RESQML2_NS::UnstructuredGridRepresentation*>(w_grid); w_unstructuredGrid != nullptr)
	{
		auto* w_unstrMapper = dynamic_cast<ResqmlUnstructuredGridToVtkUnstructuredGrid*>(mapperSupportingGrid);
		if (w_unstrMapper == nullptr)
		{
			vtkOutputWindowDisplayWarningText(("BlockedWellbore (" + w_blockedWellbore->getUuid() + "): supporting UnstructuredGrid mapper missing\n").c_str());
			return;
		}
		mapperSupportingGrid->registerSubRep();
		w_vtkUnstructuredGrid->Allocate(static_cast<vtkIdType>(w_blockedCells.size()));
		w_vtkUnstructuredGrid->SetPoints(w_unstrMapper->getVtkPoints());

		w_unstructuredGrid->loadGeometry();
		uint64_t const* w_cumulativeFaceCountPerCell = w_unstructuredGrid->isFaceCountOfCellsConstant()
			? nullptr
			: w_unstructuredGrid->getCumulativeFaceCountPerCell(); // owned by FESAPI
		std::unique_ptr<unsigned char[]> w_cellFaceNormalOutwardlyDirected(new unsigned char[w_cumulativeFaceCountPerCell == nullptr
			? w_unstructuredGrid->getCellCount() * w_unstructuredGrid->getConstantFaceCountOfCells()
			: w_cumulativeFaceCountPerCell[w_unstructuredGrid->getCellCount() - 1]]);
		w_unstructuredGrid->getCellFaceIsRightHanded(w_cellFaceNormalOutwardlyDirected.get());

		for (int64_t w_blocked : w_blockedCells)
		{
			const uint64_t w_cell = static_cast<uint64_t>(w_blocked);
			bool w_isOptimizedCell = false;
			const uint64_t w_localFaceCount = w_unstructuredGrid->getFaceCountOfCell(w_cell);
			if (w_localFaceCount == 4)
			{ // VTK_TETRA
				w_unstrMapper->cellVtkTetra(w_vtkUnstructuredGrid, w_cumulativeFaceCountPerCell, w_cellFaceNormalOutwardlyDirected.get(), w_cell);
				w_isOptimizedCell = true;
			}
			else if (w_localFaceCount == 5)
			{ // VTK_WEDGE or VTK_PYRAMID
				w_unstrMapper->cellVtkWedgeOrPyramid(w_vtkUnstructuredGrid, w_cumulativeFaceCountPerCell, w_cellFaceNormalOutwardlyDirected.get(), w_cell);
				w_isOptimizedCell = true;
			}
			else if (w_localFaceCount == 6)
			{ // VTK_HEXAHEDRON
				w_isOptimizedCell = w_unstrMapper->cellVtkHexahedron(w_vtkUnstructuredGrid, w_cumulativeFaceCountPerCell, w_cellFaceNormalOutwardlyDirected.get(), w_cell);
			}
			else if (w_localFaceCount == 7)
			{ // VTK_PENTAGONAL_PRISM
				w_isOptimizedCell = w_unstrMapper->cellVtkPentagonalPrism(w_vtkUnstructuredGrid, w_cumulativeFaceCountPerCell, w_cellFaceNormalOutwardlyDirected.get(), w_cell);
			}
			else if (w_localFaceCount == 8)
			{ // VTK_HEXAGONAL_PRISM
				w_isOptimizedCell = w_unstrMapper->cellVtkHexagonalPrism(w_vtkUnstructuredGrid, w_cumulativeFaceCountPerCell, w_cellFaceNormalOutwardlyDirected.get(), w_cell);
			}

			if (!w_isOptimizedCell)
			{ // general polyhedron: (numFaces, numFace0Pts, ids..., numFace1Pts, ids..., ...)
				vtkSmartPointer<vtkIdList> w_idList = vtkSmartPointer<vtkIdList>::New();
				w_idList->InsertNextId(w_localFaceCount);
				for (uint64_t w_face = 0; w_face < w_localFaceCount; ++w_face)
				{
					const unsigned int w_localNodeCount = w_unstructuredGrid->getNodeCountOfFaceOfCell(w_cell, w_face);
					w_idList->InsertNextId(w_localNodeCount);
					uint64_t const* w_nodeIndices = w_unstructuredGrid->getNodeIndicesOfFaceOfCell(w_cell, w_face);
					for (unsigned int w_n = 0; w_n < w_localNodeCount; ++w_n)
					{
						w_idList->InsertNextId(w_nodeIndices[w_n]);
					}
				}
				w_vtkUnstructuredGrid->InsertNextCell(VTK_POLYHEDRON, w_idList);
			}
		}
		w_unstructuredGrid->unloadGeometry();
	}
	else
	{
		vtkOutputWindowDisplayWarningText(("BlockedWellbore (" + w_blockedWellbore->getUuid() + "): supporting grid is neither IjkGrid nor UnstructuredGrid\n").c_str());
		return;
	}

	_vtkData->SetPartition(0, w_vtkUnstructuredGrid);
	_vtkData->Modified();
}

//----------------------------------------------------------------------------
std::string ResqmlBlockedWellboreToVtkUnstructuredGrid::unregisterToMapperSupportingGrid()
{
	this->mapperSupportingGrid->unregisterSubRep();
	return this->mapperSupportingGrid->getUuid();
}
