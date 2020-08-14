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
#include "VtkIjkGridRepresentation.h"

#include <array>

// include VTK library
#include <vtkHexahedron.h>
#include <vtkCellData.h>
#include <vtkEmptyCell.h>
#include <vtkDataArray.h>

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
vtkSmartPointer<vtkPoints> VtkIjkGridRepresentation::createpoint()
{
	if (points == nullptr) {
		points = vtkSmartPointer<vtkPoints>::New();
		COMMON_NS::AbstractObject* obj = subRepresentation
			? epcPackageRepresentation->getDataObjectByUuid(getParent())
			: epcPackageRepresentation->getDataObjectByUuid(getUuid());
		if (obj != nullptr && dynamic_cast<RESQML2_NS::AbstractIjkGridRepresentation*>(obj) == nullptr) {
			throw std::logic_error("The object is not an IjkGridRepresentation");
		}

		RESQML2_NS::AbstractIjkGridRepresentation* ijkGridRepresentation = static_cast<RESQML2_NS::AbstractIjkGridRepresentation*>(obj);
		iCellCount = ijkGridRepresentation->getICellCount();
		jCellCount = ijkGridRepresentation->getJCellCount();
		kCellCount = ijkGridRepresentation->getKCellCount();

		const ULONG64 kInterfaceNodeCount = ijkGridRepresentation->getXyzPointCountOfKInterface();

		checkHyperslabingCapacity(ijkGridRepresentation);
		if (isHyperslabed && !ijkGridRepresentation->isNodeGeometryCompressed()) {
			auto optim = kCellCount / getMaxProc();
			initKIndex = getIdProc() * optim;
			maxKIndex = initKIndex + optim;

			if (getIdProc() == getMaxProc() - 1) {
				maxKIndex = kCellCount;
			}

			std::unique_ptr<double[]> allXyzPoints(new double[kInterfaceNodeCount * 3]);

			for (auto kInterface = initKIndex; kInterface <= maxKIndex; ++kInterface) {
				ijkGridRepresentation->getXyzPointsOfKInterface(kInterface, allXyzPoints.get());
				const double zIndice = ijkGridRepresentation->getLocalCrs(0)->isDepthOriented() ? -1 : 1;

				for (ULONG64 nodeIndex = 0; nodeIndex < kInterfaceNodeCount * 3; nodeIndex += 3) {
					points->InsertNextPoint(allXyzPoints[nodeIndex], allXyzPoints[nodeIndex + 1], allXyzPoints[nodeIndex + 2] * zIndice);
				}
			}

			const std::string s = "ijkGrid idProc-maxProc : " + std::to_string(getIdProc()) + "-" + std::to_string(getMaxProc()) + " Points " + std::to_string(kInterfaceNodeCount*(maxKIndex-initKIndex)) + "\n";
			vtkOutputWindowDisplayDebugText(s.c_str());

		}
		else {
			initKIndex = 0;
			maxKIndex = kCellCount;

			// POINT
			const auto nodeCount = ijkGridRepresentation->getXyzPointCountOfAllPatches();
			std::unique_ptr<double[]> allXyzPoints(new double[nodeCount * 3]);
			ijkGridRepresentation->getXyzPointsOfAllPatchesInGlobalCrs(allXyzPoints.get());
			createVtkPoints(nodeCount, allXyzPoints.get(), ijkGridRepresentation->getLocalCrs(0));
		}
	}
	return points;
}

