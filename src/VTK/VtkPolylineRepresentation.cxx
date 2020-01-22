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
#include "VtkPolylineRepresentation.h"

// include VTK library
#include <vtkPointData.h>
#include <vtkCellArray.h>
#include <vtkPolyLine.h>
#include <vtkSmartPointer.h>

// include F2i-consulting Energistics Standards API
#include <fesapi/resqml2_0_1/PolylineSetRepresentation.h>
#include <fesapi/common/EpcDocument.h>

// include F2i-consulting Energistics Standards ParaView Plugin
#include "VtkProperty.h"

//----------------------------------------------------------------------------
VtkPolylineRepresentation::VtkPolylineRepresentation(const std::string & fileName, const std::string & name, const std::string & uuid, const std::string & uuidParent, unsigned int patchNo, COMMON_NS::DataObjectRepository *repoRepresentation, COMMON_NS::DataObjectRepository *repoSubRepresentation) :
	VtkResqml2PolyData(fileName, name, uuid, uuidParent, repoRepresentation, repoSubRepresentation), patchIndex(patchNo)
{
}

//----------------------------------------------------------------------------
VtkPolylineRepresentation::~VtkPolylineRepresentation()
{
	patchIndex = 0;
	lastProperty = "";
}

//----------------------------------------------------------------------------
void VtkPolylineRepresentation::createOutput(const std::string & uuid)
{
	if (!subRepresentation)	{
		resqml2_0_1::PolylineSetRepresentation* polylineSetRepresentation = nullptr;
		common::AbstractObject* obj = epcPackageRepresentation->getDataObjectByUuid(getUuid());
		if (obj != nullptr && obj->getXmlTag() == "PolylineSetRepresentation") {
			polylineSetRepresentation = static_cast<resqml2_0_1::PolylineSetRepresentation*>(obj);
		}

		if (!vtkOutput) {
			VtkResqml2PolyData::vtkOutput = vtkSmartPointer<vtkPolyData>::New();

			// POINT
			unsigned int nodeCount = polylineSetRepresentation->getXyzPointCountOfPatch(patchIndex);

			double * allPoint = new double[nodeCount * 3];
			polylineSetRepresentation->getXyzPointsOfPatch(patchIndex, allPoint);

			createVtkPoints(nodeCount, allPoint, polylineSetRepresentation->getLocalCrs(0));
			vtkOutput->SetPoints(points);

			delete[] allPoint;

			// POLYLINE
			vtkSmartPointer<vtkCellArray> setPolylineRepresentationLines = vtkSmartPointer<vtkCellArray>::New();

			unsigned int countPolyline = polylineSetRepresentation->getPolylineCountOfPatch(patchIndex);

			unsigned int* countNodePolylineInPatch = new unsigned int[countPolyline];
			polylineSetRepresentation->getNodeCountPerPolylineInPatch(patchIndex, countNodePolylineInPatch);

			unsigned int idPoint = 0;
			for (unsigned int polylineIndex = 0; polylineIndex < countPolyline; ++polylineIndex) {
				vtkSmartPointer<vtkPolyLine> polylineRepresentation = vtkSmartPointer<vtkPolyLine>::New();
				polylineRepresentation->GetPointIds()->SetNumberOfIds(countNodePolylineInPatch[polylineIndex]);
				for (unsigned int line = 0; line < countNodePolylineInPatch[polylineIndex]; ++line) {
					polylineRepresentation->GetPointIds()->SetId(line, idPoint);
					idPoint++;

				}
				setPolylineRepresentationLines->InsertNextCell(polylineRepresentation);
				vtkOutput->SetLines(setPolylineRepresentationLines);
			}
			points = nullptr;
			delete[] countNodePolylineInPatch;
		}
		else {
			if (uuid != getUuid()) {
				vtkDataArray* arrayProperty = uuidToVtkProperty[uuid]->visualize(uuid, polylineSetRepresentation);
				addProperty(uuid, arrayProperty);
			}
		}
	}
}

//----------------------------------------------------------------------------
void VtkPolylineRepresentation::addProperty(const std::string & uuidProperty, vtkDataArray* dataProperty)
{
	vtkOutput->Modified();
	vtkOutput->GetPointData()->AddArray(dataProperty);
	lastProperty = uuidProperty;
}

//----------------------------------------------------------------------------
long VtkPolylineRepresentation::getAttachmentPropertyCount(const std::string & uuid, VtkEpcCommon::FesppAttachmentProperty propertyUnit)
{
	long result = 0;
	resqml2_0_1::PolylineSetRepresentation* polylineSetRepresentation = nullptr;
	common::AbstractObject* obj = epcPackageRepresentation->getDataObjectByUuid(getUuid());
	if (obj != nullptr && obj->getXmlTag() == "PolylineSetRepresentation") {
		polylineSetRepresentation = static_cast<resqml2_0_1::PolylineSetRepresentation*>(obj);
		result = polylineSetRepresentation->getXyzPointCountOfPatch(0);
	}
	return result;
}
