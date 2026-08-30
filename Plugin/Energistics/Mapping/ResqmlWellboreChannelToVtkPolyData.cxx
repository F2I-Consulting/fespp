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
#include "ResqmlPropertyToVtkDataArray.h"  // MakeValidNodeName (public static)

#include <vtkPointData.h>
#include <vtkTubeFilter.h>
#include <vtkDoubleArray.h>
#include <vtkIdTypeArray.h>
#include <vtkFieldData.h>
#include <vtkPolyLine.h>
#include <vtkInformation.h>

#include <fesapi/eml2/AbstractLocal3dCrs.h>
#include <fesapi/resqml2/WellboreFrameRepresentation.h>
#include <fesapi/resqml2/ContinuousProperty.h>
#include <fesapi/resqml2/DiscreteProperty.h>
#include <fesapi/resqml2/CategoricalProperty.h>
#include <fesapi/resqml2/AbstractValuesProperty.h>

//----------------------------------------------------------------------------
ResqmlWellboreChannelToVtkPolyData::ResqmlWellboreChannelToVtkPolyData(const RESQML2_NS::WellboreFrameRepresentation *frame, const RESQML2_NS::AbstractValuesProperty *property, const std::string &p_uuid, uint32_t p_procNumber, uint32_t p_maxProc)
	: ResqmlAbstractRepresentationToVtkPartitionedDataSet(frame,
														  p_procNumber,
														  p_maxProc),
	  _abstractProperty(property),
	  _uuid(p_uuid),
	  _title(property->getTitle())
{
	_vtkData = vtkSmartPointer<vtkPartitionedDataSet>::New();
	_vtkData->Modified();

	setUuid(property->getUuid());
	setTitle("Channel_"+property->getTitle());
}

//----------------------------------------------------------------------------
const RESQML2_NS::WellboreFrameRepresentation *ResqmlWellboreChannelToVtkPolyData::getResqmlData() const
{
	return static_cast<const RESQML2_NS::WellboreFrameRepresentation *>(_resqmlData);
}

