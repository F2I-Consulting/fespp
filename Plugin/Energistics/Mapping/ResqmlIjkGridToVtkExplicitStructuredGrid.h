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
#ifndef __ResqmlIjkGridToVtkExplicitStructuredGrid__h__
#define __ResqmlIjkGridToVtkExplicitStructuredGrid__h__

/** @brief	transform a resqml ijkGrid representation to vtkExplicitStructuredGrid
 */

// include system
#include <string>

// include VTK
#include <vtkSmartPointer.h>
#include <vtkPoints.h>

#include "ResqmlAbstractRepresentationToVtkPartitionedDataSet.h"

namespace RESQML2_NS
{
	class AbstractIjkGridRepresentation;
}

class ResqmlIjkGridToVtkExplicitStructuredGrid : public ResqmlAbstractRepresentationToVtkPartitionedDataSet
{
public:
	/**
	 * Constructor
	 */
	explicit ResqmlIjkGridToVtkExplicitStructuredGrid(const RESQML2_NS::AbstractIjkGridRepresentation *ijkGrid, uint32_t p_procNumber = 0, uint32_t p_maxProc = 1);

	/**
	 * load vtkDataSet with resqml data
	 */
	void loadVtkObject() override;

	/**
	 * Create the VTK points from the RESQML points of the RESQML IJK grid representation.
	 */
	void createPoints();

	/**
	 *	Return The vtkPoints
	 */
	vtkSmartPointer<vtkPoints> getVtkPoints();

protected:
	const RESQML2_NS::AbstractIjkGridRepresentation *getResqmlData() const;
	vtkSmartPointer<vtkPoints> points;

	uint32_t pointer_on_points;

	/**
	 * method : checkHyperslabingCapacity
	 * variable : ijkGridRepresentation
	 * check if an ijkgrid is Hyperslabed
	 */
	void checkHyperslabingCapacity(const RESQML2_NS::AbstractIjkGridRepresentation *ijkGrid);
};
#endif
