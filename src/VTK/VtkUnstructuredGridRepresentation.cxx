#include "VtkUnstructuredGridRepresentation.h"

#include <sstream>

// include VTK library
#include <vtkSmartPointer.h>
#include <vtkCellData.h>
#include <vtkPointData.h>
#include <vtkCellArray.h>
#include <vtkPolyhedron.h>
#include <vtkTetra.h>

// include F2i-consulting Energistics Standards API
#include <common/EpcDocument.h>
#include <resqml2_0_1/UnstructuredGridRepresentation.h>

// include F2i-consulting Energistics Standards ParaView Plugin
#include "VtkProperty.h"

//----------------------------------------------------------------------------
VtkUnstructuredGridRepresentation::VtkUnstructuredGridRepresentation(const std::string & fileName, const std::string & name, const std::string & uuid, const std::string & uuidParent, common::EpcDocument *pckRep, common::EpcDocument *pckSubRep, const int & idProc, const int & maxProc) :
VtkResqml2UnstructuredGrid(fileName, name, uuid, uuidParent, pckRep, pckSubRep, idProc, maxProc)
{
}


//----------------------------------------------------------------------------
VtkUnstructuredGridRepresentation::~VtkUnstructuredGridRepresentation()
{
	cout << "VtkUnstructuredGridRepresentation::~VtkUnstructuredGridRepresentation() " << getUuid() << "\n";
	lastProperty = "";
}

//----------------------------------------------------------------------------
void VtkUnstructuredGridRepresentation::createOutput(const std::string & uuid)
{

	if (!subRepresentation)	{

		resqml2_0_1::UnstructuredGridRepresentation* unstructuredGridRep = nullptr;
		common::AbstractObject* obj = epcPackageRepresentation->getResqmlAbstractObjectByUuid(getUuid());
		if (obj != nullptr && obj->getXmlTag() == "UnstructuredGridRepresentation")
		{
			unstructuredGridRep = static_cast<resqml2_0_1::UnstructuredGridRepresentation*>(obj);
		}

		if (!vtkOutput)
		{
			vtkOutput = vtkSmartPointer<vtkUnstructuredGrid>::New();
			// POINTS
			ULONG64 pointCount = unstructuredGridRep->getXyzPointCountOfAllPatches();
			double* allXyzPoints = new double[pointCount * 3];
			unstructuredGridRep->getXyzPointsOfAllPatchesInGlobalCrs(allXyzPoints);
			createVtkPoints(pointCount, allXyzPoints, unstructuredGridRep->getLocalCrs());
			vtkOutput->SetPoints(points);

			delete[] allXyzPoints;
			points = nullptr;

			// CELLS
			unsigned int nodeCount = unstructuredGridRep->getNodeCount();
			vtkIdType* pointIds = new vtkIdType[nodeCount];
			for (unsigned int i = 0; i < nodeCount; ++i)
				pointIds[i] = i;

			unstructuredGridRep->loadGeometry();
			bool isOptimized = false;
			if (unstructuredGridRep->isFaceCountOfCellsConstant() && unstructuredGridRep->isNodeCountOfFacesConstant())
			{
				unsigned int constantFaceCountOfCells = unstructuredGridRep->getConstantFaceCountOfCells();
				unsigned int constantNodeCountOfFaces = unstructuredGridRep->getConstantNodeCountOfFaces();

				if (constantFaceCountOfCells == 4 && constantNodeCountOfFaces == 3)
				{
					vtkOutput->SetCells(VTK_TETRA, createOutputVtkTetra(unstructuredGridRep));
					isOptimized = true;
				}
			}
			if (!isOptimized)
			{
				const ULONG64 cellCount = unstructuredGridRep->getCellCount();
				auto initCellIndex = getIdProc() * (cellCount/getMaxProc());
				auto maxCellIndex = (getIdProc()+1) * (cellCount/getMaxProc());

				cout << "unstructuredGrid " << getIdProc() << "-" << getMaxProc() << " : " << initCellIndex << " to " << maxCellIndex << "\n";
				for (ULONG64 cellIndex = initCellIndex; cellIndex < maxCellIndex; ++cellIndex)
				{
					vtkSmartPointer<vtkCellArray> faces = vtkSmartPointer<vtkCellArray>::New();
					const ULONG64 localFaceCount = unstructuredGridRep->getFaceCountOfCell(cellIndex);
					for (ULONG64 localFaceIndex = 0; localFaceIndex < localFaceCount; ++localFaceIndex)
					{
						const unsigned int localNodeCount = unstructuredGridRep->getNodeCountOfFaceOfCell(cellIndex, localFaceIndex);
						vtkIdType* nodes = new vtkIdType[localNodeCount];
						ULONG64* nodeIndices = unstructuredGridRep->getNodeIndicesOfFaceOfCell(cellIndex, localFaceIndex);
						for (unsigned int i = 0; i < localNodeCount; ++i)
						{
							nodes[i] = nodeIndices[i];
						}
						faces->InsertNextCell(localNodeCount, nodes);
						delete[] nodes;
					}
					vtkOutput->InsertNextCell(VTK_POLYHEDRON, nodeCount, pointIds, unstructuredGridRep->getFaceCountOfCell(cellIndex), faces->GetPointer());

					faces = nullptr;
				}

				delete[] pointIds;
			}
			unstructuredGridRep->unloadGeometry();
		}
		// PROPERTY(IES)
		else
		{
			if (uuid != getUuid())
			{
				vtkDataArray* arrayProperty = uuidToVtkProperty[uuid]->visualize(uuid, unstructuredGridRep);
				this->addProperty(uuid, arrayProperty);
			}
		}
	}

}

