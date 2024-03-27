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
ResqmlIjkGridToVtkExplicitStructuredGrid::ResqmlIjkGridToVtkExplicitStructuredGrid(const RESQML2_NS::AbstractIjkGridRepresentation *ijkGrid, uint32_t p_procNumber, uint32_t p_maxProc)
	: ResqmlAbstractRepresentationToVtkPartitionedDataSet(ijkGrid,
														  p_procNumber,
														  p_maxProc),
	  points(vtkSmartPointer<vtkPoints>::New()),
	  pointer_on_points(0)
{
	_iCellCount = ijkGrid->getICellCount();
	_jCellCount = ijkGrid->getJCellCount();
	_kCellCount = ijkGrid->getKCellCount();
	_pointCount = ijkGrid->getXyzPointCountOfAllPatches();
	checkHyperslabingCapacity(ijkGrid);

	if (_isHyperslabed)
	{
		const auto optim = (_kCellCount % p_maxProc) > 0 ? (_kCellCount / p_maxProc) + 1 : _kCellCount / p_maxProc;
		_initKIndex = _procNumber * optim;
		if (_initKIndex >= _kCellCount)
		{
			_initKIndex = 0;
			_maxKIndex = 0;
		}
		else
		{
			_maxKIndex = _procNumber == _maxProc - 1
								  ? _kCellCount
								  : _initKIndex + optim;
		}
	}
	else
	{
		_initKIndex = 0;
		_maxKIndex = _kCellCount;
	}

	_vtkData = vtkSmartPointer<vtkPartitionedDataSet>::New();
}

//----------------------------------------------------------------------------
const RESQML2_NS::AbstractIjkGridRepresentation *ResqmlIjkGridToVtkExplicitStructuredGrid::getResqmlData() const
{
	return static_cast<const RESQML2_NS::AbstractIjkGridRepresentation *>(_resqmlData);
}

//----------------------------------------------------------------------------
void ResqmlIjkGridToVtkExplicitStructuredGrid::checkHyperslabingCapacity(const RESQML2_NS::AbstractIjkGridRepresentation *ijkGrid)
{
	try
	{
		const RESQML2_NS::AbstractIjkGridRepresentation *ijkGrid = getResqmlData();
		const auto kInterfaceNodeCount = ijkGrid->getXyzPointCountOfKInterface();
		std::unique_ptr<double[]> allXyzPoints(new double[kInterfaceNodeCount * 3]);
		const_cast<RESQML2_NS::AbstractIjkGridRepresentation *>(ijkGrid)->getXyzPointsOfKInterface(0, allXyzPoints.get());
		_isHyperslabed = true;
	}
	catch (const std::exception &)
	{
		_isHyperslabed = false;
	}
}

