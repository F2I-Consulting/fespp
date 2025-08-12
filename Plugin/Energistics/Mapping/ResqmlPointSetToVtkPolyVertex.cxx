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
	double *allXyzPoints = new double[_pointCount * 3]; // Will be deleted by VTK
	pointSet->getXyzPointsOfPatchInGlobalCrs(0, allXyzPoints);

	const size_t coordCount = _pointCount * 3;
	if (pointSet->getLocalCrs(0)->isDepthOriented())
	{
		for (size_t zCoordIndex = 2; zCoordIndex < coordCount; zCoordIndex += 3)
		{
			allXyzPoints[zCoordIndex] *= -1;
		}
	}

	vtkSmartPointer<vtkDoubleArray> pointsArray = vtkSmartPointer<vtkDoubleArray>::New();
	pointsArray->SetNumberOfComponents(3);
	// Take ownership of the underlying C array
	pointsArray->SetArray(allXyzPoints, coordCount, vtkAbstractArray::VTK_DATA_ARRAY_DELETE);

	vtkSmartPointer<vtkPoints> vtk_points = vtkSmartPointer<vtkPoints>::New();
	vtk_points->SetData(pointsArray);
	vtk_polyvertex->GetPointIds()->SetNumberOfIds(_pointCount);
	for (int i = 0; i < _pointCount; ++i)
	{
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
