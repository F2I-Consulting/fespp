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
#include "log.h"

#include <sstream>

const std::string loggClass = "CLASS=VtkIjkGridRepresentation ";

//----------------------------------------------------------------------------
VtkIjkGridRepresentation::VtkIjkGridRepresentation(const std::string & fileName, const std::string & name, const std::string & uuid, const std::string & uuidParent, common::EpcDocument *pckEPCRep, common::EpcDocument *pckEPCSubRep, const int & idProc, const int & maxProc) :
VtkResqml2UnstructuredGrid(fileName, name, uuid, uuidParent, pckEPCRep, pckEPCSubRep, idProc, maxProc)
{
    L_(linfo) << loggClass << "FUNCTION=VtkIjkGridRepresentation " << "STATUS=IN " << "LEVEL=1 ";

	isHyperslabed = false;

	iCellCount = 0;
	jCellCount = 0;
	kCellCount = 0;
	initKIndex = 0;
	maxKIndex = 0;

    L_(linfo) << loggClass << "FUNCTION=VtkIjkGridRepresentation " << "STATUS=OUT "<< "LEVEL=1";
}

VtkIjkGridRepresentation::~VtkIjkGridRepresentation()
{
    L_(linfo) << loggClass << "FUNCTION=~VtkIjkGridRepresentation " << "STATUS=IN "<< "LEVEL=1";

	lastProperty = "";

	iCellCount = 0;
	jCellCount = 0;
	kCellCount = 0;
	initKIndex = 0;
	maxKIndex = 0;

    L_(linfo) << loggClass << "FUNCTION=~VtkIjkGridRepresentation " << "STATUS=OUT "<< "LEVEL=1 ";



}
vtkSmartPointer<vtkPoints> VtkIjkGridRepresentation::createpoint()
{
    L_(linfo) << loggClass << "FUNCTION=createpoint " << "STATUS=IN "<< "LEVEL=2 ";

	if (!vtkPointsIsCreated())
	{
		L_(linfo) << loggClass << "FUNCTION=~createpoint " << "STATUS=IN "<< "LEVEL=1 " << "API=vtk " << "APIFUNC=vtkSmartPointer<vtkPoints>::New() ";
		points = vtkSmartPointer<vtkPoints>::New();
		L_(linfo) << loggClass << "FUNCTION=~createpoint " << "STATUS=OUT "<< "LEVEL=1 " << "API=vtk " << "APIFUNC=vtkSmartPointer<vtkPoints>::New() ";
		common::AbstractObject* obj = nullptr;
		resqml2_0_1::AbstractIjkGridRepresentation* ijkGridRepresentation = nullptr;
		if (subRepresentation){
			L_(linfo) << loggClass << "FUNCTION=~createpoint " << "STATUS=IN "<< "LEVEL=1 " << "API=fesapi " << "APIFUNC=getDataObjectByUuid ";
			obj = epcPackageRepresentation->getDataObjectByUuid(getParent());
			L_(linfo) << loggClass << "FUNCTION=~createpoint " << "STATUS=OUT "<< "LEVEL=1 " << "API=fesapi " << "APIFUNC=getDataObjectByUuid ";
		}
		else{
			L_(linfo) << loggClass << "FUNCTION=~createpoint " << "STATUS=IN "<< "LEVEL=1 " << "API=fesapi " << "APIFUNC=getDataObjectByUuid ";
			obj = epcPackageRepresentation->getDataObjectByUuid(getUuid());
			L_(linfo) << loggClass << "FUNCTION=~createpoint " << "STATUS=OUT "<< "LEVEL=1 " << "API=fesapi " << "APIFUNC=getXmlTag() ";
//			obj = epcPackageRepresentation->getResqmlAbstractObjectByUuid(getUuid());
		}

		if (obj != nullptr && (obj->getXmlTag() == resqml2_0_1::AbstractIjkGridRepresentation::XML_TAG || obj->getXmlTag() == resqml2_0_1::AbstractIjkGridRepresentation::XML_TAG_TRUNCATED)) {
			L_(linfo) << loggClass << "FUNCTION=~createpoint " << "STATUS=IN "<< "LEVEL=1 " << "API=fesapi " << "APIFUNC=static_cast<resqml2_0_1::AbstractIjkGridRepresentation*> ";
			ijkGridRepresentation = static_cast<resqml2_0_1::AbstractIjkGridRepresentation*>(obj);
			L_(linfo) << loggClass << "FUNCTION=~createpoint " << "STATUS=OUT "<< "LEVEL=1 " << "API=fesapi " << "APIFUNC=static_cast<resqml2_0_1::AbstractIjkGridRepresentation*> ";

		}

		L_(linfo) << loggClass << "FUNCTION=~createpoint " << "STATUS=IN "<< "LEVEL=1 " << "API=fesapi " << "APIFUNC=getICellCount ";
		iCellCount = ijkGridRepresentation->getICellCount();
		L_(linfo) << loggClass << "FUNCTION=~createpoint " << "STATUS=OUT "<< "LEVEL=1 " << "API=fesapi " << "APIFUNC=getICellCount ";
		L_(linfo) << loggClass << "FUNCTION=~createpoint " << "STATUS=IN "<< "LEVEL=1 " << "API=fesapi " << "APIFUNC=getJCellCount ";
		jCellCount = ijkGridRepresentation->getJCellCount();
		L_(linfo) << loggClass << "FUNCTION=~createpoint " << "STATUS=OUT "<< "LEVEL=1 " << "API=fesapi " << "APIFUNC=getJCellCount ";
		L_(linfo) << loggClass << "FUNCTION=~createpoint " << "STATUS=IN "<< "LEVEL=1 " << "API=fesapi " << "APIFUNC=getKCellCount ";
		kCellCount = ijkGridRepresentation->getKCellCount();
		L_(linfo) << loggClass << "FUNCTION=~createpoint " << "STATUS=OUT "<< "LEVEL=1 " << "API=fesapi " << "APIFUNC=getKCellCount ";


#ifdef PARAVIEW_USE_MPI
		const ULONG64 kInterfaceNodeCount = ijkGridRepresentation->getXyzPointCountOfKInterfaceOfPatch(0);

		checkHyperslabingCapacity(ijkGridRepresentation);
		if (isHyperslabed)
		{
			initKIndex = getIdProc() * (kCellCount / getMaxProc());
			maxKIndex = (getIdProc()+1) * (kCellCount / getMaxProc());

			if (getIdProc() == getMaxProc()-1)
				maxKIndex = kCellCount;

			auto allXyzPoints = new double[kInterfaceNodeCount * 3];

			for (auto kInterface = initKIndex; kInterface <= maxKIndex; ++kInterface)
			{

				ijkGridRepresentation->getXyzPointsOfKInterfaceOfPatch(kInterface, 0, allXyzPoints);
				double zIndice = 1;

				if (ijkGridRepresentation->getLocalCrs()->isDepthOriented())
					zIndice = -1;

				for (ULONG64 nodeIndex = 0; nodeIndex < kInterfaceNodeCount * 3; nodeIndex += 3)
				{
					points->InsertNextPoint(allXyzPoints[nodeIndex], allXyzPoints[nodeIndex + 1], allXyzPoints[nodeIndex + 2] * zIndice);
				}


			}
			delete[] allXyzPoints;
			std::string s = "ijkGrid idProc-maxProc : " + std::to_string(getIdProc()) + "-" + std::to_string(getMaxProc()) + " Points " + std::to_string(kInterfaceNodeCount*(maxKIndex-initKIndex)) +"\n";
			char const * pchar = s.c_str();
			vtkOutputWindowDisplayDebugText(pchar);

		}
		else
		{
#endif
			initKIndex = 0;
			maxKIndex = kCellCount;

			// POINT
			L_(linfo) << loggClass << "FUNCTION=~createpoint " << "STATUS=IN "<< "LEVEL=1 " << "API=fesapi " << "APIFUNC=getXyzPointCountOfAllPatches ";
			const auto nodeCount = ijkGridRepresentation->getXyzPointCountOfAllPatches();
			L_(linfo) << loggClass << "FUNCTION=~createpoint " << "STATUS=OUT "<< "LEVEL=1 " << "API=fesapi " << "APIFUNC=getXyzPointCountOfAllPatches ";
			L_(linfo) << loggClass << "FUNCTION=~createpoint " << "STATUS=IN "<< "LEVEL=1 " << "API=fespp " << "APIFUNC=new double[nodeCount * 3] ";
			auto allXyzPoints = new double[nodeCount * 3];
			L_(linfo) << loggClass << "FUNCTION=~createpoint " << "STATUS=OUT "<< "LEVEL=1 " << "API=fespp " << "APIFUNC=new double[nodeCount * 3] ";
			L_(linfo) << loggClass << "FUNCTION=~createpoint " << "STATUS=IN "<< "LEVEL=1 " << "API=fesapi " << "APIFUNC=getXyzPointsOfAllPatchesInGlobalCrs ";
			ijkGridRepresentation->getXyzPointsOfAllPatchesInGlobalCrs(allXyzPoints);
			L_(linfo) << loggClass << "FUNCTION=~createpoint " << "STATUS=OUT "<< "LEVEL=1 " << "API=fesapi " << "APIFUNC=getXyzPointsOfAllPatchesInGlobalCrs ";
			createVtkPoints(nodeCount, allXyzPoints, ijkGridRepresentation->getLocalCrs());

			L_(linfo) << loggClass << "FUNCTION=~createpoint " << "STATUS=IN "<< "LEVEL=1 " << "API=fespp " << "APIFUNC=delete[] allXyzPoints ";
			delete[] allXyzPoints;
			L_(linfo) << loggClass << "FUNCTION=~createpoint " << "STATUS=OUT "<< "LEVEL=1 " << "API=fespp " << "APIFUNC=delete[] allXyzPoints ";
#ifdef PARAVIEW_USE_MPI
		}
#endif

	}

    L_(linfo) << loggClass << "FUNCTION=createpoint " << "STATUS=OUT "<< "LEVEL=2 ";

	return points;

}

