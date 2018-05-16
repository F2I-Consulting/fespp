
#include "VtkPolylineRepresentation.h"

// include VTK library
#include <vtkPointData.h>
#include <vtkCellArray.h>
#include <vtkPolyLine.h>
#include <vtkSmartPointer.h>

// include F2i-consulting Energistics Standards API
#include <resqml2_0_1/PolylineSetRepresentation.h>
#include <common/EpcDocument.h>

// include F2i-consulting Energistics Standards ParaView Plugin
#include "VtkProperty.h"

//----------------------------------------------------------------------------
VtkPolylineRepresentation::VtkPolylineRepresentation(const std::string & fileName, const std::string & name, const std::string & uuid, const std::string & uuidParent, const unsigned int & patchNo, common::EpcDocument *pckRep, common::EpcDocument *pckSubRep) :
VtkResqml2PolyData(fileName, name, uuid, uuidParent, pckRep, pckSubRep), patchIndex(patchNo)
{
}

//----------------------------------------------------------------------------
VtkPolylineRepresentation::~VtkPolylineRepresentation()
{
	patchIndex = 0;
	lastProperty = "";
}

//----------------------------------------------------------------------------
void VtkPolylineRepresentation::createOutput(const std::string & uuid)
{
	if (!subRepresentation)	{
		resqml2_0_1::PolylineSetRepresentation* polylineSetRepresentation = nullptr;
		common::AbstractObject* obj = epcPackageRepresentation->getResqmlAbstractObjectByUuid(getUuid());
		if (obj != nullptr && obj->getXmlTag() == "PolylineSetRepresentation")
		{
			polylineSetRepresentation = static_cast<resqml2_0_1::PolylineSetRepresentation*>(obj);
		}

		if (!vtkOutput)
		{
			VtkResqml2PolyData::vtkOutput = vtkSmartPointer<vtkPolyData>::New();

			// POINT
			unsigned int nodeCount = polylineSetRepresentation->getXyzPointCountOfPatch(this->patchIndex);

			double * allPoint = new double[nodeCount * 3];
			polylineSetRepresentation->getXyzPointsOfPatch(this->patchIndex, allPoint);

			createVtkPoints(nodeCount, allPoint, polylineSetRepresentation->getLocalCrs());
			vtkOutput->SetPoints(points);

			delete[] allPoint;

			// POLYLINE
			vtkSmartPointer<vtkCellArray> setPolylineRepresentationLines = vtkSmartPointer<vtkCellArray>::New();

			unsigned int countPolyline = polylineSetRepresentation->getPolylineCountOfPatch(this->patchIndex);

			unsigned int* countNodePolylineInPatch = new unsigned int[countPolyline];
			polylineSetRepresentation->getNodeCountPerPolylineInPatch(this->patchIndex, countNodePolylineInPatch);

			unsigned int idPoint = 0;
			for (unsigned int polylineIndex = 0; polylineIndex < countPolyline; ++polylineIndex)
			{
				vtkSmartPointer<vtkPolyLine> polylineRepresentation = vtkSmartPointer<vtkPolyLine>::New();
				polylineRepresentation->GetPointIds()->SetNumberOfIds(countNodePolylineInPatch[polylineIndex]);
				for (unsigned int line = 0; line < countNodePolylineInPatch[polylineIndex]; ++line)
				{
					polylineRepresentation->GetPointIds()->SetId(line, idPoint);
					idPoint++;

				}
				setPolylineRepresentationLines->InsertNextCell(polylineRepresentation);
				vtkOutput->SetLines(setPolylineRepresentationLines);
			}
			points = nullptr;
			delete[] countNodePolylineInPatch;
		}
		else
		{
			if (uuid != getUuid())
			{
				vtkDataArray* arrayProperty = uuidToVtkProperty[uuid]->visualize(uuid, polylineSetRepresentation);
				this->addProperty(uuid, arrayProperty);
			}
		}
	}
}

//----------------------------------------------------------------------------
void VtkPolylineRepresentation::addProperty(const std::string & uuidProperty, vtkDataArray* dataProperty)
{
	vtkOutput->Modified();
	vtkOutput->GetPointData()->AddArray(dataProperty);
	lastProperty = uuidProperty;
}

long VtkPolylineRepresentation::getAttachmentPropertyCount(const std::string & uuid, const FesppAttachmentProperty propertyUnit)
{
	long result = 0;
	resqml2_0_1::PolylineSetRepresentation* polylineSetRepresentation = nullptr;
	common::AbstractObject* obj = epcPackageRepresentation->getResqmlAbstractObjectByUuid(getUuid());
	if (obj != nullptr && obj->getXmlTag() == "PolylineSetRepresentation")
	{
		polylineSetRepresentation = static_cast<resqml2_0_1::PolylineSetRepresentation*>(obj);
		result = polylineSetRepresentation->getXyzPointCountOfPatch(0);
	}
	return result;
}
