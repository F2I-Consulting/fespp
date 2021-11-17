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
#include "ResqmlMapping/ResqmlGrid2dToVtkPolyData.h"

// include VTK library
#include <vtkSmartPointer.h>
#include <vtkPointData.h>
#include <vtkCellArray.h>

// include F2i-consulting Energistics Standards API
#include <fesapi/resqml2/Grid2dRepresentation.h>
#include <fesapi/resqml2/AbstractLocal3dCrs.h>

// include F2i-consulting Energistics Paraview Plugin
#include "ResqmlMapping/ResqmlPropertyToVtkDataArray.h"

//----------------------------------------------------------------------------
ResqmlGrid2dToVtkPolyData::ResqmlGrid2dToVtkPolyData(RESQML2_NS::Grid2dRepresentation *grid2D, int proc_number, int max_proc)
	: ResqmlAbstractRepresentationToVtkDataset(grid2D,
											  proc_number,
											  max_proc)
{
}

//----------------------------------------------------------------------------
void ResqmlGrid2dToVtkPolyData::loadVtkObject()
{
	vtkSmartPointer<vtkPolyData> vtkPolydata = vtkSmartPointer<vtkPolyData>::New();

	const auto grid2D = dynamic_cast<const RESQML2_NS::Grid2dRepresentation *>(this->resqmlData);

	const double originX = grid2D->getXOriginInGlobalCrs();
	const double originY = grid2D->getYOriginInGlobalCrs();
	const double XIOffset = grid2D->getXIOffsetInGlobalCrs();
	const double XJOffset = grid2D->getXJOffsetInGlobalCrs();
	const double YIOffset = grid2D->getYIOffsetInGlobalCrs();
	const double YJOffset = grid2D->getYJOffsetInGlobalCrs();
	const double zIndice = grid2D->getLocalCrs(0)->isDepthOriented() ? -1 : 1;
	const ULONG64 nbNodeI = grid2D->getNodeCountAlongIAxis();
	const ULONG64 nbNodeJ = grid2D->getNodeCountAlongJAxis();

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
			if (!isnan(z[ptId]))
			{
				vtkIdType pid = points->InsertNextPoint(
					originX + i * XIOffset + j * XJOffset,
					originY + i * YIOffset + j * YJOffset,
					z[ptId] * zIndice);
				vertices->InsertNextCell(1, &pid);
			}
		}
	}

	vtkPolydata->SetPoints(points);
	vtkPolydata->SetVerts(vertices);

	this->vtkData = vtkPolydata;
	this->vtkData->Modified();
}
