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
#include "VtkIjkGridRepresentation.h"

#include <array>

// include VTK library
#include <vtkCellData.h>
#include <vtkDataArray.h>
#include <vtkEmptyCell.h>
#include <vtkHexahedron.h>

// include F2i-consulting Energistics Standards API
#include <fesapi/resqml2/AbstractIjkGridRepresentation.h>
#include <fesapi/resqml2/SubRepresentation.h>

// include F2i-consulting Energistics Standards ParaView Plugin
#include "VtkProperty.h"

//----------------------------------------------------------------------------
VtkIjkGridRepresentation::VtkIjkGridRepresentation(const std::string & fileName, const std::string & name, const std::string & uuid, const std::string & uuidParent, COMMON_NS::DataObjectRepository const * pckEPCRep, COMMON_NS::DataObjectRepository const * pckEPCSubRep, int idProc, int maxProc) :
	VtkResqml2UnstructuredGrid(fileName, name, uuid, uuidParent, pckEPCRep, pckEPCSubRep, idProc, maxProc), lastProperty(""),
	iCellCount(0), jCellCount(0), kCellCount(0), initKIndex(0), maxKIndex(0), isHyperslabed(false)
{
}

//----------------------------------------------------------------------------
vtkSmartPointer<vtkPoints> VtkIjkGridRepresentation::createPoints()
{
	RESQML2_NS::AbstractIjkGridRepresentation* ijkGridRepresentation = subRepresentation
		? epcPackageRepresentation->getDataObjectByUuid<RESQML2_NS::AbstractIjkGridRepresentation>(getParent())
		: epcPackageRepresentation->getDataObjectByUuid<RESQML2_NS::AbstractIjkGridRepresentation>(getUuid());
	if (ijkGridRepresentation == nullptr) {
		throw std::logic_error("The object is not an IjkGridRepresentation");
	}

	iCellCount = ijkGridRepresentation->getICellCount();
	jCellCount = ijkGridRepresentation->getJCellCount();
	kCellCount = ijkGridRepresentation->getKCellCount();

	checkHyperslabingCapacity(ijkGridRepresentation);
	if (isHyperslabed && !ijkGridRepresentation->isNodeGeometryCompressed()) {
		const auto optim = kCellCount / getMaxProc();
		initKIndex = getIdProc() * optim;
		maxKIndex = getIdProc() == getMaxProc() - 1
			? kCellCount
			: initKIndex + optim;

		// Take into account K gaps
		const uint32_t kGapCount = ijkGridRepresentation->getKGapsCount();
		uint32_t initKInterfaceIndex = initKIndex;
		uint32_t maxKInterfaceIndex = maxKIndex;
		if (kGapCount > 0) {
			std::unique_ptr<bool[]> gapAfterLayer(new bool[kCellCount - 1]); // gap after each layer except for the last k cell
			ijkGridRepresentation->getKGaps(gapAfterLayer.get());
			uint32_t kLayer = 0;
			for (; kLayer < initKIndex; ++kLayer) {
				if (gapAfterLayer[kLayer]) {
					++initKInterfaceIndex;
					++maxKInterfaceIndex;
				}
			}
			for (; kLayer < kCellCount - 1 && kLayer < maxKIndex; ++kLayer) {
				if (gapAfterLayer[kLayer]) {
					++maxKInterfaceIndex;
				}
			}
		}

		const uint64_t kInterfaceNodeCount = ijkGridRepresentation->getXyzPointCountOfKInterface();
		std::unique_ptr<double[]> allXyzPoints(new double[kInterfaceNodeCount * 3]);

		vtkSmartPointer<vtkPoints> points = vtkSmartPointer<vtkPoints>::New();
		for (uint32_t kInterface = initKInterfaceIndex; kInterface <= maxKInterfaceIndex; ++kInterface) {
			ijkGridRepresentation->getXyzPointsOfKInterface(kInterface, allXyzPoints.get());
			const double zIndice = ijkGridRepresentation->getLocalCrs(0)->isDepthOriented() ? -1 : 1;

			for (uint64_t nodeIndex = 0; nodeIndex < kInterfaceNodeCount * 3; nodeIndex += 3) {
				points->InsertNextPoint(allXyzPoints[nodeIndex], allXyzPoints[nodeIndex + 1], allXyzPoints[nodeIndex + 2] * zIndice);
			}
		}

		return points;
	}
	else {
		initKIndex = 0;
		maxKIndex = kCellCount;

		// POINT
		const auto nodeCount = ijkGridRepresentation->getXyzPointCountOfAllPatches();
		double* allXyzPoints = new double[nodeCount * 3]; // Will be deleted by VTK
		ijkGridRepresentation->getXyzPointsOfAllPatchesInGlobalCrs(allXyzPoints);
		return createVtkPoints(nodeCount, allXyzPoints, ijkGridRepresentation->getLocalCrs(0));
	}
}