//----------------------------------------------------------------------------
void ResqmlWellboreChannelToVtkPolyData::loadVtkObject()
{
	RESQML2_NS::WellboreFrameRepresentation const* frame = getResqmlData();

	// We need to build first a polyline for the channel to support the vtk tube.
	_pointCount = frame->getXyzPointCountOfPatch(0);
	double* allXyzPoints = new double[_pointCount * 3]; // Will be deleted by VTK
	frame->getXyzPointsOfAllPatchesInGlobalCrs(allXyzPoints);

	vtkSmartPointer<vtkPoints> vtkPts = vtkSmartPointer<vtkPoints>::New();
	const size_t coordCount =_pointCount * 3;
	if (frame->getLocalCrs(0)->isDepthOriented())
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

	auto lines = vtkSmartPointer<vtkCellArray>::New();
	lines->InsertNextCell(_pointCount);
	for (unsigned int i = 0; i < _pointCount; ++i)
	{
		lines->InsertCellPoint(i);
	}

	auto channelPolyline = vtkSmartPointer<vtkPolyData>::New();
	channelPolyline->SetPoints(vtkPts);
	channelPolyline->SetLines(lines);

	// Varying tube radius. The VTK ARRAY name MUST go through
	// MakeValidNodeName so it matches what the Python side computes via
	// make_valid_vtk_name(title) — exactly like grid/UG properties. The
	// human-readable title stays raw (assembly 'title' attribute,
	// partition metadata), so this changes no displayed label, only the
	// internal scalar-array identifier ParaView colours by.
	const std::string w_arrayName =
		ResqmlPropertyToVtkDataArray::MakeValidNodeName(_abstractProperty->getTitle().c_str());
	auto tubeRadius = vtkSmartPointer<vtkDoubleArray>::New();
	tubeRadius->SetName(w_arrayName.c_str());
	tubeRadius->SetNumberOfTuples(_pointCount);
	// A log gap (NaN) cannot stay NaN in this array: it is the vtkTubeFilter's
	// VaryRadiusByScalar input, and a NaN radius destroys the geometry. It is
	// written as NEGATIVE ZERO instead of plain 0. -0.0 == 0.0 numerically, so
	// the tube radius and the colour map behave exactly as before, but its
	// IEEE-754 bit pattern differs: a consumer that wants to plot the log as a
	// CURVE (a well-log track, where a gap must stay a gap) recovers the gaps
	// exactly with std::signbit / math.copysign — including on a discrete log
	// whose zero is a legitimate value, which no value-range heuristic can do.
	// Careful: the sign of a zero is erased by the first arithmetic on it
	// (v + 0.0, std::abs(v)). Read the array as it comes, do not massage it.
	vtkIdType w_nanCount = 0;
	if (dynamic_cast<RESQML2_NS::ContinuousProperty const*>(_abstractProperty) != nullptr)
	{
		std::unique_ptr<double[]> values(new double[_pointCount]);
		_abstractProperty->getArrayOfValuesOfPatch(0, values.get());
		for (unsigned int i = 0; i < _pointCount; ++i)
		{
			if (std::isnan(values[i]))
			{
				++w_nanCount;
				tubeRadius->SetTuple1(i, -0.0);
			}
			else
			{
				tubeRadius->SetTuple1(i, values[i]);
			}
		}
	}
	else if (dynamic_cast<const RESQML2_NS::DiscreteProperty *>(_abstractProperty) != nullptr || dynamic_cast<const RESQML2_NS::CategoricalProperty *>(_abstractProperty) != nullptr)
	{
		std::unique_ptr<int32_t[]> values(new int[_pointCount]);
		_abstractProperty->getArrayOfValuesOfPatch(0, values.get());
		for (uint64_t i = 0; i < _pointCount; ++i)
		{
			if (values[i] > (std::numeric_limits<int>::max)())
			{
				++w_nanCount;
				tubeRadius->SetTuple1(i, -0.0);
			}
			else
			{
				tubeRadius->SetTuple1(i, values[i]);
			}
		}
	}
	else
	{
		vtkOutputWindowDisplayErrorText("Cannot show a log which is not discrete, categorical no continuous.\n");
		return;
	}
	if (w_nanCount > 0)
	{
		vtkOutputWindowDisplayWarningText(("The log `" + _abstractProperty->getTitle() + "' has NaN values. NaN values are changed to negative zero.\n").c_str());
	}

	// How many gaps there SHOULD be, so a consumer can check that the negative
	// zeros survived the pipeline instead of trusting them: a mismatch means
	// the sentinel was erased somewhere, and silently drawing the gaps as real
	// zeros is worse than knowing the answer is unreliable. One tuple.
	auto w_nanCountArray = vtkSmartPointer<vtkIdTypeArray>::New();
	w_nanCountArray->SetName((w_arrayName + "_nanCount").c_str());
	w_nanCountArray->SetNumberOfTuples(1);
	w_nanCountArray->SetTuple1(0, w_nanCount);
	channelPolyline->GetFieldData()->AddArray(w_nanCountArray);

	channelPolyline->GetPointData()->AddArray(tubeRadius);
	// Activate the SAME (sanitized) name just set on the array — must stay
	// in lockstep with the SetName above, or the active-scalar lookup fails.
	channelPolyline->GetPointData()->SetActiveScalars(w_arrayName.c_str());

	// Build the tube
	vtkSmartPointer<vtkTubeFilter> tubeFilter = vtkSmartPointer<vtkTubeFilter>::New();
	tubeFilter->SetInputData(channelPolyline);
	tubeFilter->SetNumberOfSides(10);
	tubeFilter->SetRadius(10);
	tubeFilter->SetVaryRadiusToVaryRadiusByScalar();
	tubeFilter->Update();

	_vtkData->SetPartition(0, tubeFilter->GetOutput());
	_vtkData->GetMetaData((unsigned int)0)->Set(vtkCompositeDataSet::NAME(), (const char*)(_title+"("+_uuid + ")").c_str());
	_vtkData->Modified();
}
