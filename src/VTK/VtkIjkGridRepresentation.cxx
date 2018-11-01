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
vtkSmartPointer<vtkPoints> VtkIjkGridRepresentation::createpoint()
{
	if (!vtkPointsIsCreated())
	{
		points = vtkSmartPointer<vtkPoints>::New();

		common::AbstractObject* obj = nullptr;
		resqml2_0_1::AbstractIjkGridRepresentation* ijkGridRepresentation = nullptr;
		if (subRepresentation)
			obj = epcPackageRepresentation->getResqmlAbstractObjectByUuid(getParent());
		else
			obj = epcPackageRepresentation->getResqmlAbstractObjectByUuid(getUuid());

		if (obj != nullptr && (obj->getXmlTag() == "IjkGridRepresentation" || obj->getXmlTag() == "TruncatedIjkGridRepresentation"))
			ijkGridRepresentation = static_cast<resqml2_0_1::AbstractIjkGridRepresentation*>(obj);

		const auto kInterfaceNodeCount = ijkGridRepresentation->getXyzPointCountOfKInterfaceOfPatch(0);
		iCellCount = ijkGridRepresentation->getICellCount();
		jCellCount = ijkGridRepresentation->getJCellCount();
		kCellCount = ijkGridRepresentation->getKCellCount();

		checkHyperslabingCapacity(ijkGridRepresentation);
		if (isHyperslabed)
		{
			initKIndex = getIdProc() * (kCellCount / getMaxProc());
			maxKIndex = (getIdProc()+1) * (kCellCount / getMaxProc());

			if (getIdProc() == getMaxProc()-1)
				maxKIndex = kCellCount;


			for (auto kInterface = initKIndex; kInterface <= maxKIndex; ++kInterface)
			{
				auto allXyzPoints = new double[kInterfaceNodeCount * 3];
				ijkGridRepresentation->getXyzPointsOfKInterfaceOfPatch(kInterface, 0, allXyzPoints);

				double zIndice = 1;

				if (ijkGridRepresentation->getLocalCrs()->isDepthOriented())
					zIndice = -1;

				for (auto nodeIndex = 0; nodeIndex < kInterfaceNodeCount * 3; nodeIndex += 3)
				{
					points->InsertNextPoint(allXyzPoints[nodeIndex], allXyzPoints[nodeIndex + 1], allXyzPoints[nodeIndex + 2] * zIndice);
				}
			}
			std::string s = "ijkGrid idProc-maxProc : " + std::to_string(getIdProc()) + "-" + std::to_string(getMaxProc()) + " Points " + std::to_string(kInterfaceNodeCount*(maxKIndex-initKIndex)) +"\n";
			char const * pchar = s.c_str();
			vtkOutputWindowDisplayDebugText(pchar);

		} 
		else
		{
			initKIndex = 0;
			maxKIndex = kCellCount;

			// POINT
			const auto nodeCount = ijkGridRepresentation->getXyzPointCountOfAllPatches();
			auto allXyzPoints = new double[nodeCount * 3];
			ijkGridRepresentation->getXyzPointsOfAllPatchesInGlobalCrs(allXyzPoints);
			createVtkPoints(nodeCount, allXyzPoints, ijkGridRepresentation->getLocalCrs());

			delete[] allXyzPoints;
		}
	}

	return points;

}

