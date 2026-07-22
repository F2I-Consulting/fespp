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
#include <cstring>
#include <memory>
#include <set>
#include <string>
#include <utility>
#include <vector>

// include VTK library
#include <vtkSmartPointer.h>
#include <vtkCellData.h>
#include <vtkCellType.h>
#include <vtkDataArray.h>
#include <vtkDataSet.h>
#include <vtkDataSetAttributes.h>
#include <vtkIdList.h>
#include <vtkLookupTable.h>
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
	// A (re)load rebuilds partition 0 from scratch, so every mirrored cell array
	// goes away with the previous vtkUnstructuredGrid: drop the correspondence
	// cache too, and let the sync at the end of this method rebuild it.
	_blockedCells.clear();
	_syncedArrayNames.clear();
	_syncedArrayMTimes.clear();
	_supportingCellCount = 0;

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
	// A cell the trajectory enters twice would stall the ascending flat scan
	// below (it never rewinds), silently dropping every later cell — and it
	// would break the cell k <-> w_blockedCells[k] correspondence that the
	// property mirroring relies on.
	w_blockedCells.erase(std::unique(w_blockedCells.begin(), w_blockedCells.end()), w_blockedCells.end());
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
		if (!_registeredOnSupportingGrid)
		{
			// At most once per mapper instance: loadVtkObject can run again (stub
			// repair, re-selection) while the unregister on unload runs once.
			mapperSupportingGrid->registerSubRep();
			_registeredOnSupportingGrid = true;
		}
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
		// NOT this wellbore's cell count: the flat scan below reuses these as its
		// bounds. Keep the grid's flat cell count apart, it is the yardstick the
		// cell-array mirroring validates the grid's arrays against.
		_supportingCellCount = w_ijkGrid->getCellCount();

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
		// The flat scan only advances on an exact match, so a (bogus) index at or
		// beyond ni*nj*nk sits unmatched at the sorted tail: the cells actually
		// inserted are exactly the prefix [0, w_indice). Truncate so the
		// cell k <-> w_blockedCells[k] correspondence stays exact — otherwise the
		// sync's count guard would silently disable the mirror on this wellbore.
		w_blockedCells.resize(w_indice);
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
		if (!_registeredOnSupportingGrid)
		{
			// Same once-per-instance guard as the IjkGrid branch above.
			mapperSupportingGrid->registerSubRep();
			_registeredOnSupportingGrid = true;
		}
		_supportingCellCount = w_unstructuredGrid->getCellCount();
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

	// Cells were inserted in ascending flat order, so VTK cell k IS the grid's
	// flat cell w_blockedCells[k]. Keep that mapping: it is the whole basis of
	// the cell-property restriction.
	_blockedCells = std::move(w_blockedCells);

	// Several supporting grids: the flat indices could belong to any of them
	// and loadVtkObject built everything against grid 0 only. Refuse to guess —
	// a wrong-but-plausible PORO on a wellbore is worse than no colour at all.
	// Leaving the count at 0 keeps every sync a no-op for this wellbore.
	if (w_blockedWellbore->getSupportingGridRepresentationCount() > 1)
	{
		_supportingCellCount = 0;
	}

	// The supporting grid may already carry the cell properties the user checked
	// BEFORE this wellbore (the common order: a property's node id is lower than
	// a blocked wellbore's, so it is processed first). A no-op while the grid is
	// still a points-only stub — the collection pushes to us later in that case.
	// mapperSupportingGrid was just dereferenced above (getVtkPoints), so it is
	// alive here.
	syncCellDataFromSupportingGrid(mapperSupportingGrid);
}

