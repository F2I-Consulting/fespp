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
#include "ResqmlMapping/ResqmlWellboreMarkerToVtkPolyData.h"

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
ResqmlWellboreMarkerToVtkPolyData::ResqmlWellboreMarkerToVtkPolyData(RESQML2_NS::WellboreMarkerFrameRepresentation *marker, int markerIndex, int proc_number, int max_proc)
	: ResqmlAbstractRepresentationToVtkDataset(marker,
											   proc_number,
											   max_proc)
{
	this->markerIndexInFrame = markerIndex;
	this->orientation = false;
}

//----------------------------------------------------------------------------
void ResqmlWellboreMarkerToVtkPolyData::loadVtkObject()
{
	auto *markerFrame = static_cast<RESQML2_NS::WellboreMarkerFrameRepresentation *>(this->resqmlData);

	std::vector<RESQML2_NS::WellboreMarker *> markerSet = markerFrame->getWellboreMarkerSet();
	std::unique_ptr<double[]> doublePositions(new double[markerFrame->getMdValuesCount() * 3]);
	markerFrame->getXyzPointsOfPatch(0, doublePositions.get());

	this->markerIndexInFrame = searchMarkerIndex();
	if (orientation)
	{
		if (!std::isnan(doublePositions[3 * this->markerIndexInFrame]) &&
			!std::isnan(doublePositions[3 * this->markerIndexInFrame + 1]) &&
			!std::isnan(doublePositions[3 * this->markerIndexInFrame + 2]))
		{ // no NaN Value
			if (markerSet[this->markerIndexInFrame]->hasDipAngle() &&
				markerSet[this->markerIndexInFrame]->hasDipDirection())
			{ // dips & direction exist
				createDisk(this->markerIndexInFrame);
			}
			else
			{
				createSphere(this->markerIndexInFrame);
			}
		}
	}
	else
	{
		createSphere(this->markerIndexInFrame);
	}
}

//----------------------------------------------------------------------------
void ResqmlWellboreMarkerToVtkPolyData::toggleMarkerOrientation(bool orient)
{
	orientation = orient;
}

//----------------------------------------------------------------------------
void ResqmlWellboreMarkerToVtkPolyData::setMarkerSize(int new_size)
{
	size = new_size;
}

namespace
{
	double convertToDegree(double value, gsoap_eml2_1::eml21__PlaneAngleUom uom)
	{
//		switch (uom)
//		{
//		case gsoap_eml2_1::eml21__PlaneAngleUom::0_x002e001_x0020seca:
//		case gsoap_eml2_1::eml21__PlaneAngleUom::ccgr:
//		case gsoap_eml2_1::eml21__PlaneAngleUom::cgr:
//			break;
//		case gsoap_eml2_1::eml21__PlaneAngleUom::dega:
			return value;
//		case gsoap_eml2_1::eml21__PlaneAngleUom::gon:
//			return value * 0.9;
//		case gsoap_eml2_1::eml21__PlaneAngleUom::krad:
//			return value * 1e3 * 180 / vtkMath::Pi();
//		case gsoap_eml2_1::eml21__PlaneAngleUom::mila:
//			return value * 0.0573;
//		case gsoap_eml2_1::eml21__PlaneAngleUom::mina:
//			return value * 0.01666667;
//		case gsoap_eml2_1::eml21__PlaneAngleUom::Mrad:
//			return value * 1e6 * 180 / vtkMath::Pi();
//		case gsoap_eml2_1::eml21__PlaneAngleUom::mrad:
//			return value * 1e-3 * 180 / vtkMath::Pi();
//		case gsoap_eml2_1::eml21__PlaneAngleUom::rad:
//			return value * 180 / vtkMath::Pi();
//		case gsoap_eml2_1::eml21__PlaneAngleUom::rev:
//			return value * 360;
//		case gsoap_eml2_1::eml21__PlaneAngleUom::seca:
//			return value * 0.0002777778;
//		case gsoap_eml2_1::eml21__PlaneAngleUom::urad:
//			return value * 1e-6 * 180 / vtkMath::Pi();
//		}
//
//		vtkOutputWindowDisplayErrorText("The uom of the dip of the marker is not recognized.");
//		return std::numeric_limits<double>::quiet_NaN();
	}
}

//----------------------------------------------------------------------------
void ResqmlWellboreMarkerToVtkPolyData::createDisk(size_t markerIndex)
{
	vtkSmartPointer<vtkPolyData> vtkPolydata = vtkSmartPointer<vtkPolyData>::New();

	auto *markerFrame = static_cast<RESQML2_NS::WellboreMarkerFrameRepresentation *>(this->resqmlData);

	std::unique_ptr<double[]> doublePositions(new double[markerFrame->getMdValuesCount() * 3]);
	markerFrame->getXyzPointsOfPatch(0, doublePositions.get());

	// initialize a disk
	vtkSmartPointer<vtkDiskSource> diskSource = vtkSmartPointer<vtkDiskSource>::New();
	diskSource->SetInnerRadius(0);
	diskSource->SetOuterRadius(size);
	diskSource->Update();

	vtkPolydata = diskSource->GetOutput();

	// get markerSet
	std::vector<RESQML2_NS::WellboreMarker *> markerSet = markerFrame->getWellboreMarkerSet();

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
	const double zIndice = markerFrame->getLocalCrs(0)->isDepthOriented() ? -1 : 1;
	vtkSmartPointer<vtkTransform> translation = vtkSmartPointer<vtkTransform>::New();
	translation->Translate(doublePositions[3 * markerIndex], doublePositions[3 * markerIndex + 1], zIndice * doublePositions[3 * markerIndex + 2]);

	transformFilter = vtkSmartPointer<vtkTransformPolyDataFilter>::New();
	transformFilter->SetInputData(vtkPolydata);
	transformFilter->SetTransform(translation);
	transformFilter->Update();

	this->vtkData->SetPartition(0, transformFilter->GetOutput());
}

//----------------------------------------------------------------------------
void ResqmlWellboreMarkerToVtkPolyData::createSphere(size_t markerIndex)
{
	auto *markerFrame = static_cast<RESQML2_NS::WellboreMarkerFrameRepresentation *>(this->resqmlData);

	std::unique_ptr<double[]> doublePositions(new double[markerFrame->getMdValuesCount() * 3]);
	markerFrame->getXyzPointsOfPatch(0, doublePositions.get());

	// get markerSet
//	RESQML2_NS::WellboreMarkerFrameRepresentation *frame = epcPackageRepresentation->getDataObjectByUuid<RESQML2_NS::WellboreMarkerFrameRepresentation>(getParent());
	const double zIndice = markerFrame->getLocalCrs(0)->isDepthOriented() ? -1 : 1;

	// create  sphere
	vtkSmartPointer<vtkSphereSource> sphereSource = vtkSmartPointer<vtkSphereSource>::New();
	sphereSource->SetCenter(doublePositions[3 * markerIndex], doublePositions[3 * markerIndex + 1], zIndice * doublePositions[3 * markerIndex + 2]);
	sphereSource->SetRadius(size);
	sphereSource->Update();

	this->vtkData->SetPartition(0, sphereSource->GetOutput());
}