//----------------------------------------------------------------------------
void VtkIjkGridRepresentation::createOutput(const std::string & uuid)
{
	createpoint(); // => POINTS
	if (!vtkOutput)	// => REPRESENTATION
	{
		if (subRepresentation)
		{
			// SubRep
			createWithPoints(points, epcPackageSubRepresentation->getResqmlAbstractObjectByUuid(getUuid()));
		}else
		{
			// Ijk Grid
			createWithPoints(points, epcPackageRepresentation->getResqmlAbstractObjectByUuid(getUuid()));
		}
	}

	if (uuid != getUuid()) // => PROPERTY UUID
	{
		if (subRepresentation) 
		{
			auto objRepProp = epcPackageSubRepresentation->getResqmlAbstractObjectByUuid(getUuid());
			auto property = static_cast<resqml2::SubRepresentation*>(objRepProp);
			auto cellCount = property->getElementCountOfPatch(0);
			if (isHyperslabed)
				addProperty(uuid, uuidToVtkProperty[uuid]->loadValuesPropertySet(property->getValuesPropertySet(), iCellCount*jCellCount*(maxKIndex - initKIndex), 0, iCellCount, jCellCount, maxKIndex - initKIndex, initKIndex));
			else
				addProperty(uuid, uuidToVtkProperty[uuid]->loadValuesPropertySet(property->getValuesPropertySet(), cellCount, 0));
		}
		else
		{
			auto objRepProp = epcPackageRepresentation->getResqmlAbstractObjectByUuid(getUuid());
			auto property = static_cast<resqml2_0_1::AbstractIjkGridRepresentation*>(objRepProp);
			auto cellCount = iCellCount*jCellCount*kCellCount;
			if (isHyperslabed)
			{
				addProperty(uuid, uuidToVtkProperty[uuid]->loadValuesPropertySet(property->getValuesPropertySet(), iCellCount*jCellCount*(maxKIndex - initKIndex), 0, iCellCount, jCellCount, maxKIndex - initKIndex, initKIndex));
			}
			else
			{
				addProperty(uuid, uuidToVtkProperty[uuid]->loadValuesPropertySet(property->getValuesPropertySet(), cellCount, 0));
			}
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
	catch (const std::exception & e)
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
	long result = 0;
	if (propertyUnit == VtkEpcCommon::CELLS)
	{
		common::AbstractObject* obj;
		if (subRepresentation)
		{
			// SubRep
			obj = epcPackageSubRepresentation->getResqmlAbstractObjectByUuid(getUuid());
			auto subRepresentation1 = static_cast<resqml2::SubRepresentation*>(obj);
			return isHyperslabed ? iCellCount*jCellCount*(maxKIndex - initKIndex) : subRepresentation1->getElementCountOfPatch(0);
				result = iCellCount*jCellCount*(maxKIndex - initKIndex);
			else
				result = subRepresentation1->getElementCountOfPatch(0);
		}else
		{
			// Ijk Grid
			obj = epcPackageRepresentation->getResqmlAbstractObjectByUuid(getUuid());
			auto ijkGridRepresentation = static_cast<resqml2_0_1::AbstractIjkGridRepresentation*>(obj);
			result = ijkGridRepresentation->getCellCount();
		}
	}
	return 0;

}

//----------------------------------------------------------------------------
int VtkIjkGridRepresentation::getICellCount(const std::string & uuid) const
{
	long result = 0;

	if (subRepresentation)
	{
		// SubRep
		result = iCellCount;
	}else
	{
		common::AbstractObject* obj;
		// Ijk Grid
		obj = epcPackageRepresentation->getResqmlAbstractObjectByUuid(getUuid());
		auto ijkGridRepresentation = static_cast<resqml2_0_1::AbstractIjkGridRepresentation*>(obj);
		result = ijkGridRepresentation->getICellCount();
	}
	return result;
}

//----------------------------------------------------------------------------
int VtkIjkGridRepresentation::getJCellCount(const std::string & uuid) const
{
	long result = 0;

	if (subRepresentation)
	{
		// SubRep
		result = jCellCount;
	}else
	{
		common::AbstractObject* obj;
		// Ijk Grid
		obj = epcPackageRepresentation->getResqmlAbstractObjectByUuid(getUuid());
		auto ijkGridRepresentation = static_cast<resqml2_0_1::AbstractIjkGridRepresentation*>(obj);
		result = ijkGridRepresentation->getJCellCount();
	}
	return result;
}

//----------------------------------------------------------------------------
int VtkIjkGridRepresentation::getKCellCount(const std::string & uuid) const
{
	long result = 0;

	if (subRepresentation)
	{
		// SubRep
		result = kCellCount;
	}else
	{
		common::AbstractObject* obj;
		// Ijk Grid
		obj = epcPackageRepresentation->getResqmlAbstractObjectByUuid(getUuid());
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

	if (obj != nullptr && (obj->getXmlTag() == "SubRepresentation"))
	{
		resqml2::SubRepresentation* subRepresentation1 = static_cast<resqml2::SubRepresentation*>(obj);
		resqml2_0_1::AbstractIjkGridRepresentation* ijkGridRepresentation = static_cast<resqml2_0_1::AbstractIjkGridRepresentation*>(epcPackageRepresentation->getResqmlAbstractObjectByUuid(subRepresentation1->getSupportingRepresentationUuid(0)));

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
						hex->GetPointIds()->SetId(0, ijkGridRepresentation->getXyzPointIndexFromCellCorner(vtkICellIndex, vtkJCellIndex, vtkKCellIndex, 0));
						hex->GetPointIds()->SetId(1, ijkGridRepresentation->getXyzPointIndexFromCellCorner(vtkICellIndex, vtkJCellIndex, vtkKCellIndex, 1));
						hex->GetPointIds()->SetId(2, ijkGridRepresentation->getXyzPointIndexFromCellCorner(vtkICellIndex, vtkJCellIndex, vtkKCellIndex, 2));
						hex->GetPointIds()->SetId(3, ijkGridRepresentation->getXyzPointIndexFromCellCorner(vtkICellIndex, vtkJCellIndex, vtkKCellIndex, 3));
						hex->GetPointIds()->SetId(4, ijkGridRepresentation->getXyzPointIndexFromCellCorner(vtkICellIndex, vtkJCellIndex, vtkKCellIndex, 4));
						hex->GetPointIds()->SetId(5, ijkGridRepresentation->getXyzPointIndexFromCellCorner(vtkICellIndex, vtkJCellIndex, vtkKCellIndex, 5));
						hex->GetPointIds()->SetId(6, ijkGridRepresentation->getXyzPointIndexFromCellCorner(vtkICellIndex, vtkJCellIndex, vtkKCellIndex, 6));
						hex->GetPointIds()->SetId(7, ijkGridRepresentation->getXyzPointIndexFromCellCorner(vtkICellIndex, vtkJCellIndex, vtkKCellIndex, 7));
						vtkOutput->InsertNextCell(hex->GetCellType(), hex->GetPointIds());
						indice++;
					}
					++cellIndex;
				}
			}
		}
		ijkGridRepresentation->unloadSplitInformation();
	}
	else{
		if (obj != nullptr && (obj->getXmlTag() == "IjkGridRepresentation" || obj->getXmlTag() == "TruncatedIjkGridRepresentation"))
		{
			resqml2_0_1::AbstractIjkGridRepresentation* ijkGridRepresentation = static_cast<resqml2_0_1::AbstractIjkGridRepresentation*>(obj);

			std::string s = "ijkGrid idProc-maxProc : " + std::to_string(getIdProc()) + "-" + std::to_string(getMaxProc()) + " Kcell " + std::to_string(initKIndex) + " to " + std::to_string(maxKIndex) +"\n";
			char const * pchar = s.c_str();
			vtkOutputWindowDisplayDebugText(pchar);

			ijkGridRepresentation->loadSplitInformation();

			auto test = new bool[ijkGridRepresentation->getCellCount()];
			if (ijkGridRepresentation->hasEnabledCellInformation())
			{
				ijkGridRepresentation->getEnabledCells(test);
			}
			else
			{
				for (auto j = 0; j < ijkGridRepresentation->getCellCount(); ++j)
				{
					test[j] = true;
				}
			}

			unsigned int cellIndex = 0;

			ULONG64 translatePoint = ijkGridRepresentation->getXyzPointCountOfKInterfaceOfPatch(0)*initKIndex;

			for (auto vtkKCellIndex = initKIndex; vtkKCellIndex < maxKIndex; ++vtkKCellIndex)
			{
				for (auto vtkJCellIndex = 0; vtkJCellIndex < jCellCount; ++vtkJCellIndex)
				{
					for (auto vtkICellIndex = 0; vtkICellIndex < iCellCount; ++vtkICellIndex)
					{
						if (test[cellIndex])
						{
							vtkSmartPointer<vtkHexahedron> hex = vtkSmartPointer<vtkHexahedron>::New();
							auto point0 = ijkGridRepresentation->getXyzPointIndexFromCellCorner(vtkICellIndex, vtkJCellIndex, vtkKCellIndex, 0);
							hex->GetPointIds()->SetId(0, point0-translatePoint);

							auto point1 = ijkGridRepresentation->getXyzPointIndexFromCellCorner(vtkICellIndex, vtkJCellIndex, vtkKCellIndex, 1);
							hex->GetPointIds()->SetId(1, point1-translatePoint);

							auto point2 = ijkGridRepresentation->getXyzPointIndexFromCellCorner(vtkICellIndex, vtkJCellIndex, vtkKCellIndex, 2);
							hex->GetPointIds()->SetId(2, point2-translatePoint);

							auto point3 = ijkGridRepresentation->getXyzPointIndexFromCellCorner(vtkICellIndex, vtkJCellIndex, vtkKCellIndex, 3);
							hex->GetPointIds()->SetId(3, point3-translatePoint);

							auto point4 = ijkGridRepresentation->getXyzPointIndexFromCellCorner(vtkICellIndex, vtkJCellIndex, vtkKCellIndex, 4);
							hex->GetPointIds()->SetId(4, point4-translatePoint);

							auto point5 = ijkGridRepresentation->getXyzPointIndexFromCellCorner(vtkICellIndex, vtkJCellIndex, vtkKCellIndex, 5);
							hex->GetPointIds()->SetId(5, point5-translatePoint);

							auto point6 = ijkGridRepresentation->getXyzPointIndexFromCellCorner(vtkICellIndex, vtkJCellIndex, vtkKCellIndex, 6);
							hex->GetPointIds()->SetId(6, point6-translatePoint);

							auto point7 = ijkGridRepresentation->getXyzPointIndexFromCellCorner(vtkICellIndex, vtkJCellIndex, vtkKCellIndex, 7);
							hex->GetPointIds()->SetId(7, point7-translatePoint);

							vtkOutput->InsertNextCell(hex->GetCellType(), hex->GetPointIds());
						}
						else
						{
							vtkSmartPointer<vtkEmptyCell> emptyCell = vtkSmartPointer<vtkEmptyCell>::New();
							vtkOutput->InsertNextCell(emptyCell->GetCellType(), emptyCell->GetPointIds());
						}
						++cellIndex;
					}
				}
			}
			delete[] test;
			ijkGridRepresentation->unloadSplitInformation();
		}
	}
}
