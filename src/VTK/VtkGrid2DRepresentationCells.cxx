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
#include "VtkGrid2DRepresentationCells.h"

// include VTK library
#include <vtkSmartPointer.h>
#include <vtkMath.h>
#include <vtkCellArray.h>

// include F2i-consulting Energistics Standards API
#include <fesapi/common/EpcDocument.h>
#include <fesapi/resqml2/Grid2dRepresentation.h>

//----------------------------------------------------------------------------
VtkGrid2DRepresentationCells::VtkGrid2DRepresentationCells(const std::string & fileName, const std::string & name, const std::string & uuid, const std::string & uuidParent, COMMON_NS::DataObjectRepository *pckEPCRep, COMMON_NS::DataObjectRepository *pckEPCSubRep) :
VtkResqml2StructuredGrid(fileName, name, uuid, uuidParent, pckEPCRep, pckEPCSubRep)
{

}

VtkGrid2DRepresentationCells::~VtkGrid2DRepresentationCells()
{
	lastProperty = "";
}
//----------------------------------------------------------------------------
void VtkGrid2DRepresentationCells::visualize(const std::string & uuid)
{
	if (!subRepresentation)	{

		RESQML2_NS::Grid2dRepresentation* grid2dRepresentation = nullptr;
		common::AbstractObject* obj = epcPackageRepresentation->getDataObjectByUuid(uuid);
		if (obj != nullptr && obj->getXmlTag() == "Grid2dRepresentation") {
			grid2dRepresentation = static_cast<RESQML2_NS::Grid2dRepresentation*>(obj);
		}

		if (!vtkOutput) {
			vtkOutput = vtkSmartPointer<vtkStructuredGrid>::New();

			RESQML2_NS::Grid2dRepresentation *supportingGrid = grid2dRepresentation->getSupportingRepresentation();

			const unsigned int nbNodeI = supportingGrid->getNodeCountAlongIAxis();
			const unsigned int nbNodeJ = supportingGrid->getNodeCountAlongJAxis();
			const double originX = supportingGrid->getXOriginInGlobalCrs();
			const double originY = supportingGrid->getYOriginInGlobalCrs();
			const double XIOffset = supportingGrid->getXIOffsetInGlobalCrs();
			const double XJOffset = supportingGrid->getXJOffsetInGlobalCrs();
			const double YIOffset = supportingGrid->getYIOffsetInGlobalCrs();
			const double YJOffset = supportingGrid->getYJOffsetInGlobalCrs();

			vtkSmartPointer<vtkPoints> points = vtkSmartPointer<vtkPoints>::New();
			std::unique_ptr<double[]> z(new double[nbNodeI * nbNodeJ]);
			grid2dRepresentation->getZValuesInGlobalCrs(z.get());

			std::vector<unsigned int> ptBlank;
			for (unsigned int j = 0; j < nbNodeJ; ++j) {
				for (unsigned int i = 0; i < nbNodeI; ++i) {
					unsigned int ptId = i + j * nbNodeI;
					if (vtkMath::IsNan(z[ptId])) {
						points->InsertNextPoint(i, j, i + j);
						ptBlank.push_back(ptId);
					}
					else {
						points->InsertNextPoint(
								originX + i*XIOffset + j*XJOffset,
								originY + i*YIOffset + j*YJOffset,
								z[ptId]
						);
					}
				}
			}

			vtkOutput->SetDimensions(nbNodeI, nbNodeJ, 1);
			vtkOutput->SetPoints(points);

			for (size_t i = 0; i < ptBlank.size(); ++i) {
				vtkOutput->BlankPoint(ptBlank[i]);
			}

			vtkOutput->Modified();
		}
	}
}
