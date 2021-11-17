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
#include "ResqmlTriangulatedToVtkPolyData.h"

// include VTK library
#include <vtkPointData.h>
#include <vtkCellData.h>
#include <vtkCellArray.h>
#include <vtkSmartPointer.h>
#include <vtkDoubleArray.h>
#include <vtkTriangle.h>
#include <vtkPolyData.h>

// include F2i-consulting Energistics Standards API
#include <fesapi/resqml2/TriangulatedSetRepresentation.h>
#include <fesapi/resqml2/AbstractLocal3dCrs.h>

// include F2i-consulting Energistics Paraview Plugin
#include "ResqmlMapping/ResqmlPropertyToVtkDataArray.h"

//----------------------------------------------------------------------------
ResqmlTriangulatedToVtkPolyData::ResqmlTriangulatedToVtkPolyData(RESQML2_NS::TriangulatedSetRepresentation *triangulated, unsigned int patchNo, int proc_number, int max_proc)
	: ResqmlAbstractRepresentationToVtkDataset(triangulated,
											  proc_number,
											  max_proc), 
											  patchIndex(patchNo)
{
}

//----------------------------------------------------------------------------
void ResqmlTriangulatedToVtkPolyData::loadVtkObject()
{
	vtkSmartPointer<vtkPolyData> vtkPolydata = vtkSmartPointer<vtkPolyData>::New();

	RESQML2_NS::TriangulatedSetRepresentation *triangulatedSetRepresentation = static_cast<RESQML2_NS::TriangulatedSetRepresentation *>(this->resqmlData);

	const ULONG64 nodeCount = triangulatedSetRepresentation->getXyzPointCountOfAllPatches();
	double *allXyzPoints = new double[nodeCount * 3]; // Will be deleted by VTK
	triangulatedSetRepresentation->getXyzPointsOfAllPatchesInGlobalCrs(allXyzPoints);

	vtkSmartPointer<vtkPoints> vtkPts = vtkSmartPointer<vtkPoints>::New();

	const size_t coordCount = this->pointCount * 3;
	if (triangulatedSetRepresentation->getLocalCrs(0)->isDepthOriented())
	{
		for (size_t zCoordIndex = 2; zCoordIndex < coordCount; zCoordIndex += 3)
		{
			allXyzPoints[zCoordIndex] *= -1;
		}
	}

	vtkSmartPointer<vtkDoubleArray> vtkUnderlyingArray = vtkSmartPointer<vtkDoubleArray>::New();
	vtkUnderlyingArray->SetNumberOfComponents(3);
	// Take ownership of the underlying C array
	vtkUnderlyingArray->SetArray(allXyzPoints, coordCount, vtkAbstractArray::VTK_DATA_ARRAY_DELETE);
	vtkPts->SetData(vtkUnderlyingArray);
	vtkPolydata->SetPoints(vtkPts);

	// CELLS
	vtkSmartPointer<vtkCellArray> triangulatedRepresentationTriangles = vtkSmartPointer<vtkCellArray>::New();
	std::unique_ptr<unsigned int[]> triangleIndices(new unsigned int[triangulatedSetRepresentation->getTriangleCountOfPatch(patchIndex) * 3]);
	triangulatedSetRepresentation->getTriangleNodeIndicesOfPatch(patchIndex, triangleIndices.get());
	for (unsigned int p = 0; p < triangulatedSetRepresentation->getTriangleCountOfPatch(patchIndex); ++p)
	{
		vtkSmartPointer<vtkTriangle> triangulatedRepresentationTriangle = vtkSmartPointer<vtkTriangle>::New();
		triangulatedRepresentationTriangle->GetPointIds()->SetId(0, triangleIndices[p * 3]);
		triangulatedRepresentationTriangle->GetPointIds()->SetId(1, triangleIndices[p * 3 + 1]);
		triangulatedRepresentationTriangle->GetPointIds()->SetId(2, triangleIndices[p * 3 + 2]);
		triangulatedRepresentationTriangles->InsertNextCell(triangulatedRepresentationTriangle);
	}
	vtkPolydata->SetPolys(triangulatedRepresentationTriangles);

	this->vtkData = vtkPolydata;
	this->vtkData->Modified();
}
