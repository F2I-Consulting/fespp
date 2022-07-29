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
ResqmlWellboreMarkerToVtkPolyData::ResqmlWellboreMarkerToVtkPolyData(RESQML2_NS::WellboreMarkerFrameRepresentation *marker_frame, std::string uuid, bool orientation, int size, int proc_number, int max_proc)
	: ResqmlAbstractRepresentationToVtkDataset(marker_frame,
											   proc_number - 1,
											   max_proc),
	  orientation(orientation),
	  size(size),
	  uuid(uuid),
	  title(""),
	  resqmlData(marker_frame)
{
	this->vtkData = vtkSmartPointer<vtkPartitionedDataSet>::New();
	this->loadVtkObject();
	this->vtkData->Modified();
}

//----------------------------------------------------------------------------
void ResqmlWellboreMarkerToVtkPolyData::loadVtkObject()
{
	std::vector<RESQML2_NS::WellboreMarker *> markerSet = this->resqmlData->getWellboreMarkerSet();
	// search Marker
	for (unsigned int mIndex = 0; mIndex < markerSet.size(); ++mIndex)
	{
		if (markerSet[mIndex]->getUuid() == this->uuid)
		{
			this->title = markerSet[mIndex]->getTitle();
			std::unique_ptr<double[]> doublePositions(new double[this->resqmlData->getMdValuesCount() * 3]);
			this->resqmlData->getXyzPointsOfPatch(0, doublePositions.get());

			if (orientation)
			{
				if (!std::isnan(doublePositions[3 * mIndex]) &&
					!std::isnan(doublePositions[3 * mIndex + 1]) &&
					!std::isnan(doublePositions[3 * mIndex + 2]))
				{ // no NaN Value
					if (markerSet[mIndex]->hasDipAngle() &&
						markerSet[mIndex]->hasDipDirection())
					{ // dips & direction exist
						createDisk(mIndex);
					}
					else
					{
						createSphere(mIndex);
					}
				}
			}
			else
			{
				createSphere(mIndex);
			}
		}
	}
}

//----------------------------------------------------------------------------
void ResqmlWellboreMarkerToVtkPolyData::displayOption(bool orientation, int size)
{
	this->orientation = orientation;
	this->size = size;
	this->vtkData = vtkSmartPointer<vtkPartitionedDataSet>::New();
	this->loadVtkObject();
}

namespace
{
	double convertToDegree(double value, gsoap_eml2_1::eml21__PlaneAngleUom uom)
	{
		switch (uom)
		{
			//				case gsoap_eml2_1::eml21__PlaneAngleUom::0_x002e001_x0020seca:
		case gsoap_eml2_1::eml21__PlaneAngleUom::ccgr:
		case gsoap_eml2_1::eml21__PlaneAngleUom::cgr:
			break;
		case gsoap_eml2_1::eml21__PlaneAngleUom::dega:
			return value;
		case gsoap_eml2_1::eml21__PlaneAngleUom::gon:
			return value * 0.9;
		case gsoap_eml2_1::eml21__PlaneAngleUom::krad:
			return value * 1e3 * 180 / vtkMath::Pi();
		case gsoap_eml2_1::eml21__PlaneAngleUom::mila:
			return value * 0.0573;
		case gsoap_eml2_1::eml21__PlaneAngleUom::mina:
			return value * 0.01666667;
		case gsoap_eml2_1::eml21__PlaneAngleUom::Mrad:
			return value * 1e6 * 180 / vtkMath::Pi();
		case gsoap_eml2_1::eml21__PlaneAngleUom::mrad:
			return value * 1e-3 * 180 / vtkMath::Pi();
		case gsoap_eml2_1::eml21__PlaneAngleUom::rad:
			return value * 180 / vtkMath::Pi();
		case gsoap_eml2_1::eml21__PlaneAngleUom::rev:
			return value * 360;
		case gsoap_eml2_1::eml21__PlaneAngleUom::seca:
			return value * 0.0002777778;
		case gsoap_eml2_1::eml21__PlaneAngleUom::urad:
			return value * 1e-6 * 180 / vtkMath::Pi();
		}

		vtkOutputWindowDisplayErrorText("The uom of the dip of the marker is not recognized.\n");
		return std::numeric_limits<double>::quiet_NaN();
	}
}

//----------------------------------------------------------------------------
void ResqmlWellboreMarkerToVtkPolyData::createDisk(unsigned int markerIndex)
{
	vtkSmartPointer<vtkPolyData> vtkPolydata = vtkSmartPointer<vtkPolyData>::New();

	std::unique_ptr<double[]> doublePositions(new double[this->resqmlData->getMdValuesCount() * 3]);
	this->resqmlData->getXyzPointsOfPatch(0, doublePositions.get());

	// initialize a disk
	vtkSmartPointer<vtkDiskSource> diskSource = vtkSmartPointer<vtkDiskSource>::New();
	diskSource->SetInnerRadius(0);
	diskSource->SetOuterRadius(this->size);
	diskSource->Update();

	vtkPolydata = diskSource->GetOutput();

	// get markerSet
	std::vector<RESQML2_NS::WellboreMarker *> markerSet = this->resqmlData->getWellboreMarkerSet();

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
	const double zIndice = this->resqmlData->getLocalCrs(0)->isDepthOriented() ? -1 : 1;
	vtkSmartPointer<vtkTransform> translation = vtkSmartPointer<vtkTransform>::New();
	translation->Translate(doublePositions[3 * markerIndex], doublePositions[3 * markerIndex + 1], zIndice * doublePositions[3 * markerIndex + 2]);

	transformFilter = vtkSmartPointer<vtkTransformPolyDataFilter>::New();
	transformFilter->SetInputData(vtkPolydata);
	transformFilter->SetTransform(translation);
	transformFilter->Update();

	this->vtkData->SetPartition(0, transformFilter->GetOutput());
}

//----------------------------------------------------------------------------
void ResqmlWellboreMarkerToVtkPolyData::createSphere(unsigned int markerIndex)
{
	std::unique_ptr<double[]> doublePositions(new double[this->resqmlData->getMdValuesCount() * 3]);
	this->resqmlData->getXyzPointsOfPatch(0, doublePositions.get());

	// get markerSet
	const double zIndice = this->resqmlData->getLocalCrs(0)->isDepthOriented() ? -1 : 1;

	// create  sphere
	vtkSmartPointer<vtkSphereSource> sphereSource = vtkSmartPointer<vtkSphereSource>::New();
	sphereSource->SetCenter(doublePositions[3 * markerIndex], doublePositions[3 * markerIndex + 1], zIndice * doublePositions[3 * markerIndex + 2]);
	sphereSource->SetRadius(size);
	sphereSource->Update();

	this->vtkData->SetPartition(0, sphereSource->GetOutput());
}
