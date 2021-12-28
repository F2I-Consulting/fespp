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
#include "ResqmlMapping/ResqmlPolylineToVtkPolyData.h"

// include VTK library
#include <vtkPolyData.h>
#include <vtkPointData.h>
#include <vtkCellArray.h>
#include <vtkPolyLine.h>
#include <vtkSmartPointer.h>
#include <vtkDoubleArray.h>

// include F2i-consulting Energistics Standards API
#include <fesapi/resqml2/PolylineSetRepresentation.h>
#include <fesapi/resqml2/AbstractLocal3dCrs.h>

// include F2i-consulting Energistics Standards ParaView Plugin
#include "ResqmlMapping/ResqmlPropertyToVtkDataArray.h"

//----------------------------------------------------------------------------
ResqmlPolylineToVtkPolyData::ResqmlPolylineToVtkPolyData(RESQML2_NS::PolylineSetRepresentation *polyline, int proc_number, int max_proc)
	: ResqmlAbstractRepresentationToVtkDataset(polyline,
											   proc_number - 1,
											   max_proc - 1),
	  resqmlData(polyline)
{
	this->pointCount = polyline->getXyzPointCountOfPatch(0);

	this->vtkData = vtkSmartPointer<vtkPartitionedDataSet>::New();

	this->loadVtkObject();

	this->vtkData->Modified();
}

//----------------------------------------------------------------------------
void ResqmlPolylineToVtkPolyData::loadVtkObject()
{
	// Create and set the list of points of the vtkPolyData
	vtkSmartPointer<vtkPolyData> vtk_polydata = vtkSmartPointer<vtkPolyData>::New();

	// POINT
	double *allXyzPoints = new double[this->pointCount * 3]; // Will be deleted by VTK
	this->resqmlData->getXyzPointsOfPatch(0, allXyzPoints);

	vtkSmartPointer<vtkPoints> vtkPts = vtkSmartPointer<vtkPoints>::New();

	const size_t coordCount = this->pointCount * 3;
	if (this->resqmlData->getLocalCrs(0)->isDepthOriented())
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
	vtkSmartPointer<vtkCellArray> setPolylineRepresentationLines = vtkSmartPointer<vtkCellArray>::New();

	unsigned int countPolyline = this->resqmlData->getPolylineCountOfPatch(0);

	std::unique_ptr<unsigned int[]> countNodePolylineInPatch(new unsigned int[countPolyline]);
	this->resqmlData->getNodeCountPerPolylineInPatch(0, countNodePolylineInPatch.get());

	unsigned int idPoint = 0;
	for (unsigned int polylineIndex = 0; polylineIndex < countPolyline; ++polylineIndex)
	{
		vtkSmartPointer<vtkPolyLine> polylineRepresentation = vtkSmartPointer<vtkPolyLine>::New();
		polylineRepresentation->GetPointIds()->SetNumberOfIds(countNodePolylineInPatch[polylineIndex]);
		for (unsigned int line = 0; line < countNodePolylineInPatch[polylineIndex]; ++line)
		{
			polylineRepresentation->GetPointIds()->SetId(line, idPoint);
			idPoint++;
		}
		setPolylineRepresentationLines->InsertNextCell(polylineRepresentation);
		vtk_polydata->SetLines(setPolylineRepresentationLines);
	}

	this->vtkData->SetPartition(0, vtk_polydata);
	this->vtkData->Modified();
}
