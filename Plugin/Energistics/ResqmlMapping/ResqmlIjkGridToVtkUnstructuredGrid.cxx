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
#include "ResqmlMapping/ResqmlIjkGridToVtkUnstructuredGrid.h"

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
#include <fesapi/resqml2/AbstractIjkGridRepresentation.h>
#include <fesapi/resqml2/LocalDepth3dCrs.h>

// include FESPP
#include "ResqmlPropertyToVtkDataArray.h"

//----------------------------------------------------------------------------
ResqmlIjkGridToVtkUnstructuredGrid::ResqmlIjkGridToVtkUnstructuredGrid(RESQML2_NS::AbstractIjkGridRepresentation *ijkGrid, int proc_number, int max_proc)
	: ResqmlAbstractRepresentationToVtkPartitionedDataSet(ijkGrid,
											   proc_number - 1,
											   max_proc),
	  resqmlData(ijkGrid),
	points(vtkSmartPointer<vtkPoints>::New()),
	pointer_on_points(0)
{
	this->iCellCount = ijkGrid->getICellCount();
	this->jCellCount = ijkGrid->getJCellCount();
	this->kCellCount = ijkGrid->getKCellCount();
	this->pointCount = ijkGrid->getXyzPointCountOfAllPatches();
	this->checkHyperslabingCapacity(ijkGrid);

	if (this->isHyperslabed)
	{
		const auto optim = this->kCellCount / max_proc;
		this->initKIndex = this->procNumber * optim;
		this->maxKIndex = this->procNumber == this->maxProc - 1
							  ? this->kCellCount
							  : this->initKIndex + optim;
	}
	else
	{
		this->initKIndex = 0;
		this->maxKIndex = this->kCellCount;
	}

	this->vtkData = vtkSmartPointer<vtkPartitionedDataSet>::New();
}

//----------------------------------------------------------------------------
void ResqmlIjkGridToVtkUnstructuredGrid::checkHyperslabingCapacity(RESQML2_NS::AbstractIjkGridRepresentation *ijkGrid)
{
	try
	{
		const auto kInterfaceNodeCount = this->resqmlData->getXyzPointCountOfKInterface();
		std::unique_ptr<double[]> allXyzPoints(new double[kInterfaceNodeCount * 3]);
		this->resqmlData->getXyzPointsOfKInterface(0, allXyzPoints.get());
		this->isHyperslabed = true;
	}
	catch (const std::exception &)
	{
		this->isHyperslabed = false;
	}
}

//----------------------------------------------------------------------------
// Create and set the list of hexahedra of the vtkUnstructuredGrid based on the list of points already set
void ResqmlIjkGridToVtkUnstructuredGrid::loadVtkObject()
{
	// Create and set the list of points of the vtkUnstructuredGrid
	vtkSmartPointer<vtkUnstructuredGrid> vtk_unstructuredGrid = vtkSmartPointer<vtkUnstructuredGrid>::New();
	vtk_unstructuredGrid->SetPoints(this->getVtkPoints());

	// Define hexahedron node ordering according to Paraview convention : https://lorensen.github.io/VTKExamples/site/VTKBook/05Chapter5/#Figure%205-3
	std::array<unsigned int, 8> correspondingResqmlCornerId = {0, 1, 2, 3, 4, 5, 6, 7};
	if (!this->resqmlData->isRightHanded())
	{
		correspondingResqmlCornerId = {4, 5, 6, 7, 0, 1, 2, 3};
	}

	// Check which cells have no geometry
	const uint64_t cellCount = this->resqmlData->getCellCount();
	std::unique_ptr<bool[]> enabledCells(new bool[cellCount]);
	if (this->resqmlData->hasCellGeometryIsDefinedFlags())
	{
		this->resqmlData->getCellGeometryIsDefinedFlags(enabledCells.get());
	}
	else
	{
		std::fill_n(enabledCells.get(), cellCount, true);
	}

	const uint64_t translatePoint = this->resqmlData->getXyzPointCountOfKInterface() * this->initKIndex;

	uint64_t cellIndex = 0;
	vtk_unstructuredGrid->Allocate(this->iCellCount * this->jCellCount);
	this->resqmlData->loadSplitInformation();
	for (uint32_t vtkKCellIndex = this->initKIndex; vtkKCellIndex < this->maxKIndex; ++vtkKCellIndex)
	{
		for (uint32_t vtkJCellIndex = 0; vtkJCellIndex < this->jCellCount; ++vtkJCellIndex)
		{
			for (uint32_t vtkICellIndex = 0; vtkICellIndex < this->iCellCount; ++vtkICellIndex)
			{
				vtkSmartPointer<vtkCell> cell;
				if (enabledCells[cellIndex++])
				{
					cell = vtkSmartPointer<vtkHexahedron>::New();
					for (uint8_t cornerId = 0; cornerId < 8; ++cornerId)
					{
						cell->GetPointIds()->SetId(cornerId,
												   resqmlData->getXyzPointIndexFromCellCorner(vtkICellIndex, vtkJCellIndex, vtkKCellIndex, correspondingResqmlCornerId[cornerId]) - translatePoint);
					}
				}
				else
				{
					cell = vtkSmartPointer<vtkEmptyCell>::New();
				}
				vtk_unstructuredGrid->InsertNextCell(cell->GetCellType(), cell->GetPointIds());
			}
		}
	}
	this->resqmlData->unloadSplitInformation();

	this->vtkData->SetPartition(0, vtk_unstructuredGrid);
	this->vtkData->Modified();
}

