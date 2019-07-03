/*-----------------------------------------------------------------------
Copyright F2I-CONSULTING, (2014)

cedric.robert@f2i-consulting.com

This software is a computer program whose purpose is to display data formatted using Energistics standards.

This software is governed by the CeCILL license under French law and
abiding by the rules of distribution of free software.  You can  use,
modify and/ or redistribute the software under the terms of the CeCILL
license as circulated by CEA, CNRS and INRIA at the following URL
"http://www.cecill.info".

As a counterpart to the access to the source code and  rights to copy,
modify and redistribute granted by the license, users are provided only
with a limited warranty  and the software's author,  the holder of the
economic rights,  and the successive licensors  have only  limited
liability.

In this respect, the user's attention is drawn to the risks associated
with loading,  using,  modifying and/or developing or reproducing the
software by the user in light of its specific status of free software,
that may mean  that it is complicated to manipulate,  and  that  also
therefore means  that it is reserved for developers  and  experienced
professionals having in-depth computer knowledge. Users are therefore
encouraged to load and test the software's suitability as regards their
requirements in conditions enabling the security of their systems and/or
data to be ensured and,  more generally, to use and operate it in the
same conditions as regards security.

The fact that you are presently reading this means that you have had
knowledge of the CeCILL license and that you accept its terms.
-----------------------------------------------------------------------*/
#include "VtkIjkGridRepresentation.h"

// include VTK library
#include <vtkHexahedron.h>
#include <vtkCellData.h>
#include <vtkEmptyCell.h>
#include <vtkDataArray.h>

// include F2i-consulting Energistics Standards API
#include <resqml2_0_1/AbstractIjkGridRepresentation.h>
#include <resqml2/SubRepresentation.h>

// include F2i-consulting Energistics Standards ParaView Plugin
#include "VtkProperty.h"

#include <sstream>

//----------------------------------------------------------------------------
VtkIjkGridRepresentation::VtkIjkGridRepresentation(const std::string & fileName, const std::string & name, const std::string & uuid, const std::string & uuidParent, common::EpcDocument *pckEPCRep, common::EpcDocument *pckEPCSubRep, const int & idProc, const int & maxProc) :
VtkResqml2UnstructuredGrid(fileName, name, uuid, uuidParent, pckEPCRep, pckEPCSubRep, idProc, maxProc)
{
	isHyperslabed = false;

	iCellCount = 0;
	jCellCount = 0;
	kCellCount = 0;
	initKIndex = 0;
	maxKIndex = 0;
}

