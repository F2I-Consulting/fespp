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
#ifndef __ResqmlTriangulatedToVtkPolyData_h
#define __ResqmlTriangulatedToVtkPolyData_h

/** @brief	transform a RESQML TriangulatedSetRepresentation representation to vtkPolyData
 */

// include system
#include <string>

#include "Mapping/ResqmlAbstractRepresentationToVtkPartitionedDataSet.h"

namespace RESQML2_NS
{
	class TriangulatedSetRepresentation;
}

class ResqmlTriangulatedToVtkPolyData : public ResqmlAbstractRepresentationToVtkPartitionedDataSet
{
public:
	/**
	 * Constructor
	 */
	ResqmlTriangulatedToVtkPolyData(const RESQML2_NS::TriangulatedSetRepresentation *triangulated, uint32_t patch_index, int p_procNumber = 0, int p_maxProc = 1);

	/**
	 * load vtkDataSet with RESQML data
	 */
	void loadVtkObject();

protected:
	/**
	 * Get the node count of all patches which are before than the current patch index
	 */
	uint32_t getPreviousPatchesNodeCount() const;

	const RESQML2_NS::TriangulatedSetRepresentation *getResqmlData() const;
	uint32_t patch_index;
};
#endif
