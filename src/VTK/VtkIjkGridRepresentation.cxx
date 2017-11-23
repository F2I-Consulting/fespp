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

//----------------------------------------------------------------------------
VtkIjkGridRepresentation::VtkIjkGridRepresentation(const std::string & fileName, const std::string & name, const std::string & uuid, const std::string & uuidParent, common::EpcDocument *pckEPCRep, common::EpcDocument *pckEPCSubRep) :
VtkResqml2UnstructuredGrid(fileName, name, uuid, uuidParent, pckEPCRep, pckEPCSubRep)
{
}

vtkSmartPointer<vtkPoints> VtkIjkGridRepresentation::createpoint()
{
	resqml2_0_1::AbstractIjkGridRepresentation* ijkGridRepresentation = nullptr;
	resqml2::AbstractObject* obj = epcPackageRepresentation->getResqmlAbstractObjectByUuid(getUuid());
	ijkGridRepresentation = static_cast<resqml2_0_1::AbstractIjkGridRepresentation*>(obj);
	if (!vtkPointsIsCreated())
	{
		// POINT
		const auto nodeCount = ijkGridRepresentation->getXyzPointCountOfAllPatches();
		auto allXyzPoints = new double[nodeCount * 3];
		ijkGridRepresentation->getXyzPointsOfAllPatchesInGlobalCrs(allXyzPoints);

		createVtkPoints(nodeCount, allXyzPoints, ijkGridRepresentation->getLocalCrs());

		delete[] allXyzPoints;
	}
	return points;

}

//----------------------------------------------------------------------------
void VtkIjkGridRepresentation::createOutput(const std::string & uuid)
{
	resqml2_0_1::AbstractIjkGridRepresentation* ijkGridRepresentation = nullptr;

	if (!subRepresentation)	{
		auto obj = epcPackageRepresentation->getResqmlAbstractObjectByUuid(getUuid());

		if (obj != nullptr && (obj->getXmlTag() == "IjkGridRepresentation" || obj->getXmlTag() == "TruncatedIjkGridRepresentation"))
		{
			ijkGridRepresentation = static_cast<resqml2_0_1::AbstractIjkGridRepresentation*>(obj);
		}

		if (!vtkPointsIsCreated())	{
			// POINT
			const auto nodeCount = ijkGridRepresentation->getXyzPointCountOfAllPatches();
			auto allXyzPoints = new double[nodeCount * 3];
			ijkGridRepresentation->getXyzPointsOfAllPatchesInGlobalCrs(allXyzPoints);

			createVtkPoints(nodeCount, allXyzPoints, ijkGridRepresentation->getLocalCrs());

			delete[] allXyzPoints;
		}
		if (!vtkOutput)	{
			// Ijk Grid
			createWithPoints(points, epcPackageRepresentation->getResqmlAbstractObjectByUuid(getUuid()));
		}
	}
	else{
		auto obj = epcPackageRepresentation->getResqmlAbstractObjectByUuid(getParent());

		if (obj != nullptr && (obj->getXmlTag() == "IjkGridRepresentation" || obj->getXmlTag() == "TruncatedIjkGridRepresentation"))
		{
			ijkGridRepresentation = static_cast<resqml2_0_1::AbstractIjkGridRepresentation*>(obj);
		}

		if (!vtkPointsIsCreated())	{
			// POINT
			const auto nodeCount = ijkGridRepresentation->getXyzPointCountOfAllPatches();
			auto allXyzPoints = new double[nodeCount * 3];
			ijkGridRepresentation->getXyzPointsOfAllPatchesInGlobalCrs(allXyzPoints);

			createVtkPoints(nodeCount, allXyzPoints, ijkGridRepresentation->getLocalCrs());

			delete[] allXyzPoints;
		}

		if (!vtkOutput)	{
			// Ijk Grid
			createWithPoints(points, epcPackageSubRepresentation->getResqmlAbstractObjectByUuid(getUuid()));
		}
		// PROPERTY(IES)
	}
	// PROPERTY(IES)
	if (uuid != getUuid()){
		if (subRepresentation) {
			auto objRepProp = epcPackageSubRepresentation->getResqmlAbstractObjectByUuid(getUuid());
			auto subRepProperty = static_cast<resqml2::SubRepresentation*>(objRepProp);
			addProperty(uuid, uuidToVtkProperty[uuid]->loadValuesPropertySet(subRepProperty->getValuesPropertySet(), subRepProperty->getElementCountOfPatch(0), 0));
		}
		else{
			auto objRepProp = epcPackageRepresentation->getResqmlAbstractObjectByUuid(getUuid());
			auto repProperty = static_cast<resqml2_0_1::AbstractIjkGridRepresentation*>(objRepProp);
			addProperty(uuid, uuidToVtkProperty[uuid]->loadValuesPropertySet(repProperty->getValuesPropertySet(), repProperty->getCellCount(), 0));
		}
	}
}

