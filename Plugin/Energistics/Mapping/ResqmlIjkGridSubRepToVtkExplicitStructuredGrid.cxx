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
#include "Mapping/ResqmlIjkGridSubRepToVtkExplicitStructuredGrid.h"

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
#include "ResqmlIjkGridToVtkExplicitStructuredGrid.h"

//----------------------------------------------------------------------------
ResqmlIjkGridSubRepToVtkExplicitStructuredGrid::ResqmlIjkGridSubRepToVtkExplicitStructuredGrid(const RESQML2_NS::SubRepresentation* subRep, ResqmlIjkGridToVtkExplicitStructuredGrid* support, int p_procNumber, int p_maxProc)
	: ResqmlAbstractRepresentationToVtkPartitionedDataSet(subRep,
		p_procNumber,
		p_maxProc),
	mapperIjkGrid(support)
{
	_iCellCount = subRep->getElementCountOfPatch(0);
	_pointCount = subRep->getSupportingRepresentation(0)->getXyzPointCountOfAllPatches();

	_vtkData = vtkSmartPointer<vtkPartitionedDataSet>::New();
}

//----------------------------------------------------------------------------
const RESQML2_NS::SubRepresentation* ResqmlIjkGridSubRepToVtkExplicitStructuredGrid::getResqmlData() const
{
	return static_cast<const RESQML2_NS::SubRepresentation*>(_resqmlData);
}

//----------------------------------------------------------------------------
void ResqmlIjkGridSubRepToVtkExplicitStructuredGrid::loadVtkObject()
{
	RESQML2_NS::SubRepresentation const* subRep = getResqmlData();
	auto* supportingGrid = dynamic_cast<RESQML2_NS::AbstractIjkGridRepresentation*>(subRep->getSupportingRepresentation(0));
	if (supportingGrid == nullptr) {
		vtkOutputWindowDisplayWarningText(("SubRepresentation (" + subRep->getUuid() + ") has no suuport grid\n").c_str());
		return;
	}
	if (subRep->areElementIndicesPairwise(0)) {
		vtkOutputWindowDisplayWarningText(("SubRepresentation (" + subRep->getUuid() + ") has elements indices pairwise and is not yet implemented\n").c_str());
		return;
	}
	auto elementType = subRep->getElementKindOfPatch(0, 0);
	if (elementType != gsoap_eml2_3::eml23__IndexableElement::cells) {
		vtkOutputWindowDisplayWarningText(("SubRepresentation (" + subRep->getUuid() + ") the kind elements and is not yet implemented. (only cells kind is supported for now)\n").c_str());
		return;
	}

	// Create and set the list of points of the vtkUnstructuredGrid
	vtkSmartPointer<vtkUnstructuredGrid> vtk_unstructuredGrid = vtkSmartPointer<vtkUnstructuredGrid>::New();

	vtk_unstructuredGrid->SetPoints(this->getMapperVtkPoint());

	// Define hexahedron node ordering according to Paraview convention : https://lorensen.github.io/VTKExamples/site/VTKBook/05Chapter5/#Figure%205-3
	std::array<unsigned int, 8> correspondingResqmlCornerId = { 0, 1, 2, 3, 4, 5, 6, 7 };
	if (supportingGrid->isRightHanded())
	{
		correspondingResqmlCornerId = { 4, 5, 6, 7, 0, 1, 2, 3 };
	}

	supportingGrid->loadSplitInformation();

	// Create and set the list of hexahedra of the vtkUnstructuredGrid based on the list of points already set
	uint64_t cellIndex = 0;

	uint64_t elementCountOfPatch = subRep->getElementCountOfPatch(0);
	std::unique_ptr<uint64_t[]> elementIndices(new uint64_t[elementCountOfPatch]);
	subRep->getElementIndicesOfPatch(0, 0, elementIndices.get());

	_iCellCount = supportingGrid->getICellCount();
	_jCellCount = supportingGrid->getJCellCount();
	_kCellCount = supportingGrid->getKCellCount();

	size_t indice = 0;

	for (uint_fast32_t vtkKCellIndex = 0; vtkKCellIndex < _kCellCount; ++vtkKCellIndex)
	{
		for (uint_fast32_t vtkJCellIndex = 0; vtkJCellIndex < _jCellCount; ++vtkJCellIndex)
		{
			for (uint_fast32_t vtkICellIndex = 0; vtkICellIndex < _iCellCount; ++vtkICellIndex)
			{
				if (elementIndices[indice] == cellIndex)
				{
					vtkSmartPointer<vtkHexahedron> hex = vtkSmartPointer<vtkHexahedron>::New();

					for (uint_fast8_t cornerId = 0; cornerId < 8; ++cornerId)
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
	_vtkData->SetPartition(0, vtk_unstructuredGrid);
	_vtkData->Modified();
}

//----------------------------------------------------------------------------
std::string ResqmlIjkGridSubRepToVtkExplicitStructuredGrid::unregisterToMapperSupportingGrid()
{
	this->mapperIjkGrid->unregisterSubRep();
	return this->mapperIjkGrid->getUuid();
}

//----------------------------------------------------------------------------
vtkSmartPointer<vtkPoints> ResqmlIjkGridSubRepToVtkExplicitStructuredGrid::getMapperVtkPoint()
{
	this->mapperIjkGrid->registerSubRep();
	return this->mapperIjkGrid->getVtkPoints();
}
