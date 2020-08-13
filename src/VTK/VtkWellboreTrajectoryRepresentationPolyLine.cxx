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
#include "VtkWellboreTrajectoryRepresentationPolyLine.h"

// VTK
#include <vtkPointData.h>
#include <vtkCellArray.h>
#include <vtkPolyLine.h>
#include <vtkSmartPointer.h>
#include <vtkDataArray.h>

// FESAPI
#include <fesapi/resqml2/WellboreTrajectoryRepresentation.h>

// FESPP
#include "VtkProperty.h"

//----------------------------------------------------------------------------
VtkWellboreTrajectoryRepresentationPolyLine::VtkWellboreTrajectoryRepresentationPolyLine(const std::string & fileName, const std::string & name, const std::string & uuid, const std::string & uuidParent, COMMON_NS::DataObjectRepository const * repoRepresentation, COMMON_NS::DataObjectRepository const * repoSubRepresentation) :
VtkResqml2PolyData(fileName, name, uuid, uuidParent, repoRepresentation, repoSubRepresentation)
{
}

//----------------------------------------------------------------------------
void VtkWellboreTrajectoryRepresentationPolyLine::createOutput(const std::string & uuid)
{
	if (!subRepresentation)	{

		RESQML2_NS::WellboreTrajectoryRepresentation* wellboreSetRepresentation = nullptr;
		common::AbstractObject* obj = epcPackageRepresentation->getDataObjectByUuid(getUuid().substr(0, 36));
		if (obj != nullptr && obj->getXmlTag() == "WellboreTrajectoryRepresentation") {
			wellboreSetRepresentation = static_cast<RESQML2_NS::WellboreTrajectoryRepresentation*>(obj);
		}

		if (!vtkOutput) {
			vtkOutput = vtkSmartPointer<vtkPolyData>::New();

			// POINT
			unsigned int pointCount = wellboreSetRepresentation->getXyzPointCountOfPatch(0);
			std::unique_ptr<double[]> allXyzPoints(new double[pointCount * 3]);
			wellboreSetRepresentation->getXyzPointsOfAllPatchesInGlobalCrs(allXyzPoints.get());
			createVtkPoints(pointCount, allXyzPoints.get(), wellboreSetRepresentation->getLocalCrs(0));
			vtkOutput->SetPoints(points);

			// POLYLINE
			vtkSmartPointer<vtkPolyLine> polylineRepresentation = vtkSmartPointer<vtkPolyLine>::New();
			polylineRepresentation->GetPointIds()->SetNumberOfIds(pointCount);
			for (unsigned int nodeIndex = 0; nodeIndex < pointCount; ++nodeIndex) {
				polylineRepresentation->GetPointIds()->SetId(nodeIndex, nodeIndex);
			}

			vtkSmartPointer<vtkCellArray> setPolylineRepresentationLines = vtkSmartPointer<vtkCellArray>::New();
			setPolylineRepresentationLines->InsertNextCell(polylineRepresentation);

			vtkOutput->SetLines(setPolylineRepresentationLines);
			points = nullptr;
		}
		// PROPERTY(IES)
		if (uuid != getUuid().substr(0, 36)) {
			vtkDataArray* arrayProperty = uuidToVtkProperty[uuid]->visualize(uuid, wellboreSetRepresentation);
			addProperty(uuid, arrayProperty);
		}
	}
}


//----------------------------------------------------------------------------
VtkWellboreTrajectoryRepresentationPolyLine::~VtkWellboreTrajectoryRepresentationPolyLine()
{
	lastProperty = "";
}

//----------------------------------------------------------------------------
void VtkWellboreTrajectoryRepresentationPolyLine::addProperty(const std::string & uuidProperty, vtkDataArray* dataProperty)
{
	vtkOutput->Modified();
	vtkOutput->GetPointData()->AddArray(dataProperty);
	lastProperty = uuidProperty;
}

long VtkWellboreTrajectoryRepresentationPolyLine::getAttachmentPropertyCount(const std::string &, VtkEpcCommon::FesppAttachmentProperty)
{
	RESQML2_NS::WellboreTrajectoryRepresentation* obj = epcPackageRepresentation->getDataObjectByUuid<RESQML2_NS::WellboreTrajectoryRepresentation>(getUuid().substr(0, 36));
	return obj != nullptr ? obj->getXyzPointCountOfAllPatches() : 0;
}