//----------------------------------------------------------------------------
void ResqmlIjkGridToVtkExplicitStructuredGrid::loadVtkObject()
{
	const RESQML2_NS::AbstractIjkGridRepresentation *ijkGrid = getResqmlData();

	vtkExplicitStructuredGrid *vtk_explicitStructuredGrid = vtkExplicitStructuredGrid::New();

	int extent[6] = {0, _iCellCount, 0, _jCellCount, _initKIndex, _maxKIndex};
	vtk_explicitStructuredGrid->SetExtent(extent);

	vtk_explicitStructuredGrid->SetPoints(getVtkPoints());

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

	const uint64_t translatePoint = ijkGrid->getXyzPointCountOfKInterface() * _initKIndex;

	const_cast<RESQML2_NS::AbstractIjkGridRepresentation *>(ijkGrid)->loadSplitInformation();

	uint64_t cellIndex = 0;
	vtkSmartPointer<vtkIdList> nodes = vtkSmartPointer<vtkIdList>::New();
	nodes->SetNumberOfIds(8);

	for (uint_fast32_t vtkKCellIndex = _initKIndex; vtkKCellIndex < _maxKIndex; ++vtkKCellIndex)
	{
		for (uint_fast32_t vtkJCellIndex = 0; vtkJCellIndex < _jCellCount; ++vtkJCellIndex)
		{
			for (uint_fast32_t vtkICellIndex = 0; vtkICellIndex < _iCellCount; ++vtkICellIndex)
			{
				vtkIdType cellId = vtk_explicitStructuredGrid->ComputeCellId(vtkICellIndex, vtkJCellIndex, vtkKCellIndex);
				if (enabledCells[cellIndex++])
				{
					vtkIdType *indice = vtk_explicitStructuredGrid->GetCellPoints(cellId);
					indice[0] = ijkGrid->getXyzPointIndexFromCellCorner(vtkICellIndex, vtkJCellIndex, vtkKCellIndex, 0) - translatePoint;
					indice[1] = ijkGrid->getXyzPointIndexFromCellCorner(vtkICellIndex, vtkJCellIndex, vtkKCellIndex, 1) - translatePoint;
					indice[2] = ijkGrid->getXyzPointIndexFromCellCorner(vtkICellIndex, vtkJCellIndex, vtkKCellIndex, 2) - translatePoint;
					indice[3] = ijkGrid->getXyzPointIndexFromCellCorner(vtkICellIndex, vtkJCellIndex, vtkKCellIndex, 3) - translatePoint;
					indice[4] = ijkGrid->getXyzPointIndexFromCellCorner(vtkICellIndex, vtkJCellIndex, vtkKCellIndex, 4) - translatePoint;
					indice[5] = ijkGrid->getXyzPointIndexFromCellCorner(vtkICellIndex, vtkJCellIndex, vtkKCellIndex, 5) - translatePoint;
					indice[6] = ijkGrid->getXyzPointIndexFromCellCorner(vtkICellIndex, vtkJCellIndex, vtkKCellIndex, 6) - translatePoint;
					indice[7] = ijkGrid->getXyzPointIndexFromCellCorner(vtkICellIndex, vtkJCellIndex, vtkKCellIndex, 7) - translatePoint;
				}
				else
				{
					vtk_explicitStructuredGrid->BlankCell(cellId);
				}
			}
		}
	}

	const_cast<RESQML2_NS::AbstractIjkGridRepresentation *>(ijkGrid)->unloadSplitInformation();

	vtk_explicitStructuredGrid->CheckAndReorderFaces();
	vtk_explicitStructuredGrid->ComputeFacesConnectivityFlagsArray();

	_vtkData->SetPartition(0, vtk_explicitStructuredGrid);
	_vtkData->Modified();
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
	const RESQML2_NS::AbstractIjkGridRepresentation *ijkGrid = getResqmlData();

	this->points->SetNumberOfPoints(_pointCount);
	size_t point_id = 0;

	if (_isHyperslabed && !ijkGrid->isNodeGeometryCompressed())
	{
		// Take into account K gaps
		const uint32_t kGapCount = ijkGrid->getKGapsCount();
		uint32_t initKInterfaceIndex = _initKIndex;
		uint32_t maxKInterfaceIndex = _maxKIndex;
		if (kGapCount > 0)
		{
			std::unique_ptr<bool[]> gapAfterLayer(new bool[_kCellCount - 1]); // gap after each layer except for the last k cell
			ijkGrid->getKGaps(gapAfterLayer.get());
			uint32_t kLayer = 0;
			for (; kLayer < _initKIndex; ++kLayer)
			{
				if (gapAfterLayer[kLayer])
				{
					++initKInterfaceIndex;
					++maxKInterfaceIndex;
				}
			}
			for (; kLayer < _kCellCount - 1 && kLayer < _maxKIndex; ++kLayer)
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
			const_cast<RESQML2_NS::AbstractIjkGridRepresentation *>(ijkGrid)->getXyzPointsOfKInterface(kInterface, allXyzPoints.get());
			auto const *crs = ijkGrid->getLocalCrs(0);
			double xOffset = .0;
			double yOffset = .0;
			double zOffset = .0;
			double zIndice = allXyzPoints[2] > 0 ? -1. : 1.;
			if (crs != nullptr && !crs->isPartial())
			{
				xOffset = crs->getOriginOrdinal1();
				yOffset = crs->getOriginOrdinal2();
				auto const *depthCrs = dynamic_cast<RESQML2_NS::LocalDepth3dCrs const *>(crs);
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
		_initKIndex = 0;
		_maxKIndex = _kCellCount;

		std::unique_ptr<double[]> allXyzPoints(new double[_pointCount * 3]);
		auto const *crs = ijkGrid->getLocalCrs(0);
		if (crs != nullptr && !crs->isPartial())
		{
			ijkGrid->getXyzPointsOfAllPatchesInGlobalCrs(allXyzPoints.get());
			const size_t coordCount = _pointCount * 3;

			const double zIndice = ijkGrid->getLocalCrs(0)->isDepthOriented() ? -1 : 1;
			for (uint_fast64_t pointIndex = 0; pointIndex < coordCount; pointIndex += 3)
			{
				this->points->SetPoint(point_id++, allXyzPoints[pointIndex], allXyzPoints[pointIndex + 1], allXyzPoints[pointIndex + 2] * zIndice);
			}
		}
		else
		{
			vtkOutputWindowDisplayWarningText("The CRS doesn't exist or is partial");
		}
	}
}
