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

// include VTK
#include <vtkSmartPointer.h>
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

protected:
	const RESQML2_NS::BlockedWellboreRepresentation *getResqmlData() const;

	ResqmlAbstractRepresentationToVtkPartitionedDataSet *mapperSupportingGrid;
};
#endif
