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
#include "Mapping/ResqmlPointSetToVtkPolyVertex.h"

// include VTK library
#include <vtkPolyVertex.h>
#include <vtkPoints.h>
#include <vtkCellArray.h>
#include <vtkPolyData.h>
#include <vtkSmartPointer.h>
#include <vtkDoubleArray.h>

// include F2i-consulting Energistics Standards API
#include <fesapi/eml2/AbstractLocal3dCrs.h>
#include <fesapi/resqml2/PointSetRepresentation.h>

// include F2i-consulting Energistics Standards ParaView Plugin
#include "Mapping/ResqmlPropertyToVtkDataArray.h"

//----------------------------------------------------------------------------
ResqmlPointSetToVtkPolyVertex::ResqmlPointSetToVtkPolyVertex(const RESQML2_NS::PointSetRepresentation* points, uint32_t p_procNumber, uint32_t p_maxProc)
	: ResqmlAbstractRepresentationToVtkPartitionedDataSet(points,
		p_procNumber,
		p_maxProc)
{
	_pointCount = points->getXyzPointCountOfPatch(0);

	_vtkData = vtkSmartPointer<vtkPartitionedDataSet>::New();

	_vtkData->Modified();
}

//----------------------------------------------------------------------------
const RESQML2_NS::PointSetRepresentation* ResqmlPointSetToVtkPolyVertex::getResqmlData() const
{
	return static_cast<const RESQML2_NS::PointSetRepresentation*>(_resqmlData);
}

//----------------------------------------------------------------------------
void ResqmlPointSetToVtkPolyVertex::loadVtkObject()
{
	RESQML2_NS::PointSetRepresentation const* pointSet = getResqmlData();

	// Create and set the list of points of the vtkPolyData
	vtkSmartPointer<vtkPolyVertex> vtk_polyvertex = vtkSmartPointer<vtkPolyVertex>::New();

	// POINT
	double* allXyzPoints = new double[_pointCount * 3]; // Will be deleted by VTK
	pointSet->getXyzPointsOfPatchInGlobalCrs(0, allXyzPoints);
	const size_t coordCount = _pointCount * 3;

	// Determine Z transformation factors before the loop
	const bool shouldBe2D = pointSet->isIn2D(0);
	const bool shouldInvertZ = pointSet->getLocalCrs(0)->isDepthOriented();
	bool nanFound = false;

	// Explicit index counter: X=0, Y=1, Z=2. Used for fast Z identification.
	int zIndex = 0;

	for (size_t i = 0; i < coordCount; ++i) {
		// 1. NaN Handling (applies to X, Y, and Z)
		if (std::isnan(allXyzPoints[i])) {
			allXyzPoints[i] = 0.0; // Replace NaN with 0.0 as requested
			nanFound = true;
		}

		// 2. Z Transformation Management
		if (zIndex == 2) { // Only for the Z index
			if (shouldBe2D) {
				// Priority: Set Z to zero for 2D representation
				allXyzPoints[i] = 0.0;
			}
			else if (shouldInvertZ) {
				// Invert Z if not 2D and if Depth-Oriented
				allXyzPoints[i] *= -1.0;
			}
		}

		// 3. Counter Update
		zIndex++;
		if (zIndex == 3) {
			zIndex = 0;
		}
	}

	if (nanFound) {
		vtkOutputWindowDisplayText(("WARNING: Coordinates in " +
			pointSet->getTitle() +
			" contained NaN values, which have been replaced by 0.0.").c_str());
	}
	if (shouldBe2D) {
		vtkOutputWindowDisplayText((pointSet->getTitle() + " is in 2D (z = 0)").c_str());
	}

	vtkSmartPointer<vtkDoubleArray> pointsArray = vtkSmartPointer<vtkDoubleArray>::New();
	pointsArray->SetNumberOfComponents(3);
	// Take ownership of the underlying C array
	pointsArray->SetArray(allXyzPoints, coordCount, vtkAbstractArray::VTK_DATA_ARRAY_DELETE);

	vtkSmartPointer<vtkPoints> vtk_points = vtkSmartPointer<vtkPoints>::New();
	vtk_points->SetData(pointsArray);
	vtk_polyvertex->GetPointIds()->SetNumberOfIds(_pointCount);
	for (int i = 0; i < _pointCount; ++i) {
		vtk_polyvertex->GetPointIds()->SetId(i, i);
	}

	vtkNew<vtkCellArray> vtk_cells;
	vtk_cells->InsertNextCell(vtk_polyvertex);

	vtkNew<vtkPolyData> vtk_polyData;
	vtk_polyData->SetPoints(vtk_points);
	vtk_polyData->SetVerts(vtk_cells);

	_vtkData->SetPartition(0, vtk_polyData);
	_vtkData->Modified();
}
