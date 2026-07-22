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
#ifndef __ResqmlBlockedWellboreToVtkUnstructuredGrid__h__
#define __ResqmlBlockedWellboreToVtkUnstructuredGrid__h__

/** @brief	transform a RESQML BlockedWellbore into a vtkUnstructuredGrid holding
 *			ONLY the supporting grid's cells the wellbore is blocked in. The
 *			supporting grid may be an IjkGrid OR an UnstructuredGrid; the points
 *			are reused from the supporting grid's mapper. Mirrors the two
 *			SubRepresentation mappers (a SubRep is likewise a cell subset).
 */

#include "ResqmlAbstractRepresentationToVtkPartitionedDataSet.h"

// include system
#include <cstdint>
#include <map>
#include <set>
#include <string>
#include <vector>

// include VTK
#include <vtkSmartPointer.h>
#include <vtkType.h>
#include <vtkUnstructuredGrid.h>

namespace RESQML2_NS
{
	class BlockedWellboreRepresentation;
}

class ResqmlBlockedWellboreToVtkUnstructuredGrid : public ResqmlAbstractRepresentationToVtkPartitionedDataSet
{
public:
	/**
	 * Constructor — `p_supportGridMapper` is the mapper of the supporting grid
	 * (a ResqmlIjkGridToVtkExplicitStructuredGrid or a
	 * ResqmlUnstructuredGridToVtkUnstructuredGrid), held as the common base so
	 * one mapper covers both supporting-grid kinds.
	 */
	ResqmlBlockedWellboreToVtkUnstructuredGrid(const RESQML2_NS::BlockedWellboreRepresentation *p_blockedWellbore, ResqmlAbstractRepresentationToVtkPartitionedDataSet *p_supportGridMapper, uint32_t p_procNumber = 0, uint32_t p_maxProc = 1);

	/**
	 * load _vtkData with the blocked cells extracted from the supporting grid
	 */
	void loadVtkObject() override;

	/**
	 * release the refcount taken on the supporting grid mapper (call on unload)
	 */
	std::string unregisterToMapperSupportingGrid();

	/**
	 * mirror the supporting grid's CELL arrays onto this wellbore's cells:
	 * bwArray[k] = gridArray[_blockedCells[k]] — an exact restriction, no
	 * interpolation. A blocked wellbore carries no RESQML property of its own,
	 * so this is what lets it be coloured / thresholded by its grid's
	 * properties even when the grid itself is hidden.
	 *
	 * Idempotent and safe to call at any time: a no-op while this wellbore or
	 * the supporting grid is not loaded. Only the arrays a previous call
	 * mirrored are evicted, never an array the wellbore legitimately owns.
	 *
	 * p_gridMapper is passed IN and never read back from mapperSupportingGrid:
	 * the caller owns the mapper lifetimes and thereby guarantees it is alive
	 * (mapperSupportingGrid is a raw, non-owning pointer that a grid unload can
	 * leave dangling).
	 */
	void syncCellDataFromSupportingGrid(ResqmlAbstractRepresentationToVtkPartitionedDataSet *p_gridMapper);

protected:
	const RESQML2_NS::BlockedWellboreRepresentation *getResqmlData() const;

	ResqmlAbstractRepresentationToVtkPartitionedDataSet *mapperSupportingGrid;

	/** Flat cell indices of the supporting grid this wellbore is blocked in —
	 *  ascending and duplicate-free. VTK cell k of this wellbore IS the
	 *  supporting grid's flat cell _blockedCells[k]: loadVtkObject inserts the
	 *  cells in that very order (ascending flat scan). */
	std::vector<int64_t> _blockedCells;

	/** Flat cell count of the supporting grid. A grid CELL array is only
	 *  addressable by _blockedCells when it holds exactly that many tuples
	 *  (a K-hyperslabbed grid array is shifted/shorter — skip it). Left at 0
	 *  on a multi-supporting-grid wellbore, which disables the mirror rather
	 *  than guessing which grid the flat indices belong to. */
	uint64_t _supportingCellCount = 0;

	/** Names of the cell arrays THIS class mirrored, so a later sync evicts
	 *  only its own leftovers. */
	std::set<std::string> _syncedArrayNames;

	/** MTime of the grid array behind each mirrored name, at copy time. The
	 *  collection re-pushes the WHOLE selection on every batch, so without
	 *  this memo a click costs O(N_props² x N_wellbores) tuple copies. */
	std::map<std::string, vtkMTimeType> _syncedArrayMTimes;

	/** The refcount on the supporting grid mapper is taken at most once per
	 *  mapper instance — loadVtkObject can run several times (stub repair,
	 *  re-selection) while unregisterToMapperSupportingGrid runs once. */
	bool _registeredOnSupportingGrid = false;
};
#endif
