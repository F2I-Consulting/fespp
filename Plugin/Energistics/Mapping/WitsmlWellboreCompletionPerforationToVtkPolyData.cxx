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
#include <vtkInformation.h>
#include <vtkUnsignedIntArray.h>
#include <vtkColorTransferFunction.h>
#include <vtkLookupTable.h>
#include <vtkVariantArray.h>

#include <vtkCellData.h>

#include <fesapi/eml2/AbstractLocal3dCrs.h>
#include <fesapi/eml2/GraphicalInformationSet.h>
#include <fesapi/witsml2_1/WellboreCompletion.h>
#include <fesapi/resqml2/MdDatum.h>
#include <fesapi/resqml2/WellboreFeature.h>
#include <fesapi/resqml2/WellboreInterpretation.h>
#include <fesapi/resqml2/WellboreTrajectoryRepresentation.h>

WitsmlWellboreCompletionPerforationToVtkPolyData::WitsmlWellboreCompletionPerforationToVtkPolyData(const resqml2::WellboreTrajectoryRepresentation *wellboreTrajectory, const WITSML2_1_NS::WellboreCompletion *wellboreCompletion, const std::string &connectionuid, const std::string &title, const double skin, uint32_t p_procNumber, uint32_t p_maxProc)
	: CommonAbstractObjectToVtkPartitionedDataSet(wellboreCompletion,
												  p_procNumber,
												  p_maxProc),
	  wellboreCompletion(wellboreCompletion),
	  wellboreTrajectory(wellboreTrajectory),
	  title(title),
	  connectionuid(connectionuid),
	  skin(skin)
{
	// Check that the completion is valid.
	if (wellboreCompletion == nullptr)
	{
		vtkOutputWindowDisplayErrorText("Cannot compute the XYZ points of the frame without a valid wellbore completion.");
		return;
	}

	// Iterate over the perforations.
	for (uint64_t perforationIndex = 0; perforationIndex < wellboreCompletion->getConnectionCount(WITSML2_1_NS::WellboreCompletion::WellReservoirConnectionType::PERFORATION); ++perforationIndex)
	{
		if (connectionuid == wellboreCompletion->getConnectionUid(WITSML2_1_NS::WellboreCompletion::WellReservoirConnectionType::PERFORATION, perforationIndex))
		{
			this->index = perforationIndex;
		}
	}

	_vtkData = vtkSmartPointer<vtkPartitionedDataSet>::New();
	_vtkData->Modified();

	setUuid(connectionuid);
	setTitle(title);

	// this->loadVtkObject();
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

	// Check that the perforation has an MD interval.
	if (!this->wellboreCompletion->hasConnectionMdInterval(WITSML2_1_NS::WellboreCompletion::WellReservoirConnectionType::PERFORATION, this->index))
	{
		return;
	}

	// Get the MD values for the top and bottom of the perforation.
	const double top = this->wellboreCompletion->getConnectionTopMd(WITSML2_1_NS::WellboreCompletion::WellReservoirConnectionType::PERFORATION, this->index);
	const double base = this->wellboreCompletion->getConnectionBaseMd(WITSML2_1_NS::WellboreCompletion::WellReservoirConnectionType::PERFORATION, this->index);

	// Convert the MD values to XYZ values.
	std::array<double, 2> mdTrajValues = {top, base};
	std::array<double, 3 * mdTrajValues.size()> xyzTrajValues;
	this->wellboreTrajectory->convertMdValuesToXyzValues(mdTrajValues.data(), mdTrajValues.size(), xyzTrajValues.data());

	// Get the MD and XYZ values for all the patches in the wellbore trajectory.
	const uint64_t w_pointCount = this->wellboreTrajectory->getXyzPointCountOfAllPatches(); // std::vector<double> mdValues(wellboreTrajectory->getXyzPointCountOfAllPatches());
	std::unique_ptr<double[]> mdValues(new double[w_pointCount]);
	this->wellboreTrajectory->getMdValues(mdValues.get());

	std::unique_ptr<double[]> xyzPoints(new double[w_pointCount * 3]);
	this->wellboreTrajectory->getXyzPointsOfAllPatchesInGlobalCrs(xyzPoints.get());

	// Create a list of intermediate MD points.
	std::vector<std::array<double, 3>> intermediateMdPoints;
	for (size_t i = 0; i < w_pointCount; ++i)
	{
		if (top <= mdValues[i] && mdValues[i] <= base)
		{
			intermediateMdPoints.push_back({{xyzPoints[i * 3], xyzPoints[i * 3 + 1], xyzPoints[i * 3 + 2]}});
		}
	}

	const double depthOriented = wellboreTrajectory->getLocalCrs(0)->isDepthOriented() ? -1 : 1;
	// Create a vtkPoints object.
	vtkSmartPointer<vtkPoints> vtkPts = vtkSmartPointer<vtkPoints>::New();

	// Add the top point.
	vtkPts->InsertNextPoint(xyzTrajValues[0], xyzTrajValues[1], xyzTrajValues[2] * depthOriented);

	// Add the intermediate points.
	for (auto const &point : intermediateMdPoints)
	{
		vtkPts->InsertNextPoint(point[0], point[1], point[2] * depthOriented);
	}

	// Add the base point.
	vtkPts->InsertNextPoint(xyzTrajValues[3], xyzTrajValues[4], xyzTrajValues[5] * depthOriented);

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
	_vtkData->SetPartition(0, tubeFilter->GetOutput());
	_vtkData->GetMetaData((unsigned int)0)->Set(vtkCompositeDataSet::NAME(), (const char *)(this->connectionuid + "_" + this->title).c_str());
	_vtkData->Modified();

	this->addSkin();
}

void WitsmlWellboreCompletionPerforationToVtkPolyData::addSkin()
{

	vtkSmartPointer<vtkDoubleArray> array = vtkSmartPointer<vtkDoubleArray>::New();
	array->SetName("Skin");
	array->InsertNextValue(this->skin);

	_vtkData->GetPartition(0)->GetFieldData()->AddArray(array);
}