//----------------------------------------------------------------------------
void VtkIjkGridRepresentation::visualize(const std::string & uuid)
{
	if (vtkOutput == nullptr) {	// => REPRESENTATION
		createVtkUnstructuredGrid(subRepresentation
			? epcPackageSubRepresentation->getDataObjectByUuid<RESQML2_NS::AbstractRepresentation>(getUuid())
			: epcPackageRepresentation->getDataObjectByUuid<RESQML2_NS::AbstractRepresentation>(getUuid()));
	}

	if (uuid != getUuid()) { // => PROPERTY UUID
		if (subRepresentation) {
			RESQML2_NS::SubRepresentation* subRep = epcPackageSubRepresentation->getDataObjectByUuid<RESQML2_NS::SubRepresentation>(getUuid());
			if (isHyperslabed) {
				addProperty(uuid, uuidToVtkProperty[uuid]->loadValuesPropertySet(subRep->getValuesPropertySet(), iCellCount*jCellCount*(maxKIndex - initKIndex), 0, iCellCount, jCellCount, maxKIndex - initKIndex, initKIndex));
			}
			else {
				addProperty(uuid, uuidToVtkProperty[uuid]->loadValuesPropertySet(subRep->getValuesPropertySet(), subRep->getElementCountOfPatch(0), 0));
			}
		}
		else {
			RESQML2_NS::AbstractIjkGridRepresentation* ijkGrid = epcPackageRepresentation->getDataObjectByUuid<RESQML2_NS::AbstractIjkGridRepresentation>(getUuid());
			if (isHyperslabed) {
				addProperty(uuid, uuidToVtkProperty[uuid]->loadValuesPropertySet(ijkGrid->getValuesPropertySet(), iCellCount*jCellCount*(maxKIndex - initKIndex), 0, iCellCount, jCellCount, maxKIndex - initKIndex, initKIndex));
			}
			else {
				addProperty(uuid, uuidToVtkProperty[uuid]->loadValuesPropertySet(ijkGrid->getValuesPropertySet(), iCellCount*jCellCount*kCellCount, 0));
			}
		}
	}
}

//----------------------------------------------------------------------------
void VtkIjkGridRepresentation::checkHyperslabingCapacity(RESQML2_NS::AbstractIjkGridRepresentation* ijkGridRepresentation)
{
	try {
		const auto kInterfaceNodeCount = ijkGridRepresentation->getXyzPointCountOfKInterface();
		std::unique_ptr<double[]> allXyzPoints(new double[kInterfaceNodeCount * 3]);
		ijkGridRepresentation->getXyzPointsOfKInterface(0, allXyzPoints.get());
		isHyperslabed = true;
	}
	catch (const std::exception&) {
		isHyperslabed = false;
	}
}

