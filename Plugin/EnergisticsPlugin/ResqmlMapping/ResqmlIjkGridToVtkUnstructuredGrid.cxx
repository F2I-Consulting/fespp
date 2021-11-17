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


// include F2i-consulting Energistics Standards API
#include <fesapi/resqml2/AbstractIjkGridRepresentation.h>
//#include <fesapi/resqml2/SubRepresentation.h>
#include <fesapi/resqml2/AbstractLocal3dCrs.h>

// include F2i-consulting Energistics Paraview Plugin
#include "ResqmlMapping/ResqmlPropertyToVtkDataArray.h"

//----------------------------------------------------------------------------
ResqmlIjkGridToVtkUnstructuredGrid::ResqmlIjkGridToVtkUnstructuredGrid(RESQML2_NS::AbstractIjkGridRepresentation *ijkGrid, int proc_number, int max_proc)
	: ResqmlAbstractRepresentationToVtkDataset(ijkGrid,
											   proc_number,
											   max_proc)
{
	this->iCellCount = ijkGrid->getICellCount();
	this->jCellCount = ijkGrid->getJCellCount();
	this->kCellCount = ijkGrid->getKCellCount();
	this->checkHyperslabingCapacity();
}

//----------------------------------------------------------------------------
void ResqmlIjkGridToVtkUnstructuredGrid::checkHyperslabingCapacity()
{
	auto ijkGrid = dynamic_cast<RESQML2_NS::AbstractIjkGridRepresentation *>(this->resqmlData);

	try
	{
		const auto kInterfaceNodeCount = ijkGrid->getXyzPointCountOfKInterface();
		std::unique_ptr<double[]> allXyzPoints(new double[kInterfaceNodeCount * 3]);
		ijkGrid->getXyzPointsOfKInterface(0, allXyzPoints.get());
		this->isHyperslabed = true;
	}
	catch (const std::exception &)
	{
		this->isHyperslabed = false;
	}
}

//----------------------------------------------------------------------------
void ResqmlIjkGridToVtkUnstructuredGrid::loadVtkObject()
{
	// Create and set the list of points of the vtkUnstructuredGrid
	vtkSmartPointer<vtkUnstructuredGrid> vtk_unstructuredGrid = vtkSmartPointer<vtkUnstructuredGrid>::New();

	auto ijkGrid = dynamic_cast< RESQML2_NS::AbstractIjkGridRepresentation *>(this->resqmlData);

	vtk_unstructuredGrid->SetPoints(createPoints());

	// Define hexahedron node ordering according to Paraview convention : https://lorensen.github.io/VTKExamples/site/VTKBook/05Chapter5/#Figure%205-3
	std::array<unsigned int, 8> correspondingResqmlCornerId = {0, 1, 2, 3, 4, 5, 6, 7};
	if (!ijkGrid->isRightHanded())
	{
		correspondingResqmlCornerId = {4, 5, 6, 7, 0, 1, 2, 3};
	}

	// Create and set the list of hexahedra of the vtkUnstructuredGrid based on the list of points already set
	ijkGrid->loadSplitInformation();
	uint64_t cellIndex = 0;

	const uint64_t cellCount = ijkGrid->getCellCount();
	std::unique_ptr<bool[]> enabledCells(new bool[cellCount]);
	if (ijkGrid->hasEnabledCellInformation())
	{
		ijkGrid->getEnabledCells(enabledCells.get());
	}
	else
	{
		std::fill_n(enabledCells.get(), cellCount, true);
	}

	const uint64_t translatePoint = ijkGrid->getXyzPointCountOfKInterface() * this->initKIndex;

	vtk_unstructuredGrid->Allocate(this->iCellCount * this->jCellCount);
	for (uint32_t vtkKCellIndex = this->initKIndex; vtkKCellIndex < this->maxKIndex; ++vtkKCellIndex)
	{
		for (uint32_t vtkJCellIndex = 0; vtkJCellIndex < this->jCellCount; ++vtkJCellIndex)
		{
			for (uint32_t vtkICellIndex = 0; vtkICellIndex < this->iCellCount; ++vtkICellIndex)
			{
				if (enabledCells[cellIndex])
				{
					vtkSmartPointer<vtkHexahedron> hex = vtkSmartPointer<vtkHexahedron>::New();

					for (uint8_t cornerId = 0; cornerId < 8; ++cornerId)
					{
						uint64_t pointIndex = ijkGrid->getXyzPointIndexFromCellCorner(vtkICellIndex, vtkJCellIndex, vtkKCellIndex, correspondingResqmlCornerId[cornerId]);
						hex->GetPointIds()->SetId(cornerId, pointIndex - translatePoint);
					}

					vtk_unstructuredGrid->InsertNextCell(hex->GetCellType(), hex->GetPointIds());
				}
				else
				{
					vtkSmartPointer<vtkEmptyCell> emptyCell = vtkSmartPointer<vtkEmptyCell>::New();
					vtk_unstructuredGrid->InsertNextCell(emptyCell->GetCellType(), emptyCell->GetPointIds());
				}
				++cellIndex;
			}
		}
	}

	ijkGrid->unloadSplitInformation();

	this->vtkData = vtk_unstructuredGrid;
	this->vtkData->Modified();
}

//----------------------------------------------------------------------------
vtkSmartPointer<vtkPoints> ResqmlIjkGridToVtkUnstructuredGrid::createPoints()
{
	vtkSmartPointer<vtkPoints> points = vtkSmartPointer<vtkPoints>::New();

	auto ijkGrid = dynamic_cast<RESQML2_NS::AbstractIjkGridRepresentation *>(this->resqmlData);

	if (this->isHyperslabed && !ijkGrid->isNodeGeometryCompressed())
	{
		const auto optim = this->kCellCount / this->maxProc;
		this->initKIndex = this->procNumber * optim;
		this->maxKIndex = this->procNumber == this->maxProc - 1
							  ? this->kCellCount
							  : this->initKIndex + optim;

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
		for (uint32_t kInterface = initKInterfaceIndex; kInterface <= maxKInterfaceIndex; ++kInterface)
		{
			this->pointCount += kInterfaceNodeCount;

			ijkGrid->getXyzPointsOfKInterface(kInterface, allXyzPoints.get());
			const double zIndice = ijkGrid->getLocalCrs(0)->isDepthOriented() ? -1 : 1;
			for (uint64_t nodeIndex = 0; nodeIndex < kInterfaceNodeCount * 3; nodeIndex += 3)
			{
				points->InsertNextPoint(allXyzPoints[nodeIndex], allXyzPoints[nodeIndex + 1], allXyzPoints[nodeIndex + 2] * zIndice);
			}
		}
	}
	else
	{
		this->initKIndex = 0;
		this->maxKIndex = this->kCellCount;

		// POINT
		this->pointCount = ijkGrid->getXyzPointCountOfAllPatches();

		double *allXyzPoints = new double[this->pointCount * 3]; // Will be deleted by VTK
		ijkGrid->getXyzPointsOfAllPatchesInGlobalCrs(allXyzPoints);
		const size_t coordCount = this->pointCount * 3;
		if (ijkGrid->getLocalCrs(0)->isDepthOriented())
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
		points->SetData(vtkUnderlyingArray);
	}
	return points;
}
