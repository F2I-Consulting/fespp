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
#include <vtkPointData.h>
#include <vtkCellArray.h>

// FESAPI
#include <fesapi/resqml2/Grid2dRepresentation.h>

// FESPP
#include "VtkProperty.h"

//----------------------------------------------------------------------------
VtkGrid2DRepresentationPoints::VtkGrid2DRepresentationPoints(const std::string & fileName, const std::string & name, const std::string & uuid, const std::string & uuidParent, COMMON_NS::DataObjectRepository const * repoRepresentation, COMMON_NS::DataObjectRepository const * repoSubRepresentation) :
VtkResqml2PolyData(fileName, name, uuid, uuidParent, repoRepresentation, repoSubRepresentation), lastProperty("")
{
}

VtkGrid2DRepresentationPoints::~VtkGrid2DRepresentationPoints()
{
	lastProperty = "";
}
//----------------------------------------------------------------------------
void VtkGrid2DRepresentationPoints::visualize(const std::string & uuid)
{
	if (!subRepresentation)	{

		RESQML2_NS::Grid2dRepresentation const * grid2dRepresentation = epcPackageRepresentation->getDataObjectByUuid<RESQML2_NS::Grid2dRepresentation>(getUuid().substr(0, 36));

		if (vtkOutput == nullptr) {
			vtkOutput = vtkSmartPointer<vtkPolyData>::New();

			const double originX = grid2dRepresentation->getXOriginInGlobalCrs();
			const double originY = grid2dRepresentation->getYOriginInGlobalCrs();
			const double XIOffset = grid2dRepresentation->getXIOffsetInGlobalCrs();
			const double XJOffset = grid2dRepresentation->getXJOffsetInGlobalCrs();
			const double YIOffset = grid2dRepresentation->getYIOffsetInGlobalCrs();
			const double YJOffset = grid2dRepresentation->getYJOffsetInGlobalCrs();
			const double zIndice = grid2dRepresentation->getLocalCrs(0)->isDepthOriented() ? -1 : 1;
			const ULONG64 nbNodeI = grid2dRepresentation->getNodeCountAlongIAxis();
			const ULONG64 nbNodeJ = grid2dRepresentation->getNodeCountAlongJAxis();
			std::unique_ptr<double[]> z(new double[nbNodeI * nbNodeJ]);
			grid2dRepresentation->getZValuesInGlobalCrs(z.get());

			vtkSmartPointer<vtkPoints> points = vtkSmartPointer<vtkPoints>::New();
			vtkSmartPointer<vtkCellArray> vertices = vtkSmartPointer<vtkCellArray>::New();
			for (ULONG64 j = 0; j < nbNodeJ; ++j) {
				for (ULONG64 i = 0; i < nbNodeI; ++i) {
					const size_t ptId = i + j * nbNodeI;
					if (!isnan(z[ptId])) {
						vtkIdType pid = points->InsertNextPoint(
								originX + i*XIOffset + j*XJOffset,
								originY + i*YIOffset + j*YJOffset,
								z[ptId] * zIndice);
						vertices->InsertNextCell(1, &pid);
					}
				}
			}
			vtkOutput->SetPoints(points);
			vtkOutput->SetVerts(vertices);
		}
		if (uuid != getUuid().substr(0, 36)) {
			addProperty(uuid, uuidToVtkProperty[uuid]->visualize(uuid, grid2dRepresentation));
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
long VtkGrid2DRepresentationPoints::getAttachmentPropertyCount(const std::string &, VtkEpcCommon::FesppAttachmentProperty)
{
	RESQML2_NS::Grid2dRepresentation const * grid2dRepresentation = epcPackageRepresentation->getDataObjectByUuid<RESQML2_NS::Grid2dRepresentation>(getUuid().substr(0, 36));

	return grid2dRepresentation != nullptr
		? grid2dRepresentation->getNodeCountAlongIAxis() * grid2dRepresentation->getNodeCountAlongJAxis()
		: 0;
}