//----------------------------------------------------------------------------
void VtkIjkGridRepresentation::createOutput(const std::string & uuid)
{
	createpoint(); // => POINTS
	if (!vtkOutput) {	// => REPRESENTATION
		auto dataObj = subRepresentation
			? epcPackageSubRepresentation->getDataObjectByUuid(getUuid())
			: epcPackageRepresentation->getDataObjectByUuid(getUuid());
		createWithPoints(points, dataObj);

	}

	if (uuid != getUuid()) { // => PROPERTY UUID
		if (subRepresentation){
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
void VtkIjkGridRepresentation::createWithPoints(vtkSmartPointer<vtkPoints> pointsRepresentation, COMMON_NS::AbstractObject* obj)
{
	// Check if we are rendering an entire RESQML IJK grid or a RESQML subrepresentation of a RESQML IJK grid.
	RESQML2_NS::SubRepresentation const * subRepresentation = dynamic_cast<RESQML2_NS::SubRepresentation*>(obj);
	RESQML2_NS::AbstractIjkGridRepresentation* ijkGridRepresentation = subRepresentation != nullptr
		? epcPackageRepresentation->getDataObjectByUuid<RESQML2_NS::AbstractIjkGridRepresentation>(subRepresentation->getSupportingRepresentationDor(0).getUuid())
		: dynamic_cast<RESQML2_NS::AbstractIjkGridRepresentation*>(obj);

	if (ijkGridRepresentation == nullptr) {
		vtkOutputWindowDisplayDebugText("The input data is neither a subrepresentation of an IJK grid nor an entire IJK grid.\n");
		return;
	}

	// Create and set the list of points of the vtkUnstructuredGrid
	vtkOutput = vtkSmartPointer<vtkUnstructuredGrid>::New();
	vtkOutput->SetPoints(pointsRepresentation);

	// Define hexahedron node ordering according to Paraview convention : https://lorensen.github.io/VTKExamples/site/VTKBook/05Chapter5/#Figure%205-3
	std::array<unsigned int, 8> correspondingResqmlCornerId = { 0, 1, 2, 3, 4, 5, 6, 7 };
	if (ijkGridRepresentation->getLocalCrs(0)->isDepthOriented() == ijkGridRepresentation->isRightHanded()) {
		correspondingResqmlCornerId = { 4, 5, 6, 7, 0, 1, 2, 3 };
	}

	// Create and set the list of hexahedra of the vtkUnstructuredGrid based on the list of points already set
	ijkGridRepresentation->loadSplitInformation();
	unsigned int cellIndex = 0;
	if (subRepresentation != nullptr) {
		auto elementCountOfPatch = subRepresentation->getElementCountOfPatch(0);
		std::unique_ptr<ULONG64[]> elementIndices(new ULONG64[elementCountOfPatch]);
		subRepresentation->getElementIndicesOfPatch(0, 0, elementIndices.get());

		iCellCount = ijkGridRepresentation->getICellCount();
		jCellCount = ijkGridRepresentation->getJCellCount();
		kCellCount = ijkGridRepresentation->getKCellCount();

		initKIndex = 0;
		maxKIndex = kCellCount;

		vtkOutputWindowDisplayDebugText(("ijkGrid- SubRep idProc-maxProc : " + std::to_string(getIdProc()) + "-" + std::to_string(getMaxProc()) + " Kcell " + std::to_string(initKIndex) + " to " + std::to_string(maxKIndex) + "\n").c_str());

		size_t indice = 0;

		for (auto vtkKCellIndex = initKIndex; vtkKCellIndex < maxKIndex; ++vtkKCellIndex) {
			for (auto vtkJCellIndex = 0; vtkJCellIndex < jCellCount; ++vtkJCellIndex) {
				for (auto vtkICellIndex = 0; vtkICellIndex < iCellCount; ++vtkICellIndex) {
					if (elementIndices[indice] == cellIndex) {
						vtkSmartPointer<vtkHexahedron> hex = vtkSmartPointer<vtkHexahedron>::New();

						for (unsigned int cornerId = 0; cornerId < 8; ++cornerId) {
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
		vtkOutputWindowDisplayDebugText(("ijkGrid idProc-maxProc : " + std::to_string(getIdProc()) + "-" + std::to_string(getMaxProc()) + " Kcell " + std::to_string(initKIndex) + " to " + std::to_string(maxKIndex) + "\n").c_str());

		const ULONG64 cellCount = ijkGridRepresentation->getCellCount();
		std::unique_ptr<bool[]> enabledCells(new bool[cellCount]);
		if (ijkGridRepresentation->hasEnabledCellInformation())	{
			ijkGridRepresentation->getEnabledCells(enabledCells.get());
		}
		else {
			std::fill_n(enabledCells.get(), cellCount, true);
		}

		const ULONG64 translatePoint = ijkGridRepresentation->getXyzPointCountOfKInterface() * initKIndex;

		vtkOutput->Allocate(iCellCount*jCellCount*maxKIndex);
		for (auto vtkKCellIndex = initKIndex; vtkKCellIndex < maxKIndex; ++vtkKCellIndex){
			for (auto vtkJCellIndex = 0; vtkJCellIndex < jCellCount; ++vtkJCellIndex){
				for (auto vtkICellIndex = 0; vtkICellIndex < iCellCount; ++vtkICellIndex){
					if (enabledCells[cellIndex]){
						vtkSmartPointer<vtkHexahedron> hex = vtkSmartPointer<vtkHexahedron>::New();

						for (unsigned int cornerId = 0; cornerId < 8; ++cornerId) {
							hex->GetPointIds()->SetId(cornerId,
								ijkGridRepresentation->getXyzPointIndexFromCellCorner(vtkICellIndex, vtkJCellIndex, vtkKCellIndex, correspondingResqmlCornerId[cornerId]) - translatePoint);
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
