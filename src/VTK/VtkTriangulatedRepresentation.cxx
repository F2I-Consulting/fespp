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
#include "VtkTriangulatedRepresentation.h"

// include VTK library
#include <vtkPointData.h>
#include <vtkCellArray.h>
#include <vtkSmartPointer.h>
#include <vtkTriangle.h>

// include F2i-consulting Energistics Standards API
#include <fesapi/resqml2_0_1/TriangulatedSetRepresentation.h>

// include F2i-consulting Energistics Standards ParaView Plugin
#include "VtkProperty.h"

//----------------------------------------------------------------------------
VtkTriangulatedRepresentation::VtkTriangulatedRepresentation(const std::string & fileName, const std::string & name, const std::string & uuid, const std::string & uuidParent, unsigned int patchNo, COMMON_NS::DataObjectRepository *pckRep, COMMON_NS::DataObjectRepository *pckSubRep) :
	VtkResqml2PolyData(fileName, name, uuid, uuidParent, pckRep, pckSubRep), patchIndex(patchNo)
{
}

//----------------------------------------------------------------------------
VtkTriangulatedRepresentation::~VtkTriangulatedRepresentation()
{
	patchIndex = 0;
	lastProperty = "";
}

//----------------------------------------------------------------------------
void VtkTriangulatedRepresentation::createOutput(const std::string &uuid)
{
	if (!subRepresentation)	{
		RESQML2_0_1_NS::TriangulatedSetRepresentation* triangulatedSetRepresentation = epcPackageRepresentation->getDataObjectByUuid<RESQML2_0_1_NS::TriangulatedSetRepresentation>(getUuid());

		if (vtkOutput == nullptr && triangulatedSetRepresentation != nullptr) {
			vtkOutput = vtkSmartPointer<vtkPolyData>::New();

			// POINT
			vtkSmartPointer<vtkPoints> triangulatedRepresentationPoints = vtkSmartPointer<vtkPoints>::New();

			const unsigned int nodeCount = triangulatedSetRepresentation->getXyzPointCountOfAllPatches();
			std::unique_ptr<double[]> allXyzPoints(new double[nodeCount * 3]);
			triangulatedSetRepresentation->getXyzPointsOfAllPatchesInGlobalCrs(allXyzPoints.get());
			createVtkPoints(nodeCount, allXyzPoints.get(), triangulatedSetRepresentation->getLocalCrs(0));
			vtkOutput->SetPoints(points);

			// CELLS
			vtkSmartPointer<vtkCellArray> triangulatedRepresentationTriangles = vtkSmartPointer<vtkCellArray>::New();
			std::unique_ptr<unsigned int[]> triangleIndices(new unsigned int[triangulatedSetRepresentation->getTriangleCountOfPatch(patchIndex) * 3]);
			triangulatedSetRepresentation->getTriangleNodeIndicesOfPatch(patchIndex, triangleIndices.get());
			for (unsigned int p = 0; p < triangulatedSetRepresentation->getTriangleCountOfPatch(patchIndex); ++p) {
				vtkSmartPointer<vtkTriangle> triangulatedRepresentationTriangle = vtkSmartPointer<vtkTriangle>::New();
				triangulatedRepresentationTriangle->GetPointIds()->SetId(0, triangleIndices[p * 3]);
				triangulatedRepresentationTriangle->GetPointIds()->SetId(1, triangleIndices[p * 3 + 1]);
				triangulatedRepresentationTriangle->GetPointIds()->SetId(2, triangleIndices[p * 3 + 2]);
				triangulatedRepresentationTriangles->InsertNextCell(triangulatedRepresentationTriangle);
			}
			vtkOutput->SetPolys(triangulatedRepresentationTriangles);

			points = nullptr;
		}
		if (uuid != getUuid()) {
			vtkDataArray* arrayProperty = uuidToVtkProperty[uuid]->visualize(uuid, triangulatedSetRepresentation);
			addProperty(uuid, arrayProperty);
		}
	}
}

//----------------------------------------------------------------------------
void VtkTriangulatedRepresentation::addProperty(const std::string & uuidProperty, vtkDataArray* dataProperty)
{
	vtkOutput->Modified();
	vtkOutput->GetPointData()->AddArray(dataProperty);
	lastProperty = uuidProperty;
}

long VtkTriangulatedRepresentation::getAttachmentPropertyCount(const std::string &, VtkEpcCommon::FesppAttachmentProperty)
{
	RESQML2_0_1_NS::TriangulatedSetRepresentation* triangulatedSetRepresentation = epcPackageRepresentation->getDataObjectByUuid<RESQML2_0_1_NS::TriangulatedSetRepresentation>(getUuid());
	return triangulatedSetRepresentation != nullptr ? triangulatedSetRepresentation->getXyzPointCountOfAllPatches() : 0;
}
