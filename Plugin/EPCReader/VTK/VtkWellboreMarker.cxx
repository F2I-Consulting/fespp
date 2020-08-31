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
#include "VtkWellboreMarker.h"

#include <iostream>

#include <vtkMath.h>
#include <vtkTransform.h>
#include <vtkTransformPolyDataFilter.h>
#include <vtkDiskSource.h>
#include <vtkSphereSource.h>

#include <fesapi/resqml2/WellboreMarker.h>
#include <fesapi/resqml2/WellboreMarkerFrameRepresentation.h>

//----------------------------------------------------------------------------
VtkWellboreMarker::VtkWellboreMarker(const std::string & fileName, const std::string & name, const std::string & uuid, const std::string & uuidParent, const COMMON_NS::DataObjectRepository *repoRepresentation, const COMMON_NS::DataObjectRepository *repoSubRepresentation) :
VtkResqml2PolyData(fileName, name, uuid, uuidParent, repoRepresentation, repoSubRepresentation), orientation (true), size(10)
{}

//----------------------------------------------------------------------------
void VtkWellboreMarker::visualize(const std::string & uuid)
{
	if (uuid == getUuid()) {
		RESQML2_NS::WellboreMarkerFrameRepresentation *markerFrame = static_cast<RESQML2_NS::WellboreMarkerFrameRepresentation *>(epcPackageRepresentation->getDataObjectByUuid(getParent()));
		std::vector<RESQML2_NS::WellboreMarker *> markerSet = markerFrame->getWellboreMarkerSet();
		std::unique_ptr<double[]> doublePositions(new double[markerFrame->getMdValuesCount()*3]);
		markerFrame->getXyzPointsOfPatch(0, doublePositions.get());

		size_t marker_index = searchMarkerIndex();
		if (orientation) {
			if (!std::isnan(doublePositions[3*marker_index]) &&
				!std::isnan(doublePositions[3*marker_index+1]) &&
				!std::isnan(doublePositions[3*marker_index+2])) { // no NaN Value
				if (markerSet[marker_index]->hasDipAngle() &&
					markerSet[marker_index]->hasDipDirection()) { // dips & direction exist
					cout << markerSet[marker_index]->getDipDirectionUom() << endl;
					createDisk(marker_index);
				}
				else {
					createSphere(marker_index);
				}
			}
		}
		else {
			createSphere(marker_index);
		}
	}
}

//----------------------------------------------------------------------------
void VtkWellboreMarker::toggleMarkerOrientation(bool orient) {
	orientation = orient;
	visualize(getUuid());
}

//----------------------------------------------------------------------------
void VtkWellboreMarker::setMarkerSize(int new_size) {
	this->size = new_size;
	visualize(getUuid());
}

//----------------------------------------------------------------------------
size_t VtkWellboreMarker::searchMarkerIndex(){
	// search Marker
	RESQML2_NS::WellboreMarkerFrameRepresentation *frame = static_cast<RESQML2_NS::WellboreMarkerFrameRepresentation *>(epcPackageRepresentation->getDataObjectByUuid(getParent()));
	std::vector<RESQML2_NS::WellboreMarker *> markerSet = frame->getWellboreMarkerSet();
	for (size_t mIndex = 0; mIndex < markerSet.size(); ++mIndex) {
		if (markerSet[mIndex]->getUuid() == getUuid() ){
			return mIndex;
		}
	}
	return (std::numeric_limits<size_t>::max)();
}

namespace {
	double convertToDegree(double value, gsoap_eml2_1::eml21__PlaneAngleUom uom) {
		switch (uom) {
		case gsoap_eml2_1::eml21__PlaneAngleUom__0_x002e001_x0020seca:
		case gsoap_eml2_1::eml21__PlaneAngleUom__ccgr:
		case gsoap_eml2_1::eml21__PlaneAngleUom__cgr: break;
		case gsoap_eml2_1::eml21__PlaneAngleUom__dega: return value;
		case gsoap_eml2_1::eml21__PlaneAngleUom__gon: return value * 0.9;
		case gsoap_eml2_1::eml21__PlaneAngleUom__krad: return value * 1e3 * 180 / vtkMath::Pi();
		case gsoap_eml2_1::eml21__PlaneAngleUom__mila: return value * 0.0573;
		case gsoap_eml2_1::eml21__PlaneAngleUom__mina: return value * 0.01666667;
		case gsoap_eml2_1::eml21__PlaneAngleUom__Mrad: return value * 1e6 * 180 / vtkMath::Pi();
		case gsoap_eml2_1::eml21__PlaneAngleUom__mrad: return value * 1e-3 * 180 / vtkMath::Pi();
		case gsoap_eml2_1::eml21__PlaneAngleUom__rad: return value * 180 / vtkMath::Pi();
		case gsoap_eml2_1::eml21__PlaneAngleUom__rev: return value * 360;
		case gsoap_eml2_1::eml21__PlaneAngleUom__seca: return value * 0.0002777778;
		case gsoap_eml2_1::eml21__PlaneAngleUom__urad: return value * 1e-6 * 180 / vtkMath::Pi();
		}

		vtkOutputWindowDisplayErrorText("The uom of the dip of the marker is not recognized.");
		return std::numeric_limits<double>::quiet_NaN();
	}
}

