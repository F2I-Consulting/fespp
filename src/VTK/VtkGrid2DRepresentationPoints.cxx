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
#include "VtkGrid2DRepresentationPoints.h"

// VTK
#include <vtkSmartPointer.h>
#include <vtkMath.h>
#include <vtkPointData.h>
#include <vtkCellArray.h>

// FESAPI
#include <fesapi/common/EpcDocument.h>
#include <fesapi/resqml2_0_1/Grid2dRepresentation.h>

// FESPP
#include "VtkProperty.h"

//----------------------------------------------------------------------------
VtkGrid2DRepresentationPoints::VtkGrid2DRepresentationPoints(const std::string & fileName, const std::string & name, const std::string & uuid, const std::string & uuidParent, COMMON_NS::DataObjectRepository *repoRepresentation, COMMON_NS::DataObjectRepository *repoSubRepresentation) :
VtkResqml2PolyData(fileName, name, uuid, uuidParent, repoRepresentation, repoSubRepresentation)
{
}

VtkGrid2DRepresentationPoints::~VtkGrid2DRepresentationPoints()
{
	lastProperty = "";
}
//----------------------------------------------------------------------------
void VtkGrid2DRepresentationPoints::createOutput(const std::string & uuid)
{
	if (!subRepresentation)	{

		resqml2_0_1::Grid2dRepresentation* grid2dRepresentation = nullptr;
		common::AbstractObject* obj = epcPackageRepresentation->getDataObjectByUuid(getUuid().substr(0, 36));

		if (obj != nullptr && obj->getXmlTag() == "Grid2dRepresentation")
			grid2dRepresentation = static_cast<resqml2_0_1::Grid2dRepresentation*>(obj);

		if (!vtkOutput) {
			vtkOutput = vtkSmartPointer<vtkPolyData>::New();

			const ULONG64 nbNodeI = grid2dRepresentation->getNodeCountAlongIAxis();
			const ULONG64 nbNodeJ = grid2dRepresentation->getNodeCountAlongJAxis();
			const double originX = grid2dRepresentation->getXOriginInGlobalCrs();
			const double originY = grid2dRepresentation->getYOriginInGlobalCrs();
			double *z = new double[nbNodeI * nbNodeJ];
			grid2dRepresentation->getZValuesInGlobalCrs(z);
			vtkSmartPointer<vtkPoints> points = vtkSmartPointer<vtkPoints>::New();
			vtkSmartPointer<vtkCellArray> vertices = vtkSmartPointer<vtkCellArray>::New();
			const double XIOffset = grid2dRepresentation->getXIOffsetInGlobalCrs();
			const double XJOffset = grid2dRepresentation->getXJOffsetInGlobalCrs();
			const double YIOffset = grid2dRepresentation->getYIOffsetInGlobalCrs();
			const double YJOffset = grid2dRepresentation->getYJOffsetInGlobalCrs();

			double zIndice = 1;

			if (grid2dRepresentation->getLocalCrs(0)->isDepthOriented()) {
				zIndice = -1;
			}

			for (ULONG64 j = 0; j < nbNodeJ; ++j) {
				for (ULONG64 i = 0; i < nbNodeI; ++i) {
					size_t ptId = i + j * nbNodeI;
					if (!(vtkMath::IsNan(z[ptId]))) {
						vtkIdType pid[1];
						pid[0] = points->InsertNextPoint(
								originX + i*XIOffset + j*XJOffset,
								originY + i*YIOffset + j*YJOffset,
								z[ptId] * zIndice
						);
						vertices->InsertNextCell(1, pid);
					}
				}
			}
			delete[] z;

			vtkOutput->SetPoints(points);
			vtkOutput->SetVerts(vertices);

		}
		else {
			if (uuid != getUuid().substr(0, 36)) {
				vtkDataArray* arrayProperty = uuidToVtkProperty[uuid]->visualize(uuid, grid2dRepresentation);
				addProperty(uuid, arrayProperty);
			}
		}
	}
}

//----------------------------------------------------------------------------
void VtkGrid2DRepresentationPoints::addProperty(const std::string & uuidProperty, vtkDataArray* dataProperty)
{
	vtkOutput->Modified();
	vtkOutput->GetPointData()->AddArray(dataProperty);
	lastProperty = uuidProperty;
}

//----------------------------------------------------------------------------
long VtkGrid2DRepresentationPoints::getAttachmentPropertyCount(const std::string & uuid, VtkEpcCommon::FesppAttachmentProperty propertyUnit)
{
	long result = 0;
	resqml2_0_1::Grid2dRepresentation* grid2dRepresentation = nullptr;
	common::AbstractObject* obj = epcPackageRepresentation->getDataObjectByUuid(getUuid().substr(0, 36));

	if (obj != nullptr && obj->getXmlTag() == "Grid2dRepresentation"){
		grid2dRepresentation = static_cast<resqml2_0_1::Grid2dRepresentation*>(obj);

		result = grid2dRepresentation->getNodeCountAlongIAxis() * grid2dRepresentation->getNodeCountAlongJAxis();
	}
	return result;
}

