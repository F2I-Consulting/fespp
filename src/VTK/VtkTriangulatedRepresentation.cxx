#include "VtkTriangulatedRepresentation.h"

// include VTK library
#include <vtkPointData.h>
#include <vtkCellArray.h>
#include <vtkSmartPointer.h>
#include <vtkTriangle.h>

// include F2i-consulting Energistics Standards API
#include <resqml2_0_1/TriangulatedSetRepresentation.h>

// include F2i-consulting Energistics Standards ParaView Plugin
#include "VtkProperty.h"

//----------------------------------------------------------------------------
VtkTriangulatedRepresentation::VtkTriangulatedRepresentation(const std::string & fileName, const std::string & name, const std::string & uuid, const std::string & uuidParent, const unsigned int & patchNo, common::EpcDocument *pckRep, common::EpcDocument *pckSubRep) :
VtkResqml2PolyData(fileName, name, uuid, uuidParent, pckRep, pckSubRep), patchIndex(patchNo)
{
}

//----------------------------------------------------------------------------
VtkTriangulatedRepresentation::~VtkTriangulatedRepresentation()
{
	cout << "VtkTriangulatedRepresentation::~VtkTriangulatedRepresentation() " << getUuid() << "\n";
	patchIndex = 0;
	lastProperty = "";
}

//----------------------------------------------------------------------------
void VtkTriangulatedRepresentation::createOutput(const std::string &uuid)
{
	if (!subRepresentation)	{

		resqml2_0_1::TriangulatedSetRepresentation* triangulatedSetRepresentation = nullptr;
		common::AbstractObject* obj = epcPackageRepresentation->getResqmlAbstractObjectByUuid(getUuid());
		if (obj != nullptr && obj->getXmlTag() == "TriangulatedSetRepresentation")
		{
			triangulatedSetRepresentation = static_cast<resqml2_0_1::TriangulatedSetRepresentation*>(obj);
		}

		if (!vtkOutput)
		{
			vtkOutput = vtkSmartPointer<vtkPolyData>::New();

			// POINT
			vtkSmartPointer<vtkPoints> triangulatedRepresentationPoints = vtkSmartPointer<vtkPoints>::New();

			unsigned int nodeCount = triangulatedSetRepresentation->getXyzPointCountOfAllPatches();
			double* allXyzPoints = new double[nodeCount * 3];
			triangulatedSetRepresentation->getXyzPointsOfAllPatchesInGlobalCrs(allXyzPoints);

			createVtkPoints(nodeCount, allXyzPoints, triangulatedSetRepresentation->getLocalCrs());

			delete[] allXyzPoints;

			// CELLS
			vtkSmartPointer<vtkCellArray> triangulatedRepresentationTriangles = vtkSmartPointer<vtkCellArray>::New();
			unsigned int * triangleIndices = new unsigned int[triangulatedSetRepresentation->getTriangleCountOfPatch(this->patchIndex) * 3];
			triangulatedSetRepresentation->getTriangleNodeIndicesOfPatch(this->patchIndex, triangleIndices);
			for (unsigned int p = 0; p < triangulatedSetRepresentation->getTriangleCountOfPatch(this->patchIndex); ++p)
			{
				vtkSmartPointer<vtkTriangle> triangulatedRepresentationTriangle = vtkSmartPointer<vtkTriangle>::New();
				triangulatedRepresentationTriangle->GetPointIds()->SetId(0, triangleIndices[p * 3]);
				triangulatedRepresentationTriangle->GetPointIds()->SetId(1, triangleIndices[p * 3 + 1]);
				triangulatedRepresentationTriangle->GetPointIds()->SetId(2, triangleIndices[p * 3 + 2]);
				triangulatedRepresentationTriangles->InsertNextCell(triangulatedRepresentationTriangle);
			}
			vtkOutput->SetPoints(points);
			vtkOutput->SetPolys(triangulatedRepresentationTriangles);

			points = nullptr;
		}
		else
		{
			if (uuid != getUuid())
			{
				vtkDataArray* arrayProperty = uuidToVtkProperty[uuid]->visualize(uuid, triangulatedSetRepresentation);
				this->addProperty(uuid, arrayProperty);
			}
		}
	}
}

//----------------------------------------------------------------------------
void VtkTriangulatedRepresentation::addProperty(const std::string & uuidProperty, vtkDataArray* dataProperty)
{
	vtkOutput->Modified();
	vtkOutput->GetPointData()->AddArray(dataProperty);
	lastProperty = uuidProperty;
}

long VtkTriangulatedRepresentation::getAttachmentPropertyCount(const std::string & uuid, const VtkEpcCommon::FesppAttachmentProperty propertyUnit)
{
	long result = 0;
	resqml2_0_1::TriangulatedSetRepresentation* triangulatedSetRepresentation = nullptr;
	common::AbstractObject* obj = epcPackageRepresentation->getResqmlAbstractObjectByUuid(getUuid());
	if (obj != nullptr && obj->getXmlTag() == "TriangulatedSetRepresentation")
	{
		triangulatedSetRepresentation = static_cast<resqml2_0_1::TriangulatedSetRepresentation*>(obj);
		result = triangulatedSetRepresentation->getXyzPointCountOfAllPatches();
	}
	return result;
}