//----------------------------------------------------------------------------
void ResqmlBlockedWellboreToVtkUnstructuredGrid::syncCellDataFromSupportingGrid(ResqmlAbstractRepresentationToVtkPartitionedDataSet* p_gridMapper)
{
	if (p_gridMapper == nullptr || _blockedCells.empty() || _supportingCellCount == 0)
	{
		return;
	}

	// This wellbore loaded?
	if (_vtkData == nullptr || _vtkData->GetNumberOfPartitions() == 0)
	{
		return;
	}
	vtkDataSet* w_bwDataSet = vtkDataSet::SafeDownCast(_vtkData->GetPartition(0));
	if (w_bwDataSet == nullptr)
	{
		return;
	}

	// Supporting grid loaded? A points-only stub has no partition — not an
	// error, the properties simply are not there yet.
	vtkSmartPointer<vtkPartitionedDataSet> w_gridOutput = p_gridMapper->getOutput();
	if (w_gridOutput == nullptr || w_gridOutput->GetNumberOfPartitions() == 0)
	{
		return;
	}
	vtkDataSet* w_gridDataSet = vtkDataSet::SafeDownCast(w_gridOutput->GetPartition(0));
	if (w_gridDataSet == nullptr)
	{
		return;
	}

	vtkCellData* w_gridCellData = w_gridDataSet->GetCellData();
	vtkCellData* w_bwCellData = w_bwDataSet->GetCellData();
	if (w_gridCellData == nullptr || w_bwCellData == nullptr)
	{
		return;
	}

	// One VTK cell per blocked cell, in the same order — the correspondence is
	// only sound if the counts still agree.
	const vtkIdType w_bwCellCount = w_bwDataSet->GetNumberOfCells();
	if (w_bwCellCount != static_cast<vtkIdType>(_blockedCells.size()))
	{
		return;
	}
	// _blockedCells is sorted, so front()/back() bound every index: this is what
	// makes the SetTuple reads below unconditionally in range.
	if (_blockedCells.front() < 0
		|| static_cast<uint64_t>(_blockedCells.back()) >= _supportingCellCount)
	{
		return;
	}

	std::set<std::string> w_freshNames;
	std::map<std::string, vtkMTimeType> w_freshMTimes;
	const int w_arrayCount = w_gridCellData->GetNumberOfArrays();
	for (int w_a = 0; w_a < w_arrayCount; ++w_a)
	{
		vtkDataArray* w_gridArray = w_gridCellData->GetArray(w_a);
		if (w_gridArray == nullptr)
		{
			continue;
		}
		const char* w_name = w_gridArray->GetName();
		if (w_name == nullptr || w_name[0] == '\0')
		{
			continue; // an unnamed array can be neither mirrored nor evicted by name
		}
		// VTK-internal bookkeeping of the supporting grid, never property data:
		// vtkGhostType would import the grid's dead-cell blanking (VTK honours
		// the name and installs it as the live ghost array) and silently hide
		// wellbore cells; ConnectivityFlags is vtkExplicitStructuredGrid face
		// bookkeeping, meaningless on this vtkUnstructuredGrid.
		if (std::strcmp(w_name, vtkDataSetAttributes::GhostArrayName()) == 0
			|| std::strcmp(w_name, "ConnectivityFlags") == 0)
		{
			continue;
		}
		// _blockedCells holds FLAT grid indices: they only address this array's
		// tuples when it covers the WHOLE grid. A K-hyperslabbed array (multi
		// process) is shorter and shifted — skip rather than mis-map.
		if (w_gridArray->GetNumberOfTuples() != static_cast<vtkIdType>(_supportingCellCount))
		{
			continue;
		}

		// The collection re-pushes the WHOLE selection every batch, so this sync
		// runs O(N_props) times per click: skip the copy when the grid array has
		// not changed since we last mirrored it (a time-step swap reloads the
		// grid array in place and bumps its MTime, so it is never missed).
		const auto w_seen = _syncedArrayMTimes.find(w_name);
		if (w_seen != _syncedArrayMTimes.end()
			&& w_seen->second == w_gridArray->GetMTime()
			&& w_bwCellData->HasArray(w_name))
		{
			w_freshNames.insert(w_name);
			w_freshMTimes[w_name] = w_seen->second;
			continue;
		}

		// NewInstance keeps the grid's concrete type (double / float / int ...)
		// and SetTuple copies the whole tuple, so vectorial properties ride along
		// untouched. No hard-coded cast, no interpolation.
		vtkSmartPointer<vtkDataArray> w_bwArray;
		w_bwArray.TakeReference(w_gridArray->NewInstance());
		w_bwArray->SetName(w_name);
		w_bwArray->SetNumberOfComponents(w_gridArray->GetNumberOfComponents());
		w_bwArray->SetNumberOfTuples(w_bwCellCount);
		for (vtkIdType w_k = 0; w_k < w_bwCellCount; ++w_k)
		{
			w_bwArray->SetTuple(w_k, static_cast<vtkIdType>(_blockedCells[w_k]), w_gridArray);
		}
		// Categorical / property-kind colour map, if the grid's array carries one.
		w_bwArray->SetLookupTable(w_gridArray->GetLookupTable());

		w_bwCellData->AddArray(w_bwArray); // replaces the homonym, if any
		w_freshNames.insert(w_name);
		w_freshMTimes[w_name] = w_gridArray->GetMTime();
	}

	// Evict only what a PREVIOUS sync mirrored and that the grid no longer
	// carries (property unchecked, time step swapped to another array name...).
	for (const std::string& w_stale : _syncedArrayNames)
	{
		if (w_freshNames.find(w_stale) == w_freshNames.end())
		{
			w_bwCellData->RemoveArray(w_stale.c_str());
		}
	}
	_syncedArrayNames = std::move(w_freshNames);
	_syncedArrayMTimes = std::move(w_freshMTimes);

	// Follow the grid's active scalars when it has some, so the wellbore colours
	// with it out of the box. Best effort: the app binds ColorBy by name anyway.
	vtkDataArray* w_gridScalars = w_gridCellData->GetScalars();
	if (w_gridScalars != nullptr && w_gridScalars->GetName() != nullptr
		&& w_bwCellData->HasArray(w_gridScalars->GetName()))
	{
		w_bwCellData->SetActiveScalars(w_gridScalars->GetName());
	}

	w_bwDataSet->Modified();
	_vtkData->Modified();
}

//----------------------------------------------------------------------------
std::string ResqmlBlockedWellboreToVtkUnstructuredGrid::unregisterToMapperSupportingGrid()
{
	this->mapperSupportingGrid->unregisterSubRep();
	_registeredOnSupportingGrid = false;
	return this->mapperSupportingGrid->getUuid();
}