vtkSmartPointer<vtkPoints> ResqmlIjkGridToVtkUnstructuredGrid::getVtkPoints()
{
	if (this->points->GetNumberOfPoints() < 1)
	{
		createPoints();
	}
	
	return this->points;
}

//----------------------------------------------------------------------------
void ResqmlIjkGridToVtkUnstructuredGrid::createPoints()
{

	if (this->isHyperslabed && !this->resqmlData->isNodeGeometryCompressed())
	{
		// Take into account K gaps
		const uint32_t kGapCount = this->resqmlData->getKGapsCount();
		uint32_t initKInterfaceIndex = this->initKIndex;
		uint32_t maxKInterfaceIndex = this->maxKIndex;
		if (kGapCount > 0)
		{
			std::unique_ptr<bool[]> gapAfterLayer(new bool[this->kCellCount - 1]); // gap after each layer except for the last k cell
			this->resqmlData->getKGaps(gapAfterLayer.get());
			uint32_t kLayer = 0;
			for (; kLayer < this->initKIndex; ++kLayer)
			{
				if (gapAfterLayer[kLayer])
				{
					++initKInterfaceIndex;
					++maxKInterfaceIndex;
				}
			}
			for (; kLayer < this->kCellCount - 1 && kLayer < this->maxKIndex; ++kLayer)
			{
				if (gapAfterLayer[kLayer])
				{
					++maxKInterfaceIndex;
				}
			}
		}

		const uint64_t kInterfaceNodeCount = this->resqmlData->getXyzPointCountOfKInterface();
		std::unique_ptr<double[]> allXyzPoints(new double[kInterfaceNodeCount * 3]);
		for (uint32_t kInterface = initKInterfaceIndex; kInterface <= maxKInterfaceIndex; ++kInterface)
		{
			this->resqmlData->getXyzPointsOfKInterface(kInterface, allXyzPoints.get());
			auto const *crs = this->resqmlData->getLocalCrs(0);
			const double xOffset = crs->getOriginOrdinal1();
			const double yOffset = crs->getOriginOrdinal2();
			auto const *depthCrs = dynamic_cast<RESQML2_NS::LocalDepth3dCrs const *>(crs);
			const double zOffset = depthCrs != nullptr ? depthCrs->getOriginDepthOrElevation() : 0;
			const double zIndice = crs->isDepthOriented() ? -1 : 1;
			for (uint64_t nodeIndex = 0; nodeIndex < kInterfaceNodeCount * 3; nodeIndex += 3)
			{
				this->points->InsertNextPoint(allXyzPoints[nodeIndex] + xOffset, allXyzPoints[nodeIndex + 1] + yOffset, (allXyzPoints[nodeIndex + 2] + zOffset) * zIndice);
			}
		}
	}
	else
	{
		this->initKIndex = 0;
		this->maxKIndex = this->kCellCount;

		std::unique_ptr<double[]> allXyzPoints(new double[this->pointCount * 3]);
		this->resqmlData->getXyzPointsOfAllPatchesInGlobalCrs(allXyzPoints.get());
		const size_t coordCount = this->pointCount * 3;

		const double zIndice = this->resqmlData->getLocalCrs(0)->isDepthOriented() ? -1 : 1;
		for (uint64_t pointIndex = 0; pointIndex < coordCount; pointIndex += 3)
		{
			this->points->InsertNextPoint(allXyzPoints[pointIndex], allXyzPoints[pointIndex + 1], -allXyzPoints[pointIndex + 2] * zIndice);
		}
	}
}
