﻿/*-----------------------------------------------------------------------
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
#include <vtkDataArray.h>

// FESAPI
#include <fesapi/resqml2/WellboreTrajectoryRepresentation.h>

// FESPP
#include "VtkProperty.h"

//----------------------------------------------------------------------------
VtkWellboreTrajectoryRepresentationPolyLine::VtkWellboreTrajectoryRepresentationPolyLine(const std::string & fileName, const std::string & name, const std::string & uuid, const std::string & uuidParent,
	COMMON_NS::DataObjectRepository const * repoRepresentation, COMMON_NS::DataObjectRepository const * repoSubRepresentation) :
	VtkResqml2PolyData(fileName, name, uuid, uuidParent, repoRepresentation, repoSubRepresentation)
{
}

//----------------------------------------------------------------------------
void VtkWellboreTrajectoryRepresentationPolyLine::visualize(const std::string & uuid)
{
	if (!subRepresentation)	{

		const std::string internalStoredUuid = getUuid().substr(0, 36);

		RESQML2_NS::WellboreTrajectoryRepresentation const * const wellboreSetRepresentation = epcPackageRepresentation->getDataObjectByUuid<RESQML2_NS::WellboreTrajectoryRepresentation>(internalStoredUuid);
		if (wellboreSetRepresentation == nullptr) {
			vtkOutputWindowDisplayDebugText(("The UUID " + internalStoredUuid + " is not a RESQML2 WellboreTrajectoryRepresentation\n").c_str());
			return;
		}

		// GEOMETRY
		if (!vtkOutput) {
			vtkOutput = vtkSmartPointer<vtkPolyData>::New();

			// POINT
			const ULONG64 pointCount = wellboreSetRepresentation->getXyzPointCountOfPatch(0);
			double* allXyzPoints = new double[pointCount * 3]; // Will be deleted by VTK
			wellboreSetRepresentation->getXyzPointsOfAllPatchesInGlobalCrs(allXyzPoints);
			vtkSmartPointer<vtkPoints> vtkPts = createVtkPoints(pointCount, allXyzPoints, wellboreSetRepresentation->getLocalCrs(0));
			vtkOutput->SetPoints(vtkPts);

			// POLYLINE
			vtkSmartPointer<vtkPolyLine> polylineRepresentation = vtkSmartPointer<vtkPolyLine>::New();
			polylineRepresentation->GetPointIds()->SetNumberOfIds(pointCount);
			for (unsigned int nodeIndex = 0; nodeIndex < pointCount; ++nodeIndex) {
				polylineRepresentation->GetPointIds()->SetId(nodeIndex, nodeIndex);
			}

			vtkSmartPointer<vtkCellArray> setPolylineRepresentationLines = vtkSmartPointer<vtkCellArray>::New();
			setPolylineRepresentationLines->InsertNextCell(polylineRepresentation);

			vtkOutput->SetLines(setPolylineRepresentationLines);
		}

		// PROPERTY(IES)
		if (uuid != internalStoredUuid) {
			vtkDataArray* arrayProperty = uuidToVtkProperty[uuid]->visualize(uuid, wellboreSetRepresentation);
			addProperty(uuid, arrayProperty);
		}
	}
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
	RESQML2_NS::WellboreTrajectoryRepresentation const * const obj = epcPackageRepresentation->getDataObjectByUuid<RESQML2_NS::WellboreTrajectoryRepresentation>(getUuid().substr(0, 36));
	return obj != nullptr ? obj->getXyzPointCountOfAllPatches() : 0;
}
