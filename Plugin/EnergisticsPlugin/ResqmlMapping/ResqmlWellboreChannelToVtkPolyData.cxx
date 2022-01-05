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
#include "ResqmlWellboreChannelToVtkPolyData.h"

#include <vtkPointData.h>
#include <vtkTubeFilter.h>
#include <vtkDoubleArray.h>
#include <vtkPolyLine.h>

#include <fesapi/resqml2/WellboreFrameRepresentation.h>
#include <fesapi/resqml2/ContinuousProperty.h>
#include <fesapi/resqml2/DiscreteProperty.h>
#include <fesapi/resqml2/CategoricalProperty.h>
#include <fesapi/resqml2/AbstractValuesProperty.h>
#include <fesapi/resqml2/AbstractLocal3dCrs.h>

// include F2i-consulting Energistics Paraview Plugin


//----------------------------------------------------------------------------
ResqmlWellboreChannelToVtkPolyData::ResqmlWellboreChannelToVtkPolyData(resqml2::WellboreFrameRepresentation *frame, resqml2::AbstractValuesProperty *property, int proc_number, int max_proc)
	: ResqmlAbstractRepresentationToVtkDataset(frame,
											   proc_number -1,
											   max_proc)
{
	this->abstractProperty = property;
}

//----------------------------------------------------------------------------
void ResqmlWellboreChannelToVtkPolyData::loadVtkObject()
{
	auto frame = dynamic_cast<resqml2::WellboreFrameRepresentation *>(this->resqmlData);

	// We need to build first a polyline for the channel to support the vtk tube.
	const ULONG64 pointCount = frame->getXyzPointCountOfPatch(0);
	double *allXyzPoints = new double[pointCount * 3]; // Will be deleted by VTK
	frame->getXyzPointsOfAllPatchesInGlobalCrs(allXyzPoints);

		vtkSmartPointer<vtkPoints> vtkPts = vtkSmartPointer<vtkPoints>::New();
	const size_t coordCount = pointCount * 3;
	if (frame->getLocalCrs(0)->isDepthOriented()) {
		for (size_t zCoordIndex = 2; zCoordIndex < coordCount; zCoordIndex += 3) {
			allXyzPoints[zCoordIndex] *= -1;
		}
	}

	vtkSmartPointer<vtkDoubleArray> vtkUnderlyingArray = vtkSmartPointer<vtkDoubleArray>::New();
	vtkUnderlyingArray->SetNumberOfComponents(3);
	// Take ownership of the underlying C array
	vtkUnderlyingArray->SetArray(allXyzPoints, coordCount, vtkAbstractArray::VTK_DATA_ARRAY_DELETE);
	vtkPts->SetData(vtkUnderlyingArray);

	auto lines = vtkSmartPointer<vtkCellArray>::New();
	lines->InsertNextCell(pointCount);
	for (unsigned int i = 0; i < pointCount; ++i)
	{
		lines->InsertCellPoint(i);
	}

	auto channelPolyline = vtkSmartPointer<vtkPolyData>::New();
	channelPolyline->SetPoints(vtkPts);
	channelPolyline->SetLines(lines);

	// Varying tube radius
	auto tubeRadius = vtkSmartPointer<vtkDoubleArray>::New();
	tubeRadius->SetName(this->abstractProperty->getTitle().c_str());
	tubeRadius->SetNumberOfTuples(pointCount);
	if (dynamic_cast<resqml2::ContinuousProperty *>(this->abstractProperty) != nullptr)
	{
		std::unique_ptr<double[]> values(new double[pointCount]);
		this->abstractProperty->getDoubleValuesOfPatch(0, values.get());
		for (unsigned int i = 0; i < pointCount; ++i)
		{
			tubeRadius->SetTuple1(i, values[i]);
		}
	}
	else if (dynamic_cast<resqml2::DiscreteProperty *>(this->abstractProperty) != nullptr || dynamic_cast<RESQML2_NS::CategoricalProperty *>(this->abstractProperty) != nullptr)
	{
		std::unique_ptr<int[]> values(new int[pointCount]);
		this->abstractProperty->getIntValuesOfPatch(0, values.get());
		for (unsigned int i = 0; i < pointCount; ++i)
		{
			tubeRadius->SetTuple1(i, values[i]);
		}
	}
	else
	{
		vtkOutputWindowDisplayText("Cannot show a log which is not discrete, categorical no continuous.");
		return;
	}

	channelPolyline->GetPointData()->AddArray(tubeRadius);
	channelPolyline->GetPointData()->SetActiveScalars(this->abstractProperty->getTitle().c_str());

	// Build the tube
	vtkSmartPointer<vtkTubeFilter> tubeFilter = vtkSmartPointer<vtkTubeFilter>::New();
	tubeFilter->SetInputData(channelPolyline);
	tubeFilter->SetNumberOfSides(10);
	tubeFilter->SetRadius(10);
	tubeFilter->SetVaryRadiusToVaryRadiusByScalar();
	tubeFilter->Update();

		this->vtkData->SetPartition(0, tubeFilter->GetOutput());
	this->vtkData->Modified();
}
