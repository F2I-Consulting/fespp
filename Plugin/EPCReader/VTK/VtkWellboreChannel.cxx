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
#include "VtkWellboreChannel.h"

#include <vtkPointData.h>
#include <vtkTubeFilter.h>
#include <vtkDoubleArray.h>
#include <vtkPolyLine.h>

#include <fesapi/resqml2/WellboreFrameRepresentation.h>
#include <fesapi/resqml2/ContinuousProperty.h>
#include <fesapi/resqml2/DiscreteProperty.h>
#include <fesapi/resqml2/CategoricalProperty.h>

//----------------------------------------------------------------------------
VtkWellboreChannel::VtkWellboreChannel(const std::string & fileName, const std::string & name, const std::string & uuid, const std::string & uuidParent, const COMMON_NS::DataObjectRepository *repoRepresentation, const COMMON_NS::DataObjectRepository *repoSubRepresentation) :
VtkResqml2PolyData(fileName, name, uuid, uuidParent, repoRepresentation, repoSubRepresentation)
{}

//----------------------------------------------------------------------------
void VtkWellboreChannel::visualize(const std::string & uuid)
{
	if (vtkOutput != nullptr) {
		return;
	}

	if (uuid == getUuid()) {
		RESQML2_NS::WellboreFrameRepresentation* frame = epcPackageRepresentation->getDataObjectByUuid<RESQML2_NS::WellboreFrameRepresentation>(getParent());

		// We need to build first a polyline for the channel to support the vtk tube.
		const ULONG64 pointCount = frame->getXyzPointCountOfPatch(0);
		double* allXyzPoints = new double[pointCount * 3]; // Will be deleted by VTK
		frame->getXyzPointsOfAllPatchesInGlobalCrs(allXyzPoints);
		vtkSmartPointer<vtkPoints> vtkPts = createVtkPoints(pointCount, allXyzPoints, frame->getLocalCrs(0));

		auto lines = vtkSmartPointer<vtkCellArray>::New();
		lines->InsertNextCell(pointCount);
		for (unsigned int i = 0; i < pointCount; ++i) {
			lines->InsertCellPoint(i);
		}

		auto channelPolyline = vtkSmartPointer<vtkPolyData>::New();
		channelPolyline->SetPoints(vtkPts);
		channelPolyline->SetLines(lines);

		// Varying tube radius
		RESQML2_NS::AbstractValuesProperty* prop = epcPackageRepresentation->getDataObjectByUuid<RESQML2_NS::AbstractValuesProperty>(getUuid());
		auto tubeRadius = vtkSmartPointer<vtkDoubleArray>::New();
		tubeRadius->SetName(prop->getTitle().c_str());
		tubeRadius->SetNumberOfTuples(pointCount);
		if (dynamic_cast<RESQML2_NS::ContinuousProperty*>(prop) != nullptr) {
			std::unique_ptr<double[]> values(new double[pointCount]);
			prop->getDoubleValuesOfPatch(0, values.get());
			for (unsigned int i = 0; i < pointCount; ++i) {
				tubeRadius->SetTuple1(i, values[i]);
			}
		}
		else if (dynamic_cast<RESQML2_NS::DiscreteProperty*>(prop) != nullptr || dynamic_cast<RESQML2_NS::CategoricalProperty*>(prop)) {
			std::unique_ptr<int[]> values(new int[pointCount]);
			prop->getIntValuesOfPatch(0, values.get());
			for (unsigned int i = 0; i < pointCount; ++i) {
				tubeRadius->SetTuple1(i, values[i]);
			}
		}
		else {
			vtkOutputWindowDisplayText("Cannot show a log which is not discrete, categorical no continuous.");
			return;
		}

		channelPolyline->GetPointData()->AddArray(tubeRadius);
		channelPolyline->GetPointData()->SetActiveScalars(prop->getTitle().c_str());

		// Build the tube
		vtkSmartPointer<vtkTubeFilter> tubeFilter = vtkSmartPointer<vtkTubeFilter>::New();
		tubeFilter->SetInputData(channelPolyline);
		tubeFilter->SetNumberOfSides(10);
		tubeFilter->SetRadius(10);
		tubeFilter->SetVaryRadiusToVaryRadiusByScalar();
		tubeFilter->Update();

		vtkOutput = tubeFilter->GetOutput();
	}
}

//----------------------------------------------------------------------------
void VtkWellboreChannel::remove(const std::string & uuid)
{
	if (uuid == getUuid()) {
		vtkOutput = nullptr;
	}
}
