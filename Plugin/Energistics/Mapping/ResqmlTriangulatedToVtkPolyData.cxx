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
#include "Mapping/ResqmlPropertyToVtkDataArray.h"

//----------------------------------------------------------------------------
ResqmlTriangulatedToVtkPolyData::ResqmlTriangulatedToVtkPolyData(RESQML2_NS::TriangulatedSetRepresentation * triangulated, uint32_t patch_index, int proc_number, int max_proc)
	: ResqmlAbstractRepresentationToVtkPartitionedDataSet(triangulated,
											   proc_number,
											   max_proc),
	  patch_index(patch_index)
{
	this->pointCount = triangulated->getXyzPointCountOfPatch(patch_index);

	this->vtkData = vtkSmartPointer<vtkPartitionedDataSet>::New();

	this->loadVtkObject();
}

//----------------------------------------------------------------------------
RESQML2_NS::TriangulatedSetRepresentation const* ResqmlTriangulatedToVtkPolyData::getResqmlData() const
{
	return static_cast<RESQML2_NS::TriangulatedSetRepresentation const*>(resqmlData);
}

uint32_t ResqmlTriangulatedToVtkPolyData::getPreviousPatchesNodeCount() const
{
	uint32_t result = 0;
	RESQML2_NS::TriangulatedSetRepresentation const* triangulated = getResqmlData();

	for (uint_fast32_t  previousPatchIndex = 0; previousPatchIndex < patch_index; ++previousPatchIndex)
	{
		result += triangulated->getXyzPointCountOfPatch(previousPatchIndex);
	}

	return result;
}

//----------------------------------------------------------------------------
void ResqmlTriangulatedToVtkPolyData::loadVtkObject()
{
	vtkSmartPointer<vtkPolyData> vtk_polydata = vtkSmartPointer<vtkPolyData>::New();

	RESQML2_NS::TriangulatedSetRepresentation const* triangulated = getResqmlData();

	double *allXyzPoints = new double[this->pointCount * 3]; // Will be deleted by VTK
	triangulated->getXyzPointsOfPatchInGlobalCrs(this->patch_index, allXyzPoints);

	vtkSmartPointer<vtkPoints> vtkPts = vtkSmartPointer<vtkPoints>::New();

	const size_t coordCount = this->pointCount * 3;
	if (triangulated->getLocalCrs(0)->isDepthOriented())
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
	vtk_polydata->SetPoints(vtkPts);

	// CELLS
	const size_t previousPatchesNodeCount = getPreviousPatchesNodeCount();
	vtkSmartPointer<vtkCellArray> triangulatedRepresentationTriangles = vtkSmartPointer<vtkCellArray>::New();
	std::unique_ptr<unsigned int[]> triangleIndices(new unsigned int[triangulated->getTriangleCountOfPatch(this->patch_index) * 3]);
	triangulated->getTriangleNodeIndicesOfPatch(this->patch_index, triangleIndices.get());
	for (unsigned int p = 0; p < triangulated->getTriangleCountOfPatch(this->patch_index); ++p)
	{
		vtkSmartPointer<vtkTriangle> triangulatedRepresentationTriangle = vtkSmartPointer<vtkTriangle>::New();
		triangulatedRepresentationTriangle->GetPointIds()->SetId(0, triangleIndices[p * 3] - previousPatchesNodeCount);
		triangulatedRepresentationTriangle->GetPointIds()->SetId(1, triangleIndices[p * 3 + 1] - previousPatchesNodeCount);
		triangulatedRepresentationTriangle->GetPointIds()->SetId(2, triangleIndices[p * 3 + 2] - previousPatchesNodeCount);
		triangulatedRepresentationTriangles->InsertNextCell(triangulatedRepresentationTriangle);
	}
	vtk_polydata->SetPolys(triangulatedRepresentationTriangles);

	this->vtkData->SetPartition(0, vtk_polydata);
	this->vtkData->Modified();
}