//----------------------------------------------------------------------------
void VtkIjkGridRepresentation::addProperty(const std::string & uuidProperty, vtkDataArray* dataProperty)
{
	vtkOutput->Modified();
	vtkOutput->GetCellData()->AddArray(dataProperty);
	vtkOutput->Modified();
	lastProperty = uuidProperty;
}

//----------------------------------------------------------------------------
long VtkIjkGridRepresentation::getAttachmentPropertyCount(const std::string &, VtkEpcCommon::FesppAttachmentProperty propertyUnit)
{
	if (propertyUnit == VtkEpcCommon::FesppAttachmentProperty::CELLS) {
		if (subRepresentation) {
			// SubRep
			return isHyperslabed
				? iCellCount*jCellCount*(maxKIndex - initKIndex)
				: epcPackageSubRepresentation->getDataObjectByUuid<RESQML2_NS::SubRepresentation>(getUuid())->getElementCountOfPatch(0);
		}
		else {
			// Ijk Grid
			return epcPackageRepresentation->getDataObjectByUuid<RESQML2_NS::AbstractIjkGridRepresentation>(getUuid())->getCellCount();
		}
	}

	return 0;
}

//----------------------------------------------------------------------------
int VtkIjkGridRepresentation::getICellCount(const std::string &) const
{
	return subRepresentation
		? iCellCount
		: epcPackageRepresentation->getDataObjectByUuid<RESQML2_NS::AbstractIjkGridRepresentation>(getUuid())->getICellCount();
}

//----------------------------------------------------------------------------
int VtkIjkGridRepresentation::getJCellCount(const std::string &) const
{
	return subRepresentation
		? jCellCount
		: epcPackageRepresentation->getDataObjectByUuid<RESQML2_NS::AbstractIjkGridRepresentation>(getUuid())->getJCellCount();
}

//----------------------------------------------------------------------------
int VtkIjkGridRepresentation::getKCellCount(const std::string &) const
{
	return subRepresentation
		? kCellCount
		: epcPackageRepresentation->getDataObjectByUuid<RESQML2_NS::AbstractIjkGridRepresentation>(getUuid())->getKCellCount();
}

//----------------------------------------------------------------------------
int VtkIjkGridRepresentation::getInitKIndex(const std::string &) const
{
	return initKIndex;
}

