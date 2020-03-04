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

#include <sstream>

// include VTK library
#include <vtkHexahedron.h>
#include <vtkCellData.h>
#include <vtkEmptyCell.h>
#include <vtkDataArray.h>

// include F2i-consulting Energistics Standards API
#include <fesapi/resqml2_0_1/AbstractIjkGridRepresentation.h>
#include <fesapi/resqml2/SubRepresentation.h>

// include F2i-consulting Energistics Standards ParaView Plugin
#include "VtkProperty.h"

//----------------------------------------------------------------------------
VtkIjkGridRepresentation::VtkIjkGridRepresentation(const std::string & fileName, const std::string & name, const std::string & uuid, const std::string & uuidParent, COMMON_NS::DataObjectRepository *pckEPCRep, COMMON_NS::DataObjectRepository *pckEPCSubRep, int idProc, int maxProc) :
	VtkResqml2UnstructuredGrid(fileName, name, uuid, uuidParent, pckEPCRep, pckEPCSubRep, idProc, maxProc), lastProperty(""),
	iCellCount(0), jCellCount(0), kCellCount(0), initKIndex(0), maxKIndex(0), isHyperslabed(false)
{
}

VtkIjkGridRepresentation::~VtkIjkGridRepresentation()
{
}

//----------------------------------------------------------------------------
vtkSmartPointer<vtkPoints> VtkIjkGridRepresentation::createpoint()
{
	if (!vtkPointsIsCreated()) {
		points = vtkSmartPointer<vtkPoints>::New();
		common::AbstractObject* obj = nullptr;
		resqml2_0_1::AbstractIjkGridRepresentation* ijkGridRepresentation = nullptr;
		if (subRepresentation){
			obj = epcPackageRepresentation->getDataObjectByUuid(getParent());
		}
		else{
			obj = epcPackageRepresentation->getDataObjectByUuid(getUuid());
			//			obj = epcPackageRepresentation->getResqmlAbstractObjectByUuid(getUuid());
		}

		if (obj != nullptr && (obj->getXmlTag() == resqml2_0_1::AbstractIjkGridRepresentation::XML_TAG || obj->getXmlTag() == resqml2_0_1::AbstractIjkGridRepresentation::XML_TAG_TRUNCATED)) {
			ijkGridRepresentation = static_cast<resqml2_0_1::AbstractIjkGridRepresentation*>(obj);

		}

		iCellCount = ijkGridRepresentation->getICellCount();
		jCellCount = ijkGridRepresentation->getJCellCount();
		kCellCount = ijkGridRepresentation->getKCellCount();

		const ULONG64 kInterfaceNodeCount = ijkGridRepresentation->getXyzPointCountOfKInterfaceOfPatch(0);

		checkHyperslabingCapacity(ijkGridRepresentation);
		if (isHyperslabed && !ijkGridRepresentation->isNodeGeometryCompressed()) {
			auto optim = kCellCount / getMaxProc();
			initKIndex = getIdProc() * optim;
			maxKIndex = initKIndex + optim;

			if (getIdProc() == getMaxProc()-1)
				maxKIndex = kCellCount;

			auto allXyzPoints = new double[kInterfaceNodeCount * 3];

			for (auto kInterface = initKIndex; kInterface <= maxKIndex; ++kInterface) {

				ijkGridRepresentation->getXyzPointsOfKInterfaceOfPatch(kInterface, 0, allXyzPoints);
				double zIndice = 1;

				if (ijkGridRepresentation->getLocalCrs(0)->isDepthOriented())
					zIndice = -1;

				for (ULONG64 nodeIndex = 0; nodeIndex < kInterfaceNodeCount * 3; nodeIndex += 3) {
					points->InsertNextPoint(allXyzPoints[nodeIndex], allXyzPoints[nodeIndex + 1], allXyzPoints[nodeIndex + 2] * zIndice);
				}


			}
			delete[] allXyzPoints;
			std::string s = "ijkGrid idProc-maxProc : " + std::to_string(getIdProc()) + "-" + std::to_string(getMaxProc()) + " Points " + std::to_string(kInterfaceNodeCount*(maxKIndex-initKIndex)) +"\n";
			char const * pchar = s.c_str();
			vtkOutputWindowDisplayDebugText(pchar);

		}
		else {
			initKIndex = 0;
			maxKIndex = kCellCount;

			// POINT
			const auto nodeCount = ijkGridRepresentation->getXyzPointCountOfAllPatches();
			auto allXyzPoints = new double[nodeCount * 3];
			ijkGridRepresentation->getXyzPointsOfAllPatchesInGlobalCrs(allXyzPoints);
			createVtkPoints(nodeCount, allXyzPoints, ijkGridRepresentation->getLocalCrs(0));

			delete[] allXyzPoints;
		}
	}
	return points;
}

