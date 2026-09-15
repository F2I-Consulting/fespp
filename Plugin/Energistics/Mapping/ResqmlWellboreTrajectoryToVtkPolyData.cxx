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
#include "Mapping/ResqmlWellboreTrajectoryToVtkPolyData.h"

// include VTK library
#include <vtkPolyData.h>
#include <vtkSmartPointer.h>
#include <vtkPointData.h>
#include <vtkCellArray.h>
#include <vtkPolyLine.h>
#include <vtkDoubleArray.h>

#include <memory>

// include F2i-consulting Energistics Standards API
#include <fesapi/eml2/AbstractLocal3dCrs.h>
#include <fesapi/resqml2/WellboreTrajectoryRepresentation.h>

//----------------------------------------------------------------------------
ResqmlWellboreTrajectoryToVtkPolyData::ResqmlWellboreTrajectoryToVtkPolyData(const resqml2::WellboreTrajectoryRepresentation *wellbore, uint32_t p_procNumber, uint32_t p_maxProc)
	: ResqmlAbstractRepresentationToVtkPartitionedDataSet(wellbore,
														  p_procNumber,
														  p_maxProc)
{
	_pointCount = wellbore->getXyzPointCountOfPatch(0);

	_vtkData = vtkSmartPointer<vtkPartitionedDataSet>::New();

	_vtkData->Modified();
}

//----------------------------------------------------------------------------
const RESQML2_NS::WellboreTrajectoryRepresentation *ResqmlWellboreTrajectoryToVtkPolyData::getResqmlData() const
{
	return static_cast<const RESQML2_NS::WellboreTrajectoryRepresentation *>(_resqmlData);
}

//----------------------------------------------------------------------------
void ResqmlWellboreTrajectoryToVtkPolyData::loadVtkObject()
{
	try
	{
		// GEOMETRY
		vtkSmartPointer<vtkPolyData> vtk_polydata = vtkSmartPointer<vtkPolyData>::New();
		RESQML2_NS::WellboreTrajectoryRepresentation const *wellbore = getResqmlData();

		// POINT
		double *allXyzPoints = new double[_pointCount * 3]; // Will be deleted by VTK
		wellbore->getXyzPointsOfAllPatchesInGlobalCrs(allXyzPoints);
		vtkSmartPointer<vtkPoints> vtkPts = vtkSmartPointer<vtkPoints>::New();

		const size_t coordCount =_pointCount * 3;
		if (wellbore->getLocalCrs(0)->isDepthOriented())
		{
			for (size_t zCoordIndex = 2; zCoordIndex < coordCount; zCoordIndex += 3)
			{
				allXyzPoints[zCoordIndex] *= -1;
			}
		}

		vtkSmartPointer<vtkDoubleArray> vtkUnderlyingArray = vtkSmartPointer<vtkDoubleArray>::New();
		vtkUnderlyingArray->SetNumberOfComponents(3);
		// Take ownership of the underlying C array
		vtkUnderlyingArray->SetArray(allXyzPoints, coordCount, vtkAbstractArray::VTK_DATA_ARRAY_DELETE);
		vtkPts->SetData(vtkUnderlyingArray);
		vtk_polydata->SetPoints(vtkPts);

		// POLYLINE
		vtkSmartPointer<vtkPolyLine> polylineRepresentation = vtkSmartPointer<vtkPolyLine>::New();
		polylineRepresentation->GetPointIds()->SetNumberOfIds(_pointCount);
		for (uint64_t nodeIndex = 0; nodeIndex < _pointCount; ++nodeIndex)
		{
			polylineRepresentation->GetPointIds()->SetId(nodeIndex, nodeIndex);
		}

		vtkSmartPointer<vtkCellArray> setPolylineRepresentationLines = vtkSmartPointer<vtkCellArray>::New();
		setPolylineRepresentationLines->InsertNextCell(polylineRepresentation);

		vtk_polydata->SetLines(setPolylineRepresentationLines);

		// The TRUE measured depth of each station, as POINT data: the
		// geometry only carries XYZ, so a consumer sampling a grid along the
		// hole could pace nothing better than the chord length — a pseudo-MD
		// that drifts from the channels' true MD on every deviated well. The
		// polyline is not tubed here, so point data pairs with the stations
		// by index. Absent when the trajectory stores no MD (a geometry that
		// is not a parametric line, no HDF proxy): the consumer then falls
		// back to the chord.
		try
		{
			if (wellbore->hasMdValues())
			{
				std::unique_ptr<double[]> w_mdValues(new double[_pointCount]);
				wellbore->getMdValues(w_mdValues.get());
				auto w_mdArray = vtkSmartPointer<vtkDoubleArray>::New();
				w_mdArray->SetName("MD");
				w_mdArray->SetNumberOfTuples(_pointCount);
				for (uint64_t i = 0; i < _pointCount; ++i)
				{
					w_mdArray->SetTuple1(i, w_mdValues[i]);
				}
				vtk_polydata->GetPointData()->AddArray(w_mdArray);
			}
		}
		catch (const std::exception&)
		{
			// no MD stored for this trajectory — the consumer paces on the chord
		}

		_vtkData->SetPartition(0, vtk_polydata);
		_vtkData->Modified();
	}
	catch (const std::exception &)
	{
		vtkOutputWindowDisplayErrorText("Error in load for wellbore trajectory\n");
	}
}
