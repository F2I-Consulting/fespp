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
#include "WitsmlWellboreCompletionPerforationToVtkPolyData.h"

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

#include <fesapi/witsml2_1/WellboreCompletion.h>
#include <fesapi/resqml2/MdDatum.h>
#include <fesapi/resqml2/WellboreFeature.h>
#include <fesapi/resqml2/WellboreInterpretation.h>
#include <fesapi/resqml2/WellboreTrajectoryRepresentation.h>

//----------------------------------------------------------------------------
WitsmlWellboreCompletionPerforationToVtkPolyData::WitsmlWellboreCompletionPerforationToVtkPolyData(WITSML2_1_NS::WellboreCompletion *completion, std::string connection_uid, int proc_number, int max_proc)
	: CommonAbstractObjectToVtkPartitionedDataSet(completion,
												  proc_number,
												  max_proc)
{
	for (auto *interpretation : completion->getWellbore()->getResqmlWellboreFeature(0)->getInterpretationSet())
	{
		if (dynamic_cast<RESQML2_NS::WellboreInterpretation *>(interpretation) != nullptr)
		{
			auto *wellbore_interpretation = static_cast<resqml2::WellboreInterpretation *>(interpretation);
			this->wellboreTrajectory = wellbore_interpretation->getWellboreTrajectoryRepresentation(0);
			continue;
		}
	}

	// Iterate over the perforations.
	for (uint64_t perforationIndex = 0; perforationIndex < completion->getConnectionCount(WITSML2_1_NS::WellboreCompletion::WellReservoirConnectionType::PERFORATION); ++perforationIndex)
	{
		if (vtkDataAssembly::MakeValidNodeName(("_" + completion->getConnectionUid(WITSML2_1_NS::WellboreCompletion::WellReservoirConnectionType::PERFORATION, perforationIndex)).c_str()) == connection_uid)
		{
			this->index = perforationIndex;
			continue;
		}
	}

	this->vtkData = vtkSmartPointer<vtkPartitionedDataSet>::New();
	this->vtkData->Modified();
}

// This function loads a vtkPolyData object from a WITSML WellboreCompletion perforation object.
//
// Args:
//   wellboreTrajectory: The wellbore trajectory object.
//   wellboreCompletion: The wellbore completion object.
//
// Returns:
//   The vtkPolyData object.
void WitsmlWellboreCompletionPerforationToVtkPolyData::loadVtkObject()
{

	// Check that the wellboreTrajectory is valid.
	if (wellboreTrajectory == nullptr)
	{
		vtkOutputWindowDisplayErrorText("Cannot compute the XYZ points of the frame without a valid wellbore trajectory.");
		return;
	}

	// Get the MD datum.
	auto mdDatum = wellboreTrajectory->getMdDatum();
	if (mdDatum == nullptr || mdDatum->isPartial())
	{
		vtkOutputWindowDisplayErrorText("Cannot compute the XYZ points of the frame without the MD datum.");
		return;
	}

	// Get the wellbore completion object.
	const WITSML2_1_NS::WellboreCompletion *wellbore_completion = getResqmlData();
	if (wellbore_completion == nullptr)
	{
		vtkOutputWindowDisplayErrorText("Cannot get the WITSML WellboreCompletion object.");
		return;
	}

	// Create a vector to store the perforations.
	vtkSmartPointer<vtkPolyData> perforationPolyData;

	// Check that the perforation has an MD interval.
	if (!wellbore_completion->hasConnectionMdInterval(WITSML2_1_NS::WellboreCompletion::WellReservoirConnectionType::PERFORATION, this->index))
	{
		return;
	}

	// Get the MD values for the top and bottom of the perforation.
	const double top = wellbore_completion->getConnectionTopMd(WITSML2_1_NS::WellboreCompletion::WellReservoirConnectionType::PERFORATION, this->index);
	const double base = wellbore_completion->getConnectionBaseMd(WITSML2_1_NS::WellboreCompletion::WellReservoirConnectionType::PERFORATION, this->index);

	// Convert the MD values to XYZ values.
	std::array<double, 2> mdTrajValues = {top, base};
	std::array<double, 3 * mdTrajValues.size()> xyzTrajValues;
	wellboreTrajectory->convertMdValuesToXyzValues(mdTrajValues.data(), mdTrajValues.size(), xyzTrajValues.data());

	// Get the MD and XYZ values for all the patches in the wellbore trajectory.
	const uint64_t pointCount = wellboreTrajectory->getXyzPointCountOfAllPatches(); // std::vector<double> mdValues(wellboreTrajectory->getXyzPointCountOfAllPatches());
	std::unique_ptr<double[]> mdValues = std::make_unique<double[]>(pointCount);
	wellboreTrajectory->getMdValues(mdValues.get());

	std::unique_ptr<double[]> xyzPoints = std::make_unique<double[]>(pointCount * 3);
	wellboreTrajectory->getXyzPointsOfAllPatchesInGlobalCrs(xyzPoints.get());

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
	vtkSmartPointer<vtkPolyData> polyData = vtkSmartPointer<vtkPolyData>::New();
	polyData->SetPoints(vtkPts);
	polyData->SetLines(lines);

	// Create a vtkTubeFilter object.
	vtkSmartPointer<vtkTubeFilter> tubeFilter = vtkSmartPointer<vtkTubeFilter>::New();
	tubeFilter->SetInputData(polyData);
	tubeFilter->SetNumberOfSides(10);
	tubeFilter->SetRadius(10);
	tubeFilter->SetVaryRadiusToVaryRadiusByScalar();
	tubeFilter->Update();

	// Add the polyData to the vector.
	//perforationPolyData

	// Update the vtkPartitionedDataSet.
	this->vtkData->Modified();
}