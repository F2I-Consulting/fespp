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
#include "Mapping/ResqmlGrid2dToVtkStructuredGrid.h"

// include VTK library
#include <vtkSmartPointer.h>
#include <vtkPointData.h>
#include <vtkCellArray.h>

// include F2i-consulting Energistics Standards API
#include <fesapi/resqml2/Grid2dRepresentation.h>
#include <fesapi/resqml2/AbstractLocal3dCrs.h>

// include F2i-consulting Energistics Paraview Plugin
#include "Mapping/ResqmlPropertyToVtkDataArray.h"

//----------------------------------------------------------------------------
ResqmlGrid2dToVtkStructuredGrid::ResqmlGrid2dToVtkStructuredGrid(RESQML2_NS::Grid2dRepresentation *grid2D, int proc_number, int max_proc)
	: ResqmlAbstractRepresentationToVtkPartitionedDataSet(grid2D,
											   proc_number,
											   max_proc)
{
	this->pointCount = grid2D->getNodeCountAlongIAxis() * grid2D->getNodeCountAlongJAxis();

	this->vtkData = vtkSmartPointer<vtkPartitionedDataSet>::New();

	this->vtkData->Modified();
}

//----------------------------------------------------------------------------
RESQML2_NS::Grid2dRepresentation const* ResqmlGrid2dToVtkStructuredGrid::getResqmlData() const
{
	return static_cast<RESQML2_NS::Grid2dRepresentation const*>(resqmlData);
}

//----------------------------------------------------------------------------
void ResqmlGrid2dToVtkStructuredGrid::loadVtkObject()
{
	vtkSmartPointer<vtkStructuredGrid> structuredGrid = vtkSmartPointer<vtkStructuredGrid>::New();

	RESQML2_NS::Grid2dRepresentation const* grid2D = getResqmlData();

	const double originX = grid2D->getXOriginInGlobalCrs();
	const double originY = grid2D->getYOriginInGlobalCrs();
	const double XIOffset = grid2D->getXIOffsetInGlobalCrs();
	const double XJOffset = grid2D->getXJOffsetInGlobalCrs();
	const double YIOffset = grid2D->getYIOffsetInGlobalCrs();
	const double YJOffset = grid2D->getYJOffsetInGlobalCrs();
	const double zIndice = grid2D->getLocalCrs(0)->isDepthOriented() ? -1 : 1;
	const ULONG64 nbNodeI = grid2D->getNodeCountAlongIAxis();
	const ULONG64 nbNodeJ = grid2D->getNodeCountAlongJAxis();

	structuredGrid->SetDimensions(nbNodeI, nbNodeJ, 1);
	// POINT
	this->pointCount = nbNodeI * nbNodeJ;

	std::unique_ptr<double[]> z(new double[nbNodeI * nbNodeJ]);
	grid2D->getZValuesInGlobalCrs(z.get());

	vtkSmartPointer<vtkPoints> points = vtkSmartPointer<vtkPoints>::New();
	vtkSmartPointer<vtkCellArray> vertices = vtkSmartPointer<vtkCellArray>::New();
	for (ULONG64 j = 0; j < nbNodeJ; ++j)
	{
		for (ULONG64 i = 0; i < nbNodeI; ++i)
		{
			const size_t ptId = i + j * nbNodeI;
			//if (!isnan(z[ptId]))
			//{
				vtkIdType pid = points->InsertNextPoint(
					originX + i * XIOffset + j * XJOffset,
					originY + i * YIOffset + j * YJOffset,
					z[ptId] * zIndice);
				//vertices->InsertNextCell(1, &pid);
			//}
		}
	}

	structuredGrid->SetPoints(points);
	//structuredGrid->SetVerts(vertices);

	this->vtkData->SetPartition(0, structuredGrid);
	this->vtkData->Modified();
}