//----------------------------------------------------------------------------
void VtkIjkGridRepresentation::createVtkUnstructuredGrid(RESQML2_NS::AbstractRepresentation* ijkOrSubrep)
{
	// Check if we are rendering an entire RESQML IJK grid or a RESQML subrepresentation of a RESQML IJK grid.
	RESQML2_NS::SubRepresentation const * subRepresentation = dynamic_cast<RESQML2_NS::SubRepresentation*>(ijkOrSubrep);
	RESQML2_NS::AbstractIjkGridRepresentation* ijkGridRepresentation = subRepresentation != nullptr
		? epcPackageRepresentation->getDataObjectByUuid<RESQML2_NS::AbstractIjkGridRepresentation>(subRepresentation->getSupportingRepresentationDor(0).getUuid())
		: dynamic_cast<RESQML2_NS::AbstractIjkGridRepresentation*>(ijkOrSubrep);

	if (ijkGridRepresentation == nullptr) {
		vtkOutputWindowDisplayErrorText("The input data is neither a subrepresentation of an IJK grid nor an entire IJK grid.\n");
		return;
	}

	// Create and set the list of points of the vtkUnstructuredGrid
	vtkOutput = vtkSmartPointer<vtkUnstructuredGrid>::New();
	vtkOutput->SetPoints(createPoints());

	// Define hexahedron node ordering according to Paraview convention : https://lorensen.github.io/VTKExamples/site/VTKBook/05Chapter5/#Figure%205-3
	std::array<unsigned int, 8> correspondingResqmlCornerId = { 0, 1, 2, 3, 4, 5, 6, 7 };
	if (!ijkGridRepresentation->isRightHanded()) {
		correspondingResqmlCornerId = { 4, 5, 6, 7, 0, 1, 2, 3 };
	}

	// Create and set the list of hexahedra of the vtkUnstructuredGrid based on the list of points already set
	ijkGridRepresentation->loadSplitInformation();
	uint64_t cellIndex = 0;
	if (subRepresentation != nullptr) {
		uint64_t elementCountOfPatch = subRepresentation->getElementCountOfPatch(0);
		std::unique_ptr<uint64_t[]> elementIndices(new uint64_t[elementCountOfPatch]);
		subRepresentation->getElementIndicesOfPatch(0, 0, elementIndices.get());

		iCellCount = ijkGridRepresentation->getICellCount();
		jCellCount = ijkGridRepresentation->getJCellCount();
		kCellCount = ijkGridRepresentation->getKCellCount();

		initKIndex = 0;
		maxKIndex = kCellCount;

		size_t indice = 0;

		for (uint32_t vtkKCellIndex = initKIndex; vtkKCellIndex < maxKIndex; ++vtkKCellIndex) {
			for (uint32_t vtkJCellIndex = 0; vtkJCellIndex < jCellCount; ++vtkJCellIndex) {
				for (uint32_t vtkICellIndex = 0; vtkICellIndex < iCellCount; ++vtkICellIndex) {
					if (elementIndices[indice] == cellIndex) {
						vtkSmartPointer<vtkHexahedron> hex = vtkSmartPointer<vtkHexahedron>::New();

						for (uint8_t cornerId = 0; cornerId < 8; ++cornerId) {
							hex->GetPointIds()->SetId(cornerId,
								ijkGridRepresentation->getXyzPointIndexFromCellCorner(vtkICellIndex, vtkJCellIndex, vtkKCellIndex, correspondingResqmlCornerId[cornerId]));
						}
						vtkOutput->InsertNextCell(hex->GetCellType(), hex->GetPointIds());
						indice++;
					}
					++cellIndex;
				}
			}
		}
	}
	else {
		const uint64_t cellCount = ijkGridRepresentation->getCellCount();
		std::unique_ptr<bool[]> enabledCells(new bool[cellCount]);
		if (ijkGridRepresentation->hasEnabledCellInformation())	{
			ijkGridRepresentation->getEnabledCells(enabledCells.get());
		}
		else {
			std::fill_n(enabledCells.get(), cellCount, true);
		}

		const uint64_t translatePoint = ijkGridRepresentation->getXyzPointCountOfKInterface() * initKIndex;

		vtkOutput->Allocate(iCellCount*jCellCount);
		for (uint32_t vtkKCellIndex = initKIndex; vtkKCellIndex < maxKIndex; ++vtkKCellIndex){
			for (uint32_t vtkJCellIndex = 0; vtkJCellIndex < jCellCount; ++vtkJCellIndex){
				for (uint32_t vtkICellIndex = 0; vtkICellIndex < iCellCount; ++vtkICellIndex){
					if (enabledCells[cellIndex]){
						vtkSmartPointer<vtkHexahedron> hex = vtkSmartPointer<vtkHexahedron>::New();

						for (uint8_t cornerId = 0; cornerId < 8; ++cornerId) {
							uint64_t pointIndex = ijkGridRepresentation->getXyzPointIndexFromCellCorner(vtkICellIndex, vtkJCellIndex, vtkKCellIndex, correspondingResqmlCornerId[cornerId]);
							hex->GetPointIds()->SetId(cornerId, pointIndex - translatePoint);
						}

						vtkOutput->InsertNextCell(hex->GetCellType(), hex->GetPointIds());
					}
					else {
						vtkSmartPointer<vtkEmptyCell> emptyCell = vtkSmartPointer<vtkEmptyCell>::New();
						vtkOutput->InsertNextCell(emptyCell->GetCellType(), emptyCell->GetPointIds());
					}
					++cellIndex;
				}
			}
		}
	}
	ijkGridRepresentation->unloadSplitInformation();
}
