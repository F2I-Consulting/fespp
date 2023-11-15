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
#include "Mapping/ResqmlIjkGridToVtkExplicitStructuredGrid.h"

#include <array>

// include VTK library
#include <vtkSmartPointer.h>
#include <vtkCellData.h>
#include <vtkDataArray.h>
#include <vtkDoubleArray.h>
#include <vtkEmptyCell.h>
#include <vtkHexahedron.h>
#include <vtkExplicitStructuredGrid.h>
#include "vtkPointData.h"

// include FESAPI
#include <fesapi/resqml2/AbstractIjkGridRepresentation.h>
#include <fesapi/resqml2/LocalDepth3dCrs.h>

// include FESPP
#include "ResqmlPropertyToVtkDataArray.h"

//----------------------------------------------------------------------------
ResqmlIjkGridToVtkExplicitStructuredGrid::ResqmlIjkGridToVtkExplicitStructuredGrid(const RESQML2_NS::AbstractIjkGridRepresentation *ijkGrid, int proc_number, int max_proc)
	: ResqmlAbstractRepresentationToVtkPartitionedDataSet(ijkGrid,
											   proc_number,
											   max_proc),
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
		const auto optim = (this->kCellCount % max_proc) > 0 ? (this->kCellCount / max_proc) + 1 : this->kCellCount / max_proc;
		this->initKIndex = this->procNumber * optim;
		if (this->initKIndex >= this->kCellCount)
		{
			this->initKIndex = 0;
			this->maxKIndex = 0;
		}
		else
		{
			this->maxKIndex = this->procNumber == this->maxProc - 1
				? this->kCellCount
				: this->initKIndex + optim;
		}
	}
	else
	{
		this->initKIndex = 0;
		this->maxKIndex = this->kCellCount;
	}

	this->vtkData = vtkSmartPointer<vtkPartitionedDataSet>::New();
}

//----------------------------------------------------------------------------
const RESQML2_NS::AbstractIjkGridRepresentation * ResqmlIjkGridToVtkExplicitStructuredGrid::getResqmlData() const
{
	return static_cast<const RESQML2_NS::AbstractIjkGridRepresentation *>(resqmlData);
}

//----------------------------------------------------------------------------
void ResqmlIjkGridToVtkExplicitStructuredGrid::checkHyperslabingCapacity(const RESQML2_NS::AbstractIjkGridRepresentation *ijkGrid)
{
	try
	{
		const RESQML2_NS::AbstractIjkGridRepresentation * ijkGrid = getResqmlData();
		const auto kInterfaceNodeCount = ijkGrid->getXyzPointCountOfKInterface();
		std::unique_ptr<double[]> allXyzPoints(new double[kInterfaceNodeCount * 3]);
		const_cast<RESQML2_NS::AbstractIjkGridRepresentation*>(ijkGrid)->getXyzPointsOfKInterface(0, allXyzPoints.get());
		this->isHyperslabed = true;
	}
	catch (const std::exception &)
	{
		this->isHyperslabed = false;
	}
}

//----------------------------------------------------------------------------
void ResqmlIjkGridToVtkExplicitStructuredGrid::loadVtkObject()
{
	const RESQML2_NS::AbstractIjkGridRepresentation* ijkGrid = getResqmlData();

	vtkExplicitStructuredGrid* vtk_explicitStructuredGrid = vtkExplicitStructuredGrid::New();


	int extent[6] = { 0, this->iCellCount, 0, this->jCellCount, this->initKIndex, this->maxKIndex };
	vtk_explicitStructuredGrid->SetExtent(extent);

	vtk_explicitStructuredGrid->SetPoints(this->getVtkPoints());

	// Check which cells have no geometry
	const uint64_t cellCount = ijkGrid->getCellCount();
	std::unique_ptr<bool[]> enabledCells(new bool[cellCount]);
	if (ijkGrid->hasCellGeometryIsDefinedFlags())
	{
		ijkGrid->getCellGeometryIsDefinedFlags(enabledCells.get());
	}
	else
	{
		std::fill_n(enabledCells.get(), cellCount, true);
	}

	const uint64_t translatePoint = ijkGrid->getXyzPointCountOfKInterface() * this->initKIndex;

	const_cast<RESQML2_NS::AbstractIjkGridRepresentation*>(ijkGrid)->loadSplitInformation();

	uint64_t cellIndex = 0;
	vtkSmartPointer<vtkIdList> nodes = vtkSmartPointer<vtkIdList>::New();
	nodes->SetNumberOfIds(8);

	for (uint_fast32_t vtkKCellIndex = this->initKIndex; vtkKCellIndex < this->maxKIndex; ++vtkKCellIndex)
	{
		for (uint_fast32_t vtkJCellIndex = 0; vtkJCellIndex < this->jCellCount; ++vtkJCellIndex)
		{
			for (uint_fast32_t vtkICellIndex = 0; vtkICellIndex < this->iCellCount; ++vtkICellIndex)
			{
				vtkIdType cellId = vtk_explicitStructuredGrid->ComputeCellId(vtkICellIndex, vtkJCellIndex, vtkKCellIndex);
				if (enabledCells[cellIndex++])
				{
					vtkIdType* indice = vtk_explicitStructuredGrid->GetCellPoints(cellId);
					indice[0] = ijkGrid->getXyzPointIndexFromCellCorner(vtkICellIndex, vtkJCellIndex, vtkKCellIndex, 0) - translatePoint;
					indice[1] = ijkGrid->getXyzPointIndexFromCellCorner(vtkICellIndex, vtkJCellIndex, vtkKCellIndex, 1) - translatePoint;
					indice[2] = ijkGrid->getXyzPointIndexFromCellCorner(vtkICellIndex, vtkJCellIndex, vtkKCellIndex, 2) - translatePoint;
					indice[3] = ijkGrid->getXyzPointIndexFromCellCorner(vtkICellIndex, vtkJCellIndex, vtkKCellIndex, 3) - translatePoint;
					indice[4] = ijkGrid->getXyzPointIndexFromCellCorner(vtkICellIndex, vtkJCellIndex, vtkKCellIndex, 4) - translatePoint;
					indice[5] = ijkGrid->getXyzPointIndexFromCellCorner(vtkICellIndex, vtkJCellIndex, vtkKCellIndex, 5) - translatePoint;
					indice[6] = ijkGrid->getXyzPointIndexFromCellCorner(vtkICellIndex, vtkJCellIndex, vtkKCellIndex, 6) - translatePoint;
					indice[7] = ijkGrid->getXyzPointIndexFromCellCorner(vtkICellIndex, vtkJCellIndex, vtkKCellIndex, 7) - translatePoint;

				}
				else {
					vtk_explicitStructuredGrid->BlankCell(cellId);
				}
			}
		}
	}

	const_cast<RESQML2_NS::AbstractIjkGridRepresentation*>(ijkGrid)->unloadSplitInformation();

	vtk_explicitStructuredGrid->CheckAndReorderFaces();
	vtk_explicitStructuredGrid->ComputeFacesConnectivityFlagsArray();

	this->vtkData->SetPartition(0, vtk_explicitStructuredGrid);
	this->vtkData->Modified();
}

