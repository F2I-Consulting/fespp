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
#include "VtkWellboreTrajectoryRepresentationDatum.h"

// VTK
#include <vtkSmartPointer.h>
#include <vtkCellArray.h>
#include <vtkPyramid.h>
#include <vtkPoints.h>

// FESAPI
#include <fesapi/common/EpcDocument.h>
#include <fesapi/resqml2_0_1/WellboreTrajectoryRepresentation.h>
#include <fesapi/resqml2/MdDatum.h>

//----------------------------------------------------------------------------
VtkWellboreTrajectoryRepresentationDatum::VtkWellboreTrajectoryRepresentationDatum(const std::string & fileName, const std::string & name, const std::string & uuid, const std::string & uuidParent, COMMON_NS::DataObjectRepository *repoRepresentation, COMMON_NS::DataObjectRepository *repoSubRepresentation) :
VtkResqml2UnstructuredGrid(fileName, name, uuid, uuidParent, repoRepresentation, repoSubRepresentation)
{
}

//----------------------------------------------------------------------------
void VtkWellboreTrajectoryRepresentationDatum::createOutput(const std::string & uuid)
{
	vtkOutput = vtkSmartPointer<vtkUnstructuredGrid>::New();
	resqml2_0_1::WellboreTrajectoryRepresentation* wellboreSetRepresentation = nullptr;
	common::AbstractObject* obj = epcPackageRepresentation->getDataObjectByUuid(uuid);
	if (obj != nullptr && obj->getXmlTag() ==  "WellboreTrajectoryRepresentation") {
		wellboreSetRepresentation = static_cast<resqml2_0_1::WellboreTrajectoryRepresentation*>(obj);
	}

	resqml2::MdDatum* mdDatum = wellboreSetRepresentation->getMdDatum();

	const double xOrigin = mdDatum->getXInGlobalCrs();
	const double yOrigin = mdDatum->getYInGlobalCrs();
	const double zOrigin = mdDatum->getZInGlobalCrs();

	vtkSmartPointer<vtkPoints> points = vtkSmartPointer<vtkPoints>::New();
	double p0[3] = {xOrigin+1.5, yOrigin+1.5, zOrigin};
	double p1[3] = {xOrigin-1.5, yOrigin+1.5, zOrigin};
	double p2[3] = {xOrigin-1.5, yOrigin-1.5, zOrigin};
	double p3[3] = {xOrigin+1.5, yOrigin-1.5, zOrigin};
	double p4[3] = {xOrigin, yOrigin, zOrigin-10};

	points->InsertNextPoint(p0);
	points->InsertNextPoint(p1);
	points->InsertNextPoint(p2);
	points->InsertNextPoint(p3);
	points->InsertNextPoint(p4);

	vtkSmartPointer<vtkPyramid> pyramid = vtkSmartPointer<vtkPyramid>::New();
	pyramid->GetPointIds()->SetId(0,0);
	pyramid->GetPointIds()->SetId(1,1);
	pyramid->GetPointIds()->SetId(2,2);
	pyramid->GetPointIds()->SetId(3,3);
	pyramid->GetPointIds()->SetId(4,4);

	vtkSmartPointer<vtkCellArray> cells = vtkSmartPointer<vtkCellArray>::New();
	cells->InsertNextCell (pyramid);

	vtkOutput->SetPoints(points);
	vtkOutput->InsertNextCell(pyramid->GetCellType(), pyramid->GetPointIds());
}

//----------------------------------------------------------------------------
void VtkWellboreTrajectoryRepresentationDatum::addProperty(const std::string & uuidProperty, vtkDataArray* dataProperty)
{
	;
}

long VtkWellboreTrajectoryRepresentationDatum::getAttachmentPropertyCount(const std::string & uuid, VtkEpcCommon::FesppAttachmentProperty propertyUnit)
{
	return 0;
}
