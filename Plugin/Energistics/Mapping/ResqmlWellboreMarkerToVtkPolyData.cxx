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
#include "Mapping/ResqmlWellboreMarkerToVtkPolyData.h"

#include <iostream>

// include VTK library
#include <vtkMath.h>
#include <vtkTransform.h>
#include <vtkTransformPolyDataFilter.h>
#include <vtkDiskSource.h>
#include <vtkSphereSource.h>
#include <vtkPolyData.h>

#include <fesapi/resqml2/WellboreMarker.h>
#include <fesapi/resqml2/WellboreMarkerFrameRepresentation.h>
#include <fesapi/resqml2/AbstractLocal3dCrs.h>

//----------------------------------------------------------------------------
ResqmlWellboreMarkerToVtkPolyData::ResqmlWellboreMarkerToVtkPolyData(const resqml2::WellboreMarkerFrameRepresentation *p_markerFrame, std::string p_uuid, bool p_orientation, uint32_t p_size, uint32_t p_procNumber, uint32_t p_maxProc)
	: ResqmlAbstractRepresentationToVtkPartitionedDataSet(p_markerFrame,
														  p_procNumber,
														  p_maxProc),
	  _orientation(p_orientation),
	  _size(p_size)
{
	_vtkData = vtkSmartPointer<vtkPartitionedDataSet>::New();
	_vtkData->Modified();

	setUuid(p_uuid);
}

//----------------------------------------------------------------------------
const RESQML2_NS::WellboreMarkerFrameRepresentation *ResqmlWellboreMarkerToVtkPolyData::getResqmlData() const
{
	return static_cast<const RESQML2_NS::WellboreMarkerFrameRepresentation *>(_resqmlData);
}

//----------------------------------------------------------------------------
void ResqmlWellboreMarkerToVtkPolyData::loadVtkObject()
{
	std::vector<RESQML2_NS::WellboreMarker *> w_markerSet = getResqmlData()->getWellboreMarkerSet();
	// search Marker
	for (unsigned int w_mIndex = 0; w_mIndex < w_markerSet.size(); ++w_mIndex)
	{
		if (w_markerSet[w_mIndex]->getUuid() == getUuid())
		{
			setTitle("Marker_"+ w_markerSet[w_mIndex]->getTitle());
			std::unique_ptr<double[]> w_doublePositions(new double[getResqmlData()->getMdValuesCount() * 3]);
			getResqmlData()->getXyzPointsOfPatch(0, w_doublePositions.get());

			if (_orientation)
			{
				if (!std::isnan(w_doublePositions[3 * w_mIndex]) &&
					!std::isnan(w_doublePositions[3 * w_mIndex + 1]) &&
					!std::isnan(w_doublePositions[3 * w_mIndex + 2]))
				{ // no NaN Value
					if (w_markerSet[w_mIndex]->hasDipAngle() &&
						w_markerSet[w_mIndex]->hasDipDirection())
					{ // dips & direction exist
						createDisk(w_mIndex);
					}
					else
					{
						createSphere(w_mIndex);
					}
				}
			}
			else
			{
				createSphere(w_mIndex);
			}
		}
	}
}

namespace
{
	double convertToDegree(double value, gsoap_eml2_3::eml23__PlaneAngleUom uom)
	{
		switch (uom)
		{
		case gsoap_eml2_3::eml23__PlaneAngleUom::ccgr:
		case gsoap_eml2_3::eml23__PlaneAngleUom::cgr:
			break;
		case gsoap_eml2_3::eml23__PlaneAngleUom::dega:
			return value;
		case gsoap_eml2_3::eml23__PlaneAngleUom::gon:
			return value * 0.9;
		case gsoap_eml2_3::eml23__PlaneAngleUom::krad:
			return value * 1e3 * 180 / vtkMath::Pi();
		case gsoap_eml2_3::eml23__PlaneAngleUom::mila:
			return value * 0.0573;
		case gsoap_eml2_3::eml23__PlaneAngleUom::mina:
			return value * 0.01666667;
		case gsoap_eml2_3::eml23__PlaneAngleUom::Mrad:
			return value * 1e6 * 180 / vtkMath::Pi();
		case gsoap_eml2_3::eml23__PlaneAngleUom::mrad:
			return value * 1e-3 * 180 / vtkMath::Pi();
		case gsoap_eml2_3::eml23__PlaneAngleUom::rad:
			return value * 180 / vtkMath::Pi();
		case gsoap_eml2_3::eml23__PlaneAngleUom::rev:
			return value * 360;
		case gsoap_eml2_3::eml23__PlaneAngleUom::seca:
			return value * 0.0002777778;
		case gsoap_eml2_3::eml23__PlaneAngleUom::urad:
			return value * 1e-6 * 180 / vtkMath::Pi();
		}

		vtkOutputWindowDisplayErrorText("The uom of the dip of the marker is not recognized.\n");
		return std::numeric_limits<double>::quiet_NaN();
	}
}