//----------------------------------------------------------------------------
void VtkIjkGridRepresentation::createOutput(const std::string & uuid)
{
    L_(linfo) << loggClass << "FUNCTION=createOutput " << "STATUS=IN "<< "LEVEL=1 ";

	createpoint(); // => POINTS
	if (!vtkOutput)	// => REPRESENTATION
	{
		if (subRepresentation)
		{
			// SubRep
			createWithPoints(points, epcPackageSubRepresentation->getDataObjectByUuid(getUuid()));
		}else
		{
			// Ijk Grid
			createWithPoints(points, epcPackageRepresentation->getDataObjectByUuid(getUuid()));
		}
	}

	if (uuid != getUuid()) // => PROPERTY UUID
	{
		if (subRepresentation) 
		{
			L_(linfo) << loggClass << "FUNCTION=~createpoint " << "STATUS=IN "<< "LEVEL=1 " << "API=fesapi" << "APIFUNC=getDataObjectByUuid ";
			auto objRepProp = epcPackageSubRepresentation->getDataObjectByUuid(getUuid());
			L_(linfo) << loggClass << "FUNCTION=~createpoint " << "STATUS=OUT "<< "LEVEL=1 " << "API=fesapi" << "APIFUNC=getDataObjectByUuid ";
			L_(linfo) << loggClass << "FUNCTION=~createpoint " << "STATUS=IN "<< "LEVEL=1 " << "API=fesapi" << "APIFUNC=static_cast<resqml2::SubRepresentation*> ";
			auto property = static_cast<resqml2::SubRepresentation*>(objRepProp);
			L_(linfo) << loggClass << "FUNCTION=~createpoint " << "STATUS=OUT "<< "LEVEL=1 " << "API=fesapi" << "APIFUNC=static_cast<resqml2::SubRepresentation*> ";
			L_(linfo) << loggClass << "FUNCTION=~createpoint " << "STATUS=IN "<< "LEVEL=1 " << "API=fesapi" << "APIFUNC=getElementCountOfPatch ";
			auto cellCount = property->getElementCountOfPatch(0);
			L_(linfo) << loggClass << "FUNCTION=~createpoint " << "STATUS=OUT "<< "LEVEL=1 " << "API=fesapi" << "APIFUNC=getElementCountOfPatch ";
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
		else
		{
			L_(linfo) << loggClass << "FUNCTION=~createpoint " << "STATUS=IN "<< "LEVEL=1 " << "API=fesapi" << "APIFUNC=getDataObjectByUuid ";
			auto objRepProp = epcPackageRepresentation->getDataObjectByUuid(getUuid());
			L_(linfo) << loggClass << "FUNCTION=~createpoint " << "STATUS=OUT "<< "LEVEL=1 " << "API=fesapi" << "APIFUNC=getDataObjectByUuid ";
			L_(linfo) << loggClass << "FUNCTION=~createpoint " << "STATUS=IN "<< "LEVEL=1 " << "API=fesapi" << "APIFUNC=static_cast<resqml2::AbstractIjkGridRepresentation*> ";
			auto property = static_cast<resqml2_0_1::AbstractIjkGridRepresentation*>(objRepProp);
			L_(linfo) << loggClass << "FUNCTION=~createpoint " << "STATUS=OUT "<< "LEVEL=1 " << "API=fesapi" << "APIFUNC=static_cast<resqml2::AbstractIjkGridRepresentation*> ";
			auto cellCount = iCellCount*jCellCount*kCellCount;
//#ifdef PARAVIEW_USE_MPI
			if (isHyperslabed)
			{
				addProperty(uuid, uuidToVtkProperty[uuid]->loadValuesPropertySet(property->getValuesPropertySet(), iCellCount*jCellCount*(maxKIndex - initKIndex), 0, iCellCount, jCellCount, maxKIndex - initKIndex, initKIndex));
			}
			else
			{
//#endif
				addProperty(uuid, uuidToVtkProperty[uuid]->loadValuesPropertySet(property->getValuesPropertySet(), cellCount, 0));
//#ifdef PARAVIEW_USE_MPI
			}
//#endif
		}
	}

	L_(linfo) << loggClass << "FUNCTION=createOutput " << "STATUS=OUT "<< "LEVEL=1 ";
}

//----------------------------------------------------------------------------
void VtkIjkGridRepresentation::checkHyperslabingCapacity(resqml2_0_1::AbstractIjkGridRepresentation* ijkGridRepresentation)
{
    L_(linfo) << loggClass << "FUNCTION=checkHyperslabingCapacity " << "STATUS=IN "<< "LEVEL=2 ";

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

    L_(linfo) << loggClass << "FUNCTION=checkHyperslabingCapacity " << "STATUS=OUT "<< "LEVEL=2 ";
}

//----------------------------------------------------------------------------
void VtkIjkGridRepresentation::addProperty(const std::string & uuidProperty, vtkDataArray* dataProperty)
{
    L_(linfo) << loggClass << "FUNCTION=addProperty " << "STATUS=IN "<< "LEVEL=2 ";

	vtkOutput->Modified();
	vtkOutput->GetCellData()->AddArray(dataProperty);
	vtkOutput->Modified();
	lastProperty = uuidProperty;

    L_(linfo) << loggClass << "FUNCTION=addProperty " << "STATUS=OUT "<< "LEVEL=2 ";
}

//----------------------------------------------------------------------------
long VtkIjkGridRepresentation::getAttachmentPropertyCount(const std::string & uuid, const VtkEpcCommon::FesppAttachmentProperty propertyUnit)
{
    L_(linfo) << loggClass << "FUNCTION=getAttachmentPropertyCount " << "STATUS=IN "<< "LEVEL=2 ";

	if (propertyUnit == VtkEpcCommon::CELLS)
	{
		common::AbstractObject* obj;
		if (subRepresentation)
		{
			// SubRep
			obj = epcPackageSubRepresentation->getDataObjectByUuid(getUuid());
			auto subRepresentation1 = static_cast<resqml2::SubRepresentation*>(obj);
			return isHyperslabed ? iCellCount*jCellCount*(maxKIndex - initKIndex) : subRepresentation1->getElementCountOfPatch(0);
		}else
		{
			// Ijk Grid
			obj = epcPackageRepresentation->getDataObjectByUuid(getUuid());
			auto ijkGridRepresentation = static_cast<resqml2_0_1::AbstractIjkGridRepresentation*>(obj);
			return ijkGridRepresentation->getCellCount();
		}
	}

    L_(linfo) << loggClass << "FUNCTION=getAttachmentPropertyCount " << "STATUS=OUT "<< "LEVEL=2 ";

	return 0;

}

//----------------------------------------------------------------------------
int VtkIjkGridRepresentation::getICellCount(const std::string & uuid) const
{
    L_(linfo) << loggClass << "FUNCTION=getICellCount " << "STATUS=IN "<< "LEVEL=2 ";

	long result = 0;

	if (subRepresentation)
	{
		// SubRep
		result = iCellCount;
	}else
	{
		common::AbstractObject* obj;
		// Ijk Grid
		obj = epcPackageRepresentation->getDataObjectByUuid(getUuid());
		auto ijkGridRepresentation = static_cast<resqml2_0_1::AbstractIjkGridRepresentation*>(obj);
		result = ijkGridRepresentation->getICellCount();
	}

    L_(linfo) << loggClass << "FUNCTION=getICellCount " << "STATUS=OUT "<< "LEVEL=2 ";
	return result;
}

//----------------------------------------------------------------------------
int VtkIjkGridRepresentation::getJCellCount(const std::string & uuid) const
{
    L_(linfo) << loggClass << "FUNCTION=getJCellCount " << "STATUS=IN "<< "LEVEL=2 ";

    long result = 0;

	if (subRepresentation)
	{
		// SubRep
		result = jCellCount;
	}else
	{
		common::AbstractObject* obj;
		// Ijk Grid
		obj = epcPackageRepresentation->getDataObjectByUuid(getUuid());
		auto ijkGridRepresentation = static_cast<resqml2_0_1::AbstractIjkGridRepresentation*>(obj);
		result = ijkGridRepresentation->getJCellCount();
	}

    L_(linfo) << loggClass << "FUNCTION=getJCellCount " << "STATUS=OUT "<< "LEVEL=2 ";
	return result;
}

//----------------------------------------------------------------------------
int VtkIjkGridRepresentation::getKCellCount(const std::string & uuid) const
{
    L_(linfo) << loggClass << "FUNCTION=getKCellCount " << "STATUS=IN ";

	long result = 0;

	if (subRepresentation)
	{
		// SubRep
		result = kCellCount;
	}else
	{
		common::AbstractObject* obj;
		// Ijk Grid
		obj = epcPackageRepresentation->getDataObjectByUuid(getUuid());
		auto ijkGridRepresentation = static_cast<resqml2_0_1::AbstractIjkGridRepresentation*>(obj);
		result = ijkGridRepresentation->getKCellCount();
	}

    L_(linfo) << loggClass << "FUNCTION=getKCellCount " << "STATUS=OUT "<< "LEVEL=2 ";
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
    L_(linfo) << loggClass << "FUNCTION=createWithPoints " << "STATUS=IN "<< "LEVEL=2 ";

	vtkOutput = vtkSmartPointer<vtkUnstructuredGrid>::New();
	vtkOutput->SetPoints(pointsRepresentation);

	if (obj != nullptr && (obj->getXmlTag() == "SubRepresentation"))
	{
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

		for (auto vtkKCellIndex = initKIndex; vtkKCellIndex < maxKIndex; ++vtkKCellIndex)
		{
			for (auto vtkJCellIndex = 0; vtkJCellIndex < jCellCount; ++vtkJCellIndex)
			{
				for (auto vtkICellIndex = 0; vtkICellIndex < iCellCount; ++vtkICellIndex)
				{
					if (elementIndices[indice] == cellIndex)
					{
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
	else{
		if (obj != nullptr && (obj->getXmlTag() == resqml2_0_1::AbstractIjkGridRepresentation::XML_TAG || obj->getXmlTag() == resqml2_0_1::AbstractIjkGridRepresentation::XML_TAG_TRUNCATED))
		{
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

    L_(linfo) << loggClass << "FUNCTION=createWithPoints " << "STATUS=OUT "<< "LEVEL=2 ";
}
