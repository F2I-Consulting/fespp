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
#include "Mapping/WitsmlWellboreCompletionPerforationToVtkPolyData.h"

#include <array>

#include <vtkPolyData.h>
#include <vtkPointData.h>
#include <vtkTubeFilter.h>
#include <vtkDoubleArray.h>
#include <vtkPolyLine.h>
#include <vtkCylinderSource.h>
#include <vtkTransform.h>
#include <vtkTransformPolyDataFilter.h>
#include <vtkLineSource.h>
#include <vtkDataAssembly.h>
#include <vtkPoints.h>

#include <fesapi/witsml2_1/WellboreCompletion.h>
#include <fesapi/resqml2/MdDatum.h>
#include <fesapi/resqml2/WellboreFeature.h>
#include <fesapi/resqml2/WellboreInterpretation.h>
#include <fesapi/resqml2/WellboreTrajectoryRepresentation.h>

WitsmlWellboreCompletionPerforationToVtkPolyData::WitsmlWellboreCompletionPerforationToVtkPolyData(const resqml2::WellboreTrajectoryRepresentation* wellboreTrajectory, const WITSML2_1_NS::WellboreCompletion *wellboreCompletion, const std::string &connectionuid, const std::string &title, int proc_number, int max_proc)
	: wellboreCompletion(wellboreCompletion),
	wellboreTrajectory(wellboreTrajectory),
	title(title),
	connection(connectionuid)
{
	this->vtkData = vtkSmartPointer<vtkPolyData>::New();
	this->vtkData->Modified();

	this->loadVtkObject();
}

void WitsmlWellboreCompletionPerforationToVtkPolyData::loadVtkObject()
{

	// Check that the trajectory is valid.
	if (this->wellboreTrajectory == nullptr)
	{
		vtkOutputWindowDisplayErrorText("Cannot compute the XYZ points of the frame without a valid wellbore trajectory.");
		return;
	}

	// Check that the completion is valid.
	if (this->wellboreCompletion == nullptr)
	{
		vtkOutputWindowDisplayErrorText("Cannot compute the XYZ points of the frame without a valid wellbore completion.");
		return;
	}

	// Get the MD datum.
	auto mdDatum = this->wellboreTrajectory->getMdDatum();
	if (mdDatum == nullptr || mdDatum->isPartial())
	{
		vtkOutputWindowDisplayErrorText("Cannot compute the XYZ points of the frame without the MD datum.");
		return;
	}

	// Check that the wellbore completion has any perforations.
	if (this->wellboreCompletion->getConnectionCount(WITSML2_1_NS::WellboreCompletion::WellReservoirConnectionType::PERFORATION) == 0)
	{
		vtkOutputWindowDisplayErrorText("There are no perforations on the wellbore completion.");
		return;
	}

	// Iterate over the perforations.
	for (uint64_t perforationIndex = 0; perforationIndex < this->wellboreCompletion->getConnectionCount(WITSML2_1_NS::WellboreCompletion::WellReservoirConnectionType::PERFORATION); ++perforationIndex)
	{
		if (this->connection == this->wellboreCompletion->getConnectionUid(WITSML2_1_NS::WellboreCompletion::WellReservoirConnectionType::PERFORATION, perforationIndex))
		{
			// Check that the perforation has an MD interval.
			if (!this->wellboreCompletion->hasConnectionMdInterval(WITSML2_1_NS::WellboreCompletion::WellReservoirConnectionType::PERFORATION, perforationIndex))
			{
				continue;
			}

			// Get the MD values for the top and bottom of the perforation.
			const double top = this->wellboreCompletion->getConnectionTopMd(WITSML2_1_NS::WellboreCompletion::WellReservoirConnectionType::PERFORATION, perforationIndex);
			const double base = this->wellboreCompletion->getConnectionBaseMd(WITSML2_1_NS::WellboreCompletion::WellReservoirConnectionType::PERFORATION, perforationIndex);

			// Convert the MD values to XYZ values.
			std::array<double, 2> mdTrajValues = {top, base};
			std::array<double, 3 * mdTrajValues.size()> xyzTrajValues;
			this->wellboreTrajectory->convertMdValuesToXyzValues(mdTrajValues.data(), mdTrajValues.size(), xyzTrajValues.data());

			// Get the MD and XYZ values for all the patches in the wellbore trajectory.
			const uint64_t pointCount = this->wellboreTrajectory->getXyzPointCountOfAllPatches(); // std::vector<double> mdValues(wellboreTrajectory->getXyzPointCountOfAllPatches());
			std::unique_ptr<double[]> mdValues(new double[pointCount]);
			this->wellboreTrajectory->getMdValues(mdValues.get());

			std::unique_ptr<double[]> xyzPoints(new double[pointCount * 3]);
			this->wellboreTrajectory->getXyzPointsOfAllPatchesInGlobalCrs(xyzPoints.get());

			// Create a list of intermediate MD points.
			std::vector<std::array<double, 3>> intermediateMdPoints;
			for (size_t i = 0; i < pointCount; ++i)
			{
				if (top <= mdValues[i] && mdValues[i] <= base)
				{
					intermediateMdPoints.push_back({{xyzPoints[i * 3], xyzPoints[i * 3 + 1], xyzPoints[i * 3 + 2]}});
				}
			}

			// Create a vtkPoints object.
			vtkSmartPointer<vtkPoints> vtkPts = vtkSmartPointer<vtkPoints>::New();

			// Add the top point.
			vtkPts->InsertNextPoint(xyzTrajValues[0], xyzTrajValues[1], xyzTrajValues[2]);

			// Add the intermediate points.
			for (auto const &point : intermediateMdPoints)
			{
				vtkPts->InsertNextPoint(point[0], point[1], point[2]);
			}

			// Add the base point.
			vtkPts->InsertNextPoint(xyzTrajValues[3], xyzTrajValues[4], xyzTrajValues[5]);

			// Create a vtkCellArray object.
			vtkSmartPointer<vtkCellArray> lines = vtkSmartPointer<vtkCellArray>::New();
			lines->InsertNextCell(2 + intermediateMdPoints.size());
			for (size_t i = 0; i < 2 + intermediateMdPoints.size(); ++i)
			{
				lines->InsertCellPoint(i);
			}

			// Create a vtkPolyData object for the perforation.
			vtkSmartPointer<vtkPolyData> perforationPolyData = vtkSmartPointer<vtkPolyData>::New();
			perforationPolyData->SetPoints(vtkPts);
			perforationPolyData->SetLines(lines);

			// Create a vtkTubeFilter object.
			vtkSmartPointer<vtkTubeFilter> tubeFilter = vtkSmartPointer<vtkTubeFilter>::New();
			tubeFilter->SetInputData(perforationPolyData);
			tubeFilter->SetNumberOfSides(10);
			tubeFilter->SetRadius(10);
			tubeFilter->SetVaryRadiusToVaryRadiusByScalar();
			tubeFilter->Update();

			// Add the perforationPolyData to the vector.
			this->vtkData = tubeFilter->GetOutput();
			this->vtkData->Modified();
		}
	}
}
