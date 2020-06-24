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


#include <vtkTransform.h>
#include <vtkTransformPolyDataFilter.h>
#include <vtkDiskSource.h>
#include <vtkSphereSource.h>

#include <fesapi/resqml2_0_1/WellboreMarkerFrameRepresentation.h>

//----------------------------------------------------------------------------
VtkWellboreMarker::VtkWellboreMarker(const std::string & fileName, const std::string & name, const std::string & uuid, const std::string & uuidParent, const COMMON_NS::DataObjectRepository *repoRepresentation, const COMMON_NS::DataObjectRepository *repoSubRepresentation) :
VtkResqml2PolyData(fileName, name, uuid, uuidParent, repoRepresentation, repoSubRepresentation)
{
	orientation = true;
	size = 10;
}

//----------------------------------------------------------------------------
VtkWellboreMarker::~VtkWellboreMarker()
{
	vtkOutput = nullptr;
}

//----------------------------------------------------------------------------
void VtkWellboreMarker::createOutput(const std::string & uuid)
{
	if (uuid == this->getUuid()) {
		if (orientation) {
			createDisk();

		} else {
			createSphere();

		}
	}
}

//----------------------------------------------------------------------------
void VtkWellboreMarker::toggleMarkerOrientation(const bool & orient) {

	orientation = orient;

	if (vtkOutput != nullptr) {
		createOutput(getUuid());
		/******* TODO
		 * Call enable/disable marker orientation
		 * use : vtkTransform
		 */
	}
}

//----------------------------------------------------------------------------
void VtkWellboreMarker::createDisk() {
	// create: Disk
	vtkSmartPointer<vtkDiskSource> diskSource = vtkSmartPointer<vtkDiskSource>::New();
	diskSource->SetInnerRadius(0);
	diskSource->SetOuterRadius(size);
	diskSource->Update();

	vtkOutput = diskSource->GetOutput();

	// apply dips/azimuth (rotation)
	if (orientation) {
		applyOrientation();

		/******* TODO
		 * if NaN Value => call createSphere + return;
		 */
	}

	// apply coordonate (translation)
	RESQML2_0_1_NS::WellboreMarkerFrameRepresentation *frame = static_cast<RESQML2_0_1_NS::WellboreMarkerFrameRepresentation *>(epcPackageRepresentation->getDataObjectByUuid(this->getParent()));
	std::vector<RESQML2_0_1_NS::WellboreMarker *> markerSet = frame->getWellboreMarkerSet();
	double* doublePositions = new double[frame->getMdValuesCount()*3];
	frame->getXyzPointsOfPatch(0,doublePositions);
	for (size_t mIndex = 0; mIndex < markerSet.size(); ++mIndex) {
		if (markerSet[mIndex]->getUuid() == getUuid() ){
			const double zIndice = frame->getLocalCrs(0)->isDepthOriented() ? -1 : 1;

			vtkSmartPointer<vtkTransform> translation = vtkSmartPointer<vtkTransform>::New();
			translation->Translate(doublePositions[3*mIndex], doublePositions[3*mIndex+1], zIndice*doublePositions[3*mIndex+2]);

			vtkSmartPointer<vtkTransformPolyDataFilter> transformFilter = vtkSmartPointer<vtkTransformPolyDataFilter>::New();
			transformFilter->SetInputData(vtkOutput);
			transformFilter->SetTransform(translation);
			transformFilter->Update();

			vtkOutput = transformFilter->GetOutput();
			break;
		}
	}
	delete[] doublePositions;
}

//----------------------------------------------------------------------------
void VtkWellboreMarker::createSphere() {
	vtkSmartPointer<vtkSphereSource> sphereSource = vtkSmartPointer<vtkSphereSource>::New();
	sphereSource->SetCenter(0.0, 0.0, 0.0);
	sphereSource->SetRadius(size);
	sphereSource->Update();

	vtkOutput = sphereSource->GetOutput();

	RESQML2_0_1_NS::WellboreMarkerFrameRepresentation *frame = static_cast<RESQML2_0_1_NS::WellboreMarkerFrameRepresentation *>(epcPackageRepresentation->getDataObjectByUuid(this->getParent()));
	std::vector<RESQML2_0_1_NS::WellboreMarker *> markerSet = frame->getWellboreMarkerSet();
	double* doublePositions = new double[frame->getMdValuesCount()*3];
	frame->getXyzPointsOfPatch(0,doublePositions);
	for (size_t mIndex = 0; mIndex < markerSet.size(); ++mIndex) {
		if (markerSet[mIndex]->getUuid() == getUuid() ){
			const double zIndice = frame->getLocalCrs(0)->isDepthOriented() ? -1 : 1;

			vtkSmartPointer<vtkTransform> translation = vtkSmartPointer<vtkTransform>::New();
			translation->Translate(doublePositions[3*mIndex], doublePositions[3*mIndex+1], zIndice*doublePositions[3*mIndex+2]);

			vtkSmartPointer<vtkTransformPolyDataFilter> transformFilter = vtkSmartPointer<vtkTransformPolyDataFilter>::New();
			transformFilter->SetInputData(vtkOutput);
			transformFilter->SetTransform(translation);
			transformFilter->Update();

			vtkOutput = transformFilter->GetOutput();
			break;
		}
	}
	delete[] doublePositions;
}

//----------------------------------------------------------------------------
void VtkWellboreMarker::applyOrientation() {

	/******* TODO
	 * calculate rotation Dips/Azimuth
	 * use : vtkTransform
	 */

	// for test only
	vtkSmartPointer<vtkTransform> rotation = vtkSmartPointer<vtkTransform>::New();
	rotation->RotateX(30);
	rotation->RotateY(30);
	rotation->RotateZ(30);

	vtkSmartPointer<vtkTransformPolyDataFilter> transformFilter = vtkSmartPointer<vtkTransformPolyDataFilter>::New();
	transformFilter->SetInputData(vtkOutput);
	transformFilter->SetTransform(rotation);
	transformFilter->Update();

	vtkOutput = transformFilter->GetOutput();
}

//----------------------------------------------------------------------------
void VtkWellboreMarker::remove(const std::string & uuid)
{
	if (uuid == getUuid()) {
		vtkOutput = nullptr;
	}
}