//----------------------------------------------------------------------------
void VtkWellboreMarker::createDisk(size_t markerIndex) {
	RESQML2_NS::WellboreMarkerFrameRepresentation *markerFrame = static_cast<RESQML2_NS::WellboreMarkerFrameRepresentation *>(epcPackageRepresentation->getDataObjectByUuid(getParent()));

	std::unique_ptr<double[]> doublePositions(new double[markerFrame->getMdValuesCount()*3]);
	markerFrame->getXyzPointsOfPatch(0, doublePositions.get());

	//initialize a disk
	vtkSmartPointer<vtkDiskSource> diskSource = vtkSmartPointer<vtkDiskSource>::New();
	diskSource->SetInnerRadius(0);
	diskSource->SetOuterRadius(size);
	diskSource->Update();

	vtkOutput = diskSource->GetOutput();

	// get markerSet
	RESQML2_NS::WellboreMarkerFrameRepresentation *frame = static_cast<RESQML2_NS::WellboreMarkerFrameRepresentation *>(epcPackageRepresentation->getDataObjectByUuid(getParent()));
	std::vector<RESQML2_NS::WellboreMarker *> markerSet = frame->getWellboreMarkerSet();

	// disk orientation with dipAngle & dip Direction
	vtkSmartPointer<vtkTransform> rotation = vtkSmartPointer<vtkTransform>::New();
	rotation->RotateY(convertToDegree(markerSet[markerIndex]->getDipAngleValue(), markerSet[markerIndex]->getDipAngleUom()));
	rotation->RotateZ(90 - convertToDegree(markerSet[markerIndex]->getDipDirectionValue(), markerSet[markerIndex]->getDipDirectionUom())); // The strike direction is perpendicular to the dip direction

	vtkSmartPointer<vtkTransformPolyDataFilter> transformFilter = vtkSmartPointer<vtkTransformPolyDataFilter>::New();
	transformFilter->SetInputData(vtkOutput);
	transformFilter->SetTransform(rotation);
	transformFilter->Update();

	vtkOutput = transformFilter->GetOutput();

	// disk translation with marker position
	const double zIndice = frame->getLocalCrs(0)->isDepthOriented() ? -1 : 1;
	vtkSmartPointer<vtkTransform> translation = vtkSmartPointer<vtkTransform>::New();
	translation->Translate(doublePositions[3*markerIndex], doublePositions[3*markerIndex+1], zIndice*doublePositions[3*markerIndex+2]);

	transformFilter = vtkSmartPointer<vtkTransformPolyDataFilter>::New();
	transformFilter->SetInputData(vtkOutput);
	transformFilter->SetTransform(translation);
	transformFilter->Update();

	vtkOutput = transformFilter->GetOutput();
}

//----------------------------------------------------------------------------
void VtkWellboreMarker::createSphere(size_t markerIndex) {
	RESQML2_NS::WellboreMarkerFrameRepresentation *markerFrame = static_cast<RESQML2_NS::WellboreMarkerFrameRepresentation *>(epcPackageRepresentation->getDataObjectByUuid(getParent()));

	std::unique_ptr<double[]> doublePositions(new double[markerFrame->getMdValuesCount()*3]);
	markerFrame->getXyzPointsOfPatch(0, doublePositions.get());

	// get markerSet
	RESQML2_NS::WellboreMarkerFrameRepresentation *frame = static_cast<RESQML2_NS::WellboreMarkerFrameRepresentation *>(epcPackageRepresentation->getDataObjectByUuid(getParent()));
	std::vector<RESQML2_NS::WellboreMarker *> markerSet = frame->getWellboreMarkerSet();
	const double zIndice = frame->getLocalCrs(0)->isDepthOriented() ? -1 : 1;

	//create  sphere
	vtkSmartPointer<vtkSphereSource> sphereSource = vtkSmartPointer<vtkSphereSource>::New();
	sphereSource->SetCenter(doublePositions[3*markerIndex], doublePositions[3*markerIndex+1], zIndice*doublePositions[3*markerIndex+2]);
	sphereSource->SetRadius(size);
	sphereSource->Update();

	vtkOutput = sphereSource->GetOutput();
}

//----------------------------------------------------------------------------
void VtkWellboreMarker::remove(const std::string & uuid)
{
	if (uuid == getUuid()) {
		vtkOutput = nullptr;
	}
}