//----------------------------------------------------------------------------
void VtkIjkGridRepresentation::createOutput(const std::string & uuid)
{
	createpoint(); // => POINTS
	if (!vtkOutput) {	// => REPRESENTATION
		if (subRepresentation){
			// SubRep
			createWithPoints(points, epcPackageSubRepresentation->getDataObjectByUuid(getUuid()));
		}
		else {
			// Ijk Grid
			createWithPoints(points, epcPackageRepresentation->getDataObjectByUuid(getUuid()));
		}
	}

	if (uuid != getUuid()) { // => PROPERTY UUID
		if (subRepresentation){
			RESQML2_NS::SubRepresentation* subRep = epcPackageSubRepresentation->getDataObjectByUuid<RESQML2_NS::SubRepresentation>(getUuid());
			auto cellCount = subRep->getElementCountOfPatch(0);
			//#ifdef PARAVIEW_USE_MPI
			if (isHyperslabed) {
				addProperty(uuid, uuidToVtkProperty[uuid]->loadValuesPropertySet(subRep->getValuesPropertySet(), iCellCount*jCellCount*(maxKIndex - initKIndex), 0, iCellCount, jCellCount, maxKIndex - initKIndex, initKIndex));
			}
			else {
				//#endif
				addProperty(uuid, uuidToVtkProperty[uuid]->loadValuesPropertySet(subRep->getValuesPropertySet(), cellCount, 0));
				//#ifdef PARAVIEW_USE_MPI
			}
			//#endif
		}
		else {
			RESQML2_0_1_NS::AbstractIjkGridRepresentation* ijkGrid = epcPackageRepresentation->getDataObjectByUuid<RESQML2_0_1_NS::AbstractIjkGridRepresentation>(getUuid());
			auto cellCount = iCellCount*jCellCount*kCellCount;
			//#ifdef PARAVIEW_USE_MPI
			if (isHyperslabed) {
				addProperty(uuid, uuidToVtkProperty[uuid]->loadValuesPropertySet(ijkGrid->getValuesPropertySet(), iCellCount*jCellCount*(maxKIndex - initKIndex), 0, iCellCount, jCellCount, maxKIndex - initKIndex, initKIndex));
			}
			else {
				//#endif
				addProperty(uuid, uuidToVtkProperty[uuid]->loadValuesPropertySet(ijkGrid->getValuesPropertySet(), cellCount, 0));
				//#ifdef PARAVIEW_USE_MPI
			}
			//#endif
		}
	}
}

