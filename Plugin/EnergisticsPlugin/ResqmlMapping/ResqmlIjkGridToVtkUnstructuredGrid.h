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
#ifndef __ResqmlIjkGridToVtkUnstructuredGrid__h__
#define __ResqmlIjkGridToVtkUnstructuredGrid__h__

/** @brief	transform a resqml ijkGrid representation to vtkUnstructuredGrid
 */

// include system
#include <string>

// include VTK
#include <vtkSmartPointer.h>
#include <vtkPoints.h>

#include "ResqmlAbstractRepresentationToVtkDataset.h"

namespace RESQML2_NS
{
	class AbstractIjkGridRepresentation;
}

class ResqmlIjkGridToVtkUnstructuredGrid : public ResqmlAbstractRepresentationToVtkDataset
{
public:
	/**
	 * Constructor
	 */
	ResqmlIjkGridToVtkUnstructuredGrid(RESQML2_NS::AbstractIjkGridRepresentation *ijkGrid, int proc_number = 1, int max_proc = 1 /*, RESQML2_NS::SubRepresentation* ijkGridSubRep = nullptr*/);

	/**
	 * load vtkDataSet with resqml data
	 */
	void loadVtkObject();

	/**
	 * Create the VTK points from the RESQML points of the RESQML IJK grid representation.
	 */
	vtkSmartPointer<vtkPoints> createPoints(/* RESQML2_NS::AbstractIjkGridRepresentation *ijkGrid */);

	/**
	 *	Return The vtkPoints
	 */
	vtkSmartPointer<vtkPoints> getVtkPoints();

	std::string message;

protected:
	RESQML2_NS::AbstractIjkGridRepresentation *resqmlData;

	/**
	 * method : checkHyperslabingCapacity
	 * variable : ijkGridRepresentation
	 * check if an ijkgrid is Hyperslabed
	 */
	void checkHyperslabingCapacity(RESQML2_NS::AbstractIjkGridRepresentation *ijkGrid);
};
#endif
