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
#include "ResqmlMapping/ResqmlIjkGridSubRepToVtkUnstructuredGrid.h"

#include <array>

// include VTK library
#include <vtkSmartPointer.h>
#include <vtkCellData.h>
#include <vtkDataArray.h>
#include <vtkDoubleArray.h>
#include <vtkEmptyCell.h>
#include <vtkHexahedron.h>
#include <vtkUnstructuredGrid.h>

// include FESAPI
#include <fesapi/resqml2/SubRepresentation.h>
#include <fesapi/resqml2/AbstractIjkGridRepresentation.h>
#include <fesapi/resqml2/LocalDepth3dCrs.h>

// include FESPP
#include "ResqmlIjkGridToVtkUnstructuredGrid.h"

//----------------------------------------------------------------------------
ResqmlIjkGridSubRepToVtkUnstructuredGrid::ResqmlIjkGridSubRepToVtkUnstructuredGrid(RESQML2_NS::SubRepresentation *subRep, ResqmlIjkGridToVtkUnstructuredGrid *support, int proc_number, int max_proc)
	: ResqmlAbstractRepresentationToVtkPartitionedDataSet(subRep,
											   proc_number - 1,
											   max_proc),
	  resqmlData(subRep),
	  mapperIjkGrid(support)
{
	this->iCellCount = subRep->getElementCountOfPatch(0);
	this->pointCount = subRep->getSupportingRepresentation(0)->getXyzPointCountOfAllPatches();

	this->vtkData = vtkSmartPointer<vtkPartitionedDataSet>::New();
}

//----------------------------------------------------------------------------
void ResqmlIjkGridSubRepToVtkUnstructuredGrid::loadVtkObject()
{
	auto *supportingGrid = dynamic_cast<RESQML2_NS::AbstractIjkGridRepresentation *>(this->resqmlData->getSupportingRepresentation(0));
	if (supportingGrid != nullptr)
	{
		// Create and set the list of points of the vtkUnstructuredGrid
		vtkSmartPointer<vtkUnstructuredGrid> vtk_unstructuredGrid = vtkSmartPointer<vtkUnstructuredGrid>::New();

		vtk_unstructuredGrid->SetPoints(this->mapperIjkGrid->getVtkPoints() /*createPoints()*/);

		// Define hexahedron node ordering according to Paraview convention : https://lorensen.github.io/VTKExamples/site/VTKBook/05Chapter5/#Figure%205-3
		std::array<unsigned int, 8> correspondingResqmlCornerId = {0, 1, 2, 3, 4, 5, 6, 7};
		if (supportingGrid->isRightHanded())
		{
			correspondingResqmlCornerId = {4, 5, 6, 7, 0, 1, 2, 3};
		}

		supportingGrid->loadSplitInformation();

		// Create and set the list of hexahedra of the vtkUnstructuredGrid based on the list of points already set
		uint64_t cellIndex = 0;

		uint64_t elementCountOfPatch = this->resqmlData->getElementCountOfPatch(0);
		std::unique_ptr<uint64_t[]> elementIndices(new uint64_t[elementCountOfPatch]);
		resqmlData->getElementIndicesOfPatch(0, 0, elementIndices.get());

		iCellCount = supportingGrid->getICellCount();
		jCellCount = supportingGrid->getJCellCount();
		kCellCount = supportingGrid->getKCellCount();

		initKIndex = 0;
		maxKIndex = kCellCount;

		size_t indice = 0;

		for (uint32_t vtkKCellIndex = initKIndex; vtkKCellIndex < maxKIndex; ++vtkKCellIndex)
		{
			for (uint32_t vtkJCellIndex = 0; vtkJCellIndex < jCellCount; ++vtkJCellIndex)
			{
				for (uint32_t vtkICellIndex = 0; vtkICellIndex < iCellCount; ++vtkICellIndex)
				{
					if (elementIndices[indice] == cellIndex)
					{
						vtkSmartPointer<vtkHexahedron> hex = vtkSmartPointer<vtkHexahedron>::New();

						for (uint8_t cornerId = 0; cornerId < 8; ++cornerId)
						{
							hex->GetPointIds()->SetId(cornerId,
													  supportingGrid->getXyzPointIndexFromCellCorner(vtkICellIndex, vtkJCellIndex, vtkKCellIndex, correspondingResqmlCornerId[cornerId]));
						}
						vtk_unstructuredGrid->InsertNextCell(hex->GetCellType(), hex->GetPointIds());
						indice++;
					}
					++cellIndex;
				}
			}
		}

		supportingGrid->unloadSplitInformation();
		this->vtkData->SetPartition(0, vtk_unstructuredGrid);
		this->vtkData->Modified();
	}
	else
	{
		// TODO msg d'erreur
	}
}