//----------------------------------------------------------------------------
void VtkIjkGridRepresentation::addProperty(const std::string uuidProperty, vtkDataArray* dataProperty)
{
	for (int i = 0; i < vtkOutput->GetCellData()->GetNumberOfArrays(); ++i)
	{
		vtkOutput->GetCellData()->RemoveArray(vtkOutput->GetCellData()->GetArrayName(i));
	}

	vtkOutput->Modified();
	vtkOutput->GetCellData()->AddArray(dataProperty);
	vtkOutput->Modified();
	lastProperty = uuidProperty;
}

//----------------------------------------------------------------------------
long VtkIjkGridRepresentation::getAttachmentPropertyCount(const std::string & uuid, const FesppAttachmentProperty propertyUnit)
{
	long result = 0;
	resqml2_0_1::AbstractIjkGridRepresentation* ijkGridRepresentation = nullptr;
	resqml2::AbstractObject* obj = epcPackageRepresentation->getResqmlAbstractObjectByUuid(getUuid());
	if (obj != nullptr && (obj->getXmlTag() == "IjkGridRepresentation" || obj->getXmlTag() == "TruncatedIjkGridRepresentation")) {
		ijkGridRepresentation = static_cast<resqml2_0_1::AbstractIjkGridRepresentation*>(obj);
		result = ijkGridRepresentation->getCellCount();
	}
	return result;

}

//----------------------------------------------------------------------------
void VtkIjkGridRepresentation::createWithPoints(const vtkSmartPointer<vtkPoints> & pointsRepresentation, resqml2::AbstractObject* obj)
{
	vtkOutput = vtkSmartPointer<vtkUnstructuredGrid>::New();
	vtkOutput->SetPoints(pointsRepresentation);

	if (obj != nullptr && (obj->getXmlTag() == "SubRepresentation"))
	{
		resqml2::SubRepresentation* subRepresentation1 = nullptr;
		resqml2_0_1::AbstractIjkGridRepresentation* ijkGridRepresentation = nullptr;

		subRepresentation1 = static_cast<resqml2::SubRepresentation*>(obj);
		ijkGridRepresentation = static_cast<resqml2_0_1::AbstractIjkGridRepresentation*>(epcPackageRepresentation->getResqmlAbstractObjectByUuid(subRepresentation1->getSupportingRepresentationUuid(0)));

		auto elementCountOfPatch = subRepresentation1->getElementCountOfPatch(0);
		auto elementIndices = new ULONG64[elementCountOfPatch];
		subRepresentation1->getElementIndicesOfPatch(0, 0, elementIndices);

		auto iCellCount = ijkGridRepresentation->getICellCount();
		auto jCellCount = ijkGridRepresentation->getJCellCount();
		auto kCellCount = ijkGridRepresentation->getKCellCount();
		ijkGridRepresentation->loadSplitInformation();

		auto indice = 0;
		unsigned int cellIndex = 0;
		for (auto vtkKCellIndex = 0; vtkKCellIndex < kCellCount; ++vtkKCellIndex)
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
			resqml2_0_1::AbstractIjkGridRepresentation* ijkGridRepresentation = nullptr;

			ijkGridRepresentation = static_cast<resqml2_0_1::AbstractIjkGridRepresentation*>(obj);

			auto iCellCount = ijkGridRepresentation->getICellCount();
			auto jCellCount = ijkGridRepresentation->getJCellCount();
			auto kCellCount = ijkGridRepresentation->getKCellCount();
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
			for (auto vtkKCellIndex = 0; vtkKCellIndex < kCellCount; ++vtkKCellIndex)
			{
				for (auto vtkJCellIndex = 0; vtkJCellIndex < jCellCount; ++vtkJCellIndex)
				{
					for (auto vtkICellIndex = 0; vtkICellIndex < iCellCount; ++vtkICellIndex)
					{
						if (test[cellIndex])
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