//----------------------------------------------------------------------------
vtkSmartPointer<vtkPoints> ResqmlIjkGridToVtkExplicitStructuredGrid::getVtkPoints()
{
	if (this->points->GetNumberOfPoints() < 1)
	{
		createPoints();
	}
	
	return this->points;
}

//----------------------------------------------------------------------------
void ResqmlIjkGridToVtkExplicitStructuredGrid::createPoints()
{
		const RESQML2_NS::AbstractIjkGridRepresentation* ijkGrid = getResqmlData();

		this->points->SetNumberOfPoints(this->pointCount);
		size_t point_id = 0;

		if (this->isHyperslabed && !ijkGrid->isNodeGeometryCompressed())
		{
			// Take into account K gaps
			const uint32_t kGapCount = ijkGrid->getKGapsCount();
			uint32_t initKInterfaceIndex = this->initKIndex;
			uint32_t maxKInterfaceIndex = this->maxKIndex;
			if (kGapCount > 0)
			{
				std::unique_ptr<bool[]> gapAfterLayer(new bool[this->kCellCount - 1]); // gap after each layer except for the last k cell
				ijkGrid->getKGaps(gapAfterLayer.get());
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

			const uint64_t kInterfaceNodeCount = ijkGrid->getXyzPointCountOfKInterface();
			std::unique_ptr<double[]> allXyzPoints(new double[kInterfaceNodeCount * 3]);

			for (uint_fast32_t kInterface = initKInterfaceIndex; kInterface <= maxKInterfaceIndex; ++kInterface)
			{
				const_cast<RESQML2_NS::AbstractIjkGridRepresentation*>(ijkGrid)->getXyzPointsOfKInterface(kInterface, allXyzPoints.get());
				auto const* crs = ijkGrid->getLocalCrs(0);
				double xOffset = .0;
				double yOffset = .0;
				double zOffset = .0;
				double zIndice = allXyzPoints[2] > 0 ? -1. : 1.;
				if (crs != nullptr && !crs->isPartial())
				{
					xOffset = crs->getOriginOrdinal1();
					yOffset = crs->getOriginOrdinal2();
					auto const* depthCrs = dynamic_cast<RESQML2_NS::LocalDepth3dCrs const*>(crs);
					zOffset = depthCrs != nullptr ? depthCrs->getOriginDepthOrElevation() : 0;
					zIndice = crs->isDepthOriented() ? -1. : 1.;
				}
				else
				{
					vtkOutputWindowDisplayWarningText("The CRS doesn't exist or is partial");
				}
				for (uint_fast64_t nodeIndex = 0; nodeIndex < kInterfaceNodeCount * 3; nodeIndex += 3)
				{
					this->points->SetPoint(point_id++, allXyzPoints[nodeIndex] + xOffset, allXyzPoints[nodeIndex + 1] + yOffset, (allXyzPoints[nodeIndex + 2] + zOffset) * zIndice);
				}
			}
		}
		else
		{
			this->initKIndex = 0;
			this->maxKIndex = this->kCellCount;

			std::unique_ptr<double[]> allXyzPoints(new double[this->pointCount * 3]);
			ijkGrid->getXyzPointsOfAllPatchesInGlobalCrs(allXyzPoints.get());
			const size_t coordCount = this->pointCount * 3;

			const double zIndice = ijkGrid->getLocalCrs(0)->isDepthOriented() ? -1 : 1;
			for (uint_fast64_t pointIndex = 0; pointIndex < coordCount; pointIndex += 3)
			{
				this->points->SetPoint(point_id++, allXyzPoints[pointIndex], allXyzPoints[pointIndex + 1], -allXyzPoints[pointIndex + 2] * zIndice);
			}
		}
}