//----------------------------------------------------------------------------
void VtkIjkGridRepresentation::checkHyperslabingCapacity(resqml2_0_1::AbstractIjkGridRepresentation* ijkGridRepresentation)
{
	double* allXyzPoints = nullptr;
	try {
		const auto kInterfaceNodeCount = ijkGridRepresentation->getXyzPointCountOfKInterfaceOfPatch(0);
		allXyzPoints = new double[kInterfaceNodeCount * 3];
		ijkGridRepresentation->getXyzPointsOfKInterfaceOfPatch(0, 0, allXyzPoints);
		isHyperslabed = true;
		delete[] allXyzPoints;
	}
	catch (const std::exception&) {
		isHyperslabed = false;
		if (allXyzPoints != nullptr) {
			delete[] allXyzPoints;
		}
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
long VtkIjkGridRepresentation::getAttachmentPropertyCount(const std::string & uuid, VtkEpcCommon::FesppAttachmentProperty propertyUnit)
{
	if (propertyUnit == VtkEpcCommon::CELLS) {
		common::AbstractObject* obj;
		if (subRepresentation) {
			// SubRep
			obj = epcPackageSubRepresentation->getDataObjectByUuid(getUuid());
			auto subRepresentation1 = static_cast<resqml2::SubRepresentation*>(obj);
			return isHyperslabed ? iCellCount*jCellCount*(maxKIndex - initKIndex) : subRepresentation1->getElementCountOfPatch(0);
		}
		else {
			// Ijk Grid
			obj = epcPackageRepresentation->getDataObjectByUuid(getUuid());
			auto ijkGridRepresentation = static_cast<resqml2_0_1::AbstractIjkGridRepresentation*>(obj);
			return ijkGridRepresentation->getCellCount();
		}
	}
	return 0;

}

//----------------------------------------------------------------------------
int VtkIjkGridRepresentation::getICellCount(const std::string & uuid) const
{
	return subRepresentation
		? iCellCount
		: epcPackageRepresentation->getDataObjectByUuid<RESQML2_0_1_NS::AbstractIjkGridRepresentation>(getUuid())->getICellCount();
}

//----------------------------------------------------------------------------
int VtkIjkGridRepresentation::getJCellCount(const std::string & uuid) const
{
	return subRepresentation
		? jCellCount
		: epcPackageRepresentation->getDataObjectByUuid<RESQML2_0_1_NS::AbstractIjkGridRepresentation>(getUuid())->getJCellCount();
}

//----------------------------------------------------------------------------
int VtkIjkGridRepresentation::getKCellCount(const std::string & uuid) const
{
	return subRepresentation
		? kCellCount
		: epcPackageRepresentation->getDataObjectByUuid<RESQML2_0_1_NS::AbstractIjkGridRepresentation>(getUuid())->getKCellCount();
}

//----------------------------------------------------------------------------
int VtkIjkGridRepresentation::getInitKIndex(const std::string & uuid) const
{
	return initKIndex;
}

//----------------------------------------------------------------------------
void VtkIjkGridRepresentation::createWithPoints(const vtkSmartPointer<vtkPoints> & pointsRepresentation, common::AbstractObject* obj)
{
	vtkOutput = vtkSmartPointer<vtkUnstructuredGrid>::New();
	vtkOutput->SetPoints(pointsRepresentation);

	if (obj != nullptr && (obj->getXmlTag() == "SubRepresentation")) {
		resqml2::SubRepresentation* subRepresentation1 = static_cast<resqml2::SubRepresentation*>(obj);
		resqml2_0_1::AbstractIjkGridRepresentation* ijkGridRepresentation = static_cast<resqml2_0_1::AbstractIjkGridRepresentation*>(epcPackageRepresentation->getDataObjectByUuid(subRepresentation1->getSupportingRepresentationUuid(0)));

		auto elementCountOfPatch = subRepresentation1->getElementCountOfPatch(0);
		auto elementIndices = new ULONG64[elementCountOfPatch];
		subRepresentation1->getElementIndicesOfPatch(0, 0, elementIndices);

		iCellCount = ijkGridRepresentation->getICellCount();
		jCellCount = ijkGridRepresentation->getJCellCount();
		kCellCount = ijkGridRepresentation->getKCellCount();

		initKIndex = 0;
		maxKIndex = kCellCount;

		std::string s = "ijkGrid- SubRep idProc-maxProc : " + std::to_string(getIdProc()) + "-" + std::to_string(getMaxProc()) + " Kcell " + std::to_string(initKIndex) + " to " + std::to_string(maxKIndex) +"\n";
		char const * pchar = s.c_str();
		vtkOutputWindowDisplayDebugText(pchar);

		ijkGridRepresentation->loadSplitInformation();

		auto indice = 0;
		unsigned int cellIndex = 0;

		for (auto vtkKCellIndex = initKIndex; vtkKCellIndex < maxKIndex; ++vtkKCellIndex) {
			for (auto vtkJCellIndex = 0; vtkJCellIndex < jCellCount; ++vtkJCellIndex) {
				for (auto vtkICellIndex = 0; vtkICellIndex < iCellCount; ++vtkICellIndex) {
					if (elementIndices[indice] == cellIndex) {
						vtkSmartPointer<vtkHexahedron> hex = vtkSmartPointer<vtkHexahedron>::New();


						for (unsigned int id=0; id < 8; ++id) {
							hex->GetPointIds()->SetId(id, ijkGridRepresentation->getXyzPointIndexFromCellCorner(vtkICellIndex, vtkJCellIndex, vtkKCellIndex, id));
						}
						vtkOutput->InsertNextCell(hex->GetCellType(), hex->GetPointIds());
						indice++;
					}
					++cellIndex;
				}
			}
		}
		ijkGridRepresentation->unloadSplitInformation();

		delete[] elementIndices;
	}
	else {
		if (obj != nullptr && (obj->getXmlTag() == resqml2_0_1::AbstractIjkGridRepresentation::XML_TAG || obj->getXmlTag() == resqml2_0_1::AbstractIjkGridRepresentation::XML_TAG_TRUNCATED)){
			resqml2_0_1::AbstractIjkGridRepresentation* ijkGridRepresentation = static_cast<resqml2_0_1::AbstractIjkGridRepresentation*>(obj);

			std::string s = "ijkGrid idProc-maxProc : " + std::to_string(getIdProc()) + "-" + std::to_string(getMaxProc()) + " Kcell " + std::to_string(initKIndex) + " to " + std::to_string(maxKIndex) +"\n";
			char const * pchar = s.c_str();
			vtkOutputWindowDisplayDebugText(pchar);

			ijkGridRepresentation->loadSplitInformation();

			const ULONG64 cellCount = ijkGridRepresentation->getCellCount();
			auto enabledCells = new bool[cellCount];
			if (ijkGridRepresentation->hasEnabledCellInformation())	{
				ijkGridRepresentation->getEnabledCells(enabledCells);
			}
			else {
				for (ULONG64 j = 0; j < cellCount; ++j)	{
					enabledCells[j] = true;
				}
			}

			unsigned int cellIndex = 0;

			const ULONG64 translatePoint = ijkGridRepresentation->getXyzPointCountOfKInterfaceOfPatch(0)*initKIndex;

			for (auto vtkKCellIndex = initKIndex; vtkKCellIndex < maxKIndex; ++vtkKCellIndex){
				for (auto vtkJCellIndex = 0; vtkJCellIndex < jCellCount; ++vtkJCellIndex){
					for (auto vtkICellIndex = 0; vtkICellIndex < iCellCount; ++vtkICellIndex){
						if (enabledCells[cellIndex]){
							vtkSmartPointer<vtkHexahedron> hex = vtkSmartPointer<vtkHexahedron>::New();

							for (unsigned int id=0; id < 8; ++id) {
								hex->GetPointIds()->SetId(id, ijkGridRepresentation->getXyzPointIndexFromCellCorner(vtkICellIndex, vtkJCellIndex, vtkKCellIndex, id) - translatePoint);
							}

							vtkOutput->InsertNextCell(hex->GetCellType(), hex->GetPointIds());
						}
						else{
							vtkSmartPointer<vtkEmptyCell> emptyCell = vtkSmartPointer<vtkEmptyCell>::New();
							vtkOutput->InsertNextCell(emptyCell->GetCellType(), emptyCell->GetPointIds());
						}
						++cellIndex;
					}
				}
			}
			delete[] enabledCells;
			ijkGridRepresentation->unloadSplitInformation();
		}
	}
}