vtkSmartPointer<vtkCellArray> VtkUnstructuredGridRepresentation::createOutputVtkTetra(const resqml2_0_1::UnstructuredGridRepresentation* unstructuredGridRep)
{
	vtkSmartPointer<vtkCellArray> cellArray = vtkSmartPointer<vtkCellArray>::New();

	const ULONG64 cellCount = unstructuredGridRep->getCellCount();

	auto initCellIndex = getIdProc() * (cellCount/getMaxProc());
	auto maxCellIndex = (getIdProc()+1) * (cellCount/getMaxProc());

	cout << "unstructuredGrid " << getIdProc() << "-" << getMaxProc() << " : " << initCellIndex << " to " << maxCellIndex << "\n";

	for (ULONG64 cellIndex = 0; cellIndex < cellCount; ++cellIndex)
	{

		vtkSmartPointer<vtkTetra> tetra = vtkSmartPointer<vtkTetra>::New();
		unsigned int* nodes = new unsigned int[4];

		// Face 1
		ULONG64* nodeIndices = unstructuredGridRep->getNodeIndicesOfFaceOfCell(cellIndex, 0);
		nodes[0] = nodeIndices[0];
		nodes[1] = nodeIndices[1];
		nodes[2] = nodeIndices[2];

		// Face 2
		nodeIndices = unstructuredGridRep->getNodeIndicesOfFaceOfCell(cellIndex, 1);

		if (!(nodeIndices[0] == nodes[0] || nodeIndices[0] == nodes[1] || nodeIndices[0] == nodes[2]))
			nodes[3] = nodeIndices[0];
		else if (!(nodeIndices[1] == nodes[0] || nodeIndices[1] == nodes[1] || nodeIndices[1] == nodes[2]))
			nodes[3] = nodeIndices[1];
		else if (!(nodeIndices[2] == nodes[0] || nodeIndices[2] == nodes[1] || nodeIndices[2] == nodes[2]))
			nodes[3] = nodeIndices[2];
		else if (!(nodeIndices[3] == nodes[0] || nodeIndices[3] == nodes[1] || nodeIndices[3] == nodes[2]))
			nodes[3] = nodeIndices[3];


		tetra->GetPointIds()->SetId(0, nodes[0]);
		tetra->GetPointIds()->SetId(1, nodes[1]);
		tetra->GetPointIds()->SetId(2, nodes[2]);
		tetra->GetPointIds()->SetId(3, nodes[3]);

		cellArray->InsertNextCell(tetra);
		delete[] nodes;
	}

	return cellArray;
}

//----------------------------------------------------------------------------
void VtkUnstructuredGridRepresentation::addProperty(const std::string & uuidProperty, vtkDataArray* dataProperty)
{
	vtkOutput->Modified();
	if (uuidToVtkProperty[uuidProperty]->getSupport() == VtkProperty::typeSupport::CELLS)
		vtkOutput->GetCellData()->AddArray(dataProperty);
	if (uuidToVtkProperty[uuidProperty]->getSupport() == VtkProperty::typeSupport::POINTS)
		vtkOutput->GetPointData()->AddArray(dataProperty);
	lastProperty = uuidProperty;
}

long VtkUnstructuredGridRepresentation::getAttachmentPropertyCount(const std::string & uuid, const FesppAttachmentProperty propertyUnit)
{
	long result = 0;
	resqml2_0_1::UnstructuredGridRepresentation* unstructuredGridRep = nullptr;
	common::AbstractObject* obj = epcPackageRepresentation->getResqmlAbstractObjectByUuid(getUuid());
	if (obj != nullptr && obj->getXmlTag() == "UnstructuredGridRepresentation")
	{
		unstructuredGridRep = static_cast<resqml2_0_1::UnstructuredGridRepresentation*>(obj);
		if (propertyUnit == FesppAttachmentProperty::POINTS){
			result = unstructuredGridRep->getXyzPointCountOfAllPatches();
		}
		else if (propertyUnit==FesppAttachmentProperty::CELLS){
			result = unstructuredGridRep->getCellCount();
					}
	}
	return result;
}