//----------------------------------------------------------------------------
void ResqmlWellboreMarkerToVtkPolyData::createDisk(uint32_t markerIndex)
{
	std::unique_ptr<double[]> doublePositions(new double[getResqmlData()->getMdValuesCount() * 3]);
	getResqmlData()->getXyzPointsOfPatch(0, doublePositions.get());

	// initialize a disk
	vtkSmartPointer<vtkDiskSource> diskSource = vtkSmartPointer<vtkDiskSource>::New();
	diskSource->SetInnerRadius(0);
	diskSource->SetOuterRadius(_size);
	diskSource->Update();

	vtkSmartPointer<vtkPolyData> vtkPolydata = diskSource->GetOutput();

	// get markerSet
	std::vector<RESQML2_NS::WellboreMarker *> markerSet = getResqmlData()->getWellboreMarkerSet();

	// disk orientation with dipAngle & dip Direction
	vtkSmartPointer<vtkTransform> rotation = vtkSmartPointer<vtkTransform>::New();
	double dipDirectionInDegree = convertToDegree(markerSet[markerIndex]->getDipDirectionValue(), markerSet[markerIndex]->getDipDirectionUom());
	rotation->RotateZ(-dipDirectionInDegree);
	double dipAngleInDegree = convertToDegree(markerSet[markerIndex]->getDipAngleValue(), markerSet[markerIndex]->getDipAngleUom());
	rotation->RotateX(-dipAngleInDegree);

	vtkSmartPointer<vtkTransformPolyDataFilter> transformFilter = vtkSmartPointer<vtkTransformPolyDataFilter>::New();
	transformFilter->SetInputData(vtkPolydata);
	transformFilter->SetTransform(rotation);
	transformFilter->Update();

	vtkPolydata = transformFilter->GetOutput();

	// disk translation with marker position
	const double zIndice = getResqmlData()->getLocalCrs(0)->isDepthOriented() ? -1 : 1;
	vtkSmartPointer<vtkTransform> translation = vtkSmartPointer<vtkTransform>::New();
	translation->Translate(doublePositions[3 * markerIndex], doublePositions[3 * markerIndex + 1], zIndice * doublePositions[3 * markerIndex + 2]);

	transformFilter = vtkSmartPointer<vtkTransformPolyDataFilter>::New();
	transformFilter->SetInputData(vtkPolydata);
	transformFilter->SetTransform(translation);
	transformFilter->Update();

	_vtkData->SetPartition(0, transformFilter->GetOutput());
}

//----------------------------------------------------------------------------
void ResqmlWellboreMarkerToVtkPolyData::createSphere(uint32_t markerIndex)
{
	std::unique_ptr<double[]> doublePositions(new double[getResqmlData()->getMdValuesCount() * 3]);
	getResqmlData()->getXyzPointsOfPatch(0, doublePositions.get());

	// get markerSet
	const double zIndice = getResqmlData()->getLocalCrs(0)->isDepthOriented() ? -1 : 1;

	// create  sphere
	vtkSmartPointer<vtkSphereSource> sphereSource = vtkSmartPointer<vtkSphereSource>::New();
	sphereSource->SetCenter(doublePositions[3 * markerIndex], doublePositions[3 * markerIndex + 1], zIndice * doublePositions[3 * markerIndex + 2]);
	sphereSource->SetRadius(_size);
	sphereSource->Update();

	_vtkData->SetPartition(0, sphereSource->GetOutput());
}