VtkIjkGridRepresentation::~VtkIjkGridRepresentation()
{
	lastProperty = "";

	iCellCount = 0;
	jCellCount = 0;
	kCellCount = 0;
	initKIndex = 0;
	maxKIndex = 0;
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


#ifdef PARAVIEW_USE_MPI
		const ULONG64 kInterfaceNodeCount = ijkGridRepresentation->getXyzPointCountOfKInterfaceOfPatch(0);

		checkHyperslabingCapacity(ijkGridRepresentation);
		if (isHyperslabed) {
			initKIndex = getIdProc() * (kCellCount / getMaxProc());
			maxKIndex = (getIdProc()+1) * (kCellCount / getMaxProc());

			if (getIdProc() == getMaxProc()-1)
				maxKIndex = kCellCount;

			auto allXyzPoints = new double[kInterfaceNodeCount * 3];

			for (auto kInterface = initKIndex; kInterface <= maxKIndex; ++kInterface) {

				ijkGridRepresentation->getXyzPointsOfKInterfaceOfPatch(kInterface, 0, allXyzPoints);
				double zIndice = 1;

				if (ijkGridRepresentation->getLocalCrs()->isDepthOriented())
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
#endif
			initKIndex = 0;
			maxKIndex = kCellCount;

			// POINT
			const auto nodeCount = ijkGridRepresentation->getXyzPointCountOfAllPatches();
			auto allXyzPoints = new double[nodeCount * 3];
			ijkGridRepresentation->getXyzPointsOfAllPatchesInGlobalCrs(allXyzPoints);
			createVtkPoints(nodeCount, allXyzPoints, ijkGridRepresentation->getLocalCrs());

			delete[] allXyzPoints;
#ifdef PARAVIEW_USE_MPI
		}
#endif

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
		}else{
			// Ijk Grid
			createWithPoints(points, epcPackageRepresentation->getDataObjectByUuid(getUuid()));
		}
	}

	if (uuid != getUuid()){ // => PROPERTY UUID
		if (subRepresentation){
			auto objRepProp = epcPackageSubRepresentation->getDataObjectByUuid(getUuid());
			auto property = static_cast<resqml2::SubRepresentation*>(objRepProp);
			auto cellCount = property->getElementCountOfPatch(0);
			//#ifdef PARAVIEW_USE_MPI
			if (isHyperslabed) {
				addProperty(uuid, uuidToVtkProperty[uuid]->loadValuesPropertySet(property->getValuesPropertySet(), iCellCount*jCellCount*(maxKIndex - initKIndex), 0, iCellCount, jCellCount, maxKIndex - initKIndex, initKIndex));
			}
			else {
				//#endif
				addProperty(uuid, uuidToVtkProperty[uuid]->loadValuesPropertySet(property->getValuesPropertySet(), cellCount, 0));
				//#ifdef PARAVIEW_USE_MPI
			}
			//#endif
		}
		else {
			auto objRepProp = epcPackageRepresentation->getDataObjectByUuid(getUuid());
			auto property = static_cast<resqml2_0_1::AbstractIjkGridRepresentation*>(objRepProp);
			auto cellCount = iCellCount*jCellCount*kCellCount;
			//#ifdef PARAVIEW_USE_MPI
			if (isHyperslabed) {
				addProperty(uuid, uuidToVtkProperty[uuid]->loadValuesPropertySet(property->getValuesPropertySet(), iCellCount*jCellCount*(maxKIndex - initKIndex), 0, iCellCount, jCellCount, maxKIndex - initKIndex, initKIndex));
			}
			else {
				//#endif
				addProperty(uuid, uuidToVtkProperty[uuid]->loadValuesPropertySet(property->getValuesPropertySet(), cellCount, 0));
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
	try{
		const auto kInterfaceNodeCount = ijkGridRepresentation->getXyzPointCountOfKInterfaceOfPatch(0);
		allXyzPoints = new double[kInterfaceNodeCount * 3];
		ijkGridRepresentation->getXyzPointsOfKInterfaceOfPatch(0, 0, allXyzPoints);
		isHyperslabed = true;
		delete[] allXyzPoints;
	}
	catch (const std::exception & )
	{
		isHyperslabed = false;
		if (allXyzPoints != nullptr)
			delete[] allXyzPoints;
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
long VtkIjkGridRepresentation::getAttachmentPropertyCount(const std::string & uuid, const VtkEpcCommon::FesppAttachmentProperty propertyUnit)
{
	if (propertyUnit == VtkEpcCommon::CELLS) {
		common::AbstractObject* obj;
		if (subRepresentation) {
			// SubRep
			obj = epcPackageSubRepresentation->getDataObjectByUuid(getUuid());
			auto subRepresentation1 = static_cast<resqml2::SubRepresentation*>(obj);
			return isHyperslabed ? iCellCount*jCellCount*(maxKIndex - initKIndex) : subRepresentation1->getElementCountOfPatch(0);
		}else {
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
	long result = 0;

	if (subRepresentation) {
		// SubRep
		result = iCellCount;
	}else {
		common::AbstractObject* obj;
		// Ijk Grid
		obj = epcPackageRepresentation->getDataObjectByUuid(getUuid());
		auto ijkGridRepresentation = static_cast<resqml2_0_1::AbstractIjkGridRepresentation*>(obj);
		result = ijkGridRepresentation->getICellCount();
	}
	return result;
}

//----------------------------------------------------------------------------
int VtkIjkGridRepresentation::getJCellCount(const std::string & uuid) const
{
	long result = 0;

	if (subRepresentation)	{
		// SubRep
		result = jCellCount;
	}else	{
		common::AbstractObject* obj;
		// Ijk Grid
		obj = epcPackageRepresentation->getDataObjectByUuid(getUuid());
		auto ijkGridRepresentation = static_cast<resqml2_0_1::AbstractIjkGridRepresentation*>(obj);
		result = ijkGridRepresentation->getJCellCount();
	}
	return result;
}

//----------------------------------------------------------------------------
int VtkIjkGridRepresentation::getKCellCount(const std::string & uuid) const
{
	long result = 0;

	if (subRepresentation)	{
		// SubRep
		result = kCellCount;
	}else{
		common::AbstractObject* obj;
		// Ijk Grid
		obj = epcPackageRepresentation->getDataObjectByUuid(getUuid());
		auto ijkGridRepresentation = static_cast<resqml2_0_1::AbstractIjkGridRepresentation*>(obj);
		result = ijkGridRepresentation->getKCellCount();
	}
	return result;
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
