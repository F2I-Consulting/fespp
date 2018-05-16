#include "VtkWellboreTrajectoryRepresentationPolyLine.h"

// include VTK library
#include <vtkPointData.h>
#include <vtkCellArray.h>
#include <vtkPolyLine.h>
#include <vtkSmartPointer.h>
#include <vtkDataArray.h>

// include F2i-consulting Energistics Standards API 
#include <common/EpcDocument.h>
#include <resqml2_0_1/WellboreTrajectoryRepresentation.h>

// include F2i-consulting Energistics Standards ParaView Plugin
#include "VtkProperty.h"

//----------------------------------------------------------------------------
VtkWellboreTrajectoryRepresentationPolyLine::VtkWellboreTrajectoryRepresentationPolyLine(const std::string & fileName, const std::string & name, const std::string & uuid, const std::string & uuidParent, common::EpcDocument *pckRep, common::EpcDocument *pckSubRep) :
VtkResqml2PolyData(fileName, name, uuid, uuidParent, pckRep, pckSubRep)
{
}

//----------------------------------------------------------------------------
void VtkWellboreTrajectoryRepresentationPolyLine::createOutput(const std::string & uuid)
{
	if (!subRepresentation)	{

		resqml2_0_1::WellboreTrajectoryRepresentation* wellboreSetRepresentation = nullptr;
		std::string teoto = getUuid().substr(0, 36);
		common::AbstractObject* obj = epcPackageRepresentation->getResqmlAbstractObjectByUuid(getUuid().substr(0, 36));
		if (obj != nullptr && obj->getXmlTag() == "WellboreTrajectoryRepresentation")
		{
			wellboreSetRepresentation = static_cast<resqml2_0_1::WellboreTrajectoryRepresentation*>(obj);
		}

		if (!vtkOutput)
		{
			vtkOutput = vtkSmartPointer<vtkPolyData>::New();

			// POINT
			unsigned int pointCount = wellboreSetRepresentation->getXyzPointCountOfPatch(0);
			double * allXyzPoints = new double[pointCount * 3];
			wellboreSetRepresentation->getXyzPointsOfAllPatchesInGlobalCrs(allXyzPoints);
			createVtkPoints(pointCount, allXyzPoints, wellboreSetRepresentation->getLocalCrs());
			vtkOutput->SetPoints(points);

			delete[] allXyzPoints;

			// POLYLINE
			vtkSmartPointer<vtkPolyLine> polylineRepresentation = vtkSmartPointer<vtkPolyLine>::New();
			polylineRepresentation->GetPointIds()->SetNumberOfIds(pointCount);
			for (unsigned int nodeIndex = 0; nodeIndex < pointCount; ++nodeIndex)
			{
				polylineRepresentation->GetPointIds()->SetId(nodeIndex, nodeIndex);
			}

			vtkSmartPointer<vtkCellArray> setPolylineRepresentationLines = vtkSmartPointer<vtkCellArray>::New();
			setPolylineRepresentationLines->InsertNextCell(polylineRepresentation);

			vtkOutput->SetLines(setPolylineRepresentationLines);
			points = nullptr;
		}
		// PROPERTY(IES)
		else
		{
			if (uuid != getUuid().substr(0, 36))
			{
				vtkDataArray* arrayProperty = uuidToVtkProperty[uuid]->visualize(uuid, wellboreSetRepresentation);
				this->addProperty(uuid, arrayProperty);
			}
		}
	}
}


//----------------------------------------------------------------------------
VtkWellboreTrajectoryRepresentationPolyLine::~VtkWellboreTrajectoryRepresentationPolyLine()
{
	lastProperty = "";
}

//----------------------------------------------------------------------------
void VtkWellboreTrajectoryRepresentationPolyLine::addProperty(const std::string uuidProperty, vtkDataArray* dataProperty)
{
	vtkOutput->Modified();
	vtkOutput->GetPointData()->AddArray(dataProperty);
	lastProperty = uuidProperty;
}

long VtkWellboreTrajectoryRepresentationPolyLine::getAttachmentPropertyCount(const std::string & uuid, const FesppAttachmentProperty propertyUnit)
{
	long result = 0;
	resqml2_0_1::WellboreTrajectoryRepresentation* wellboreSetRepresentation = nullptr;
	common::AbstractObject* obj = epcPackageRepresentation->getResqmlAbstractObjectByUuid(getUuid().substr(0, 36));
	if (obj != nullptr && obj->getXmlTag() == "WellboreTrajectoryRepresentation")
	{
		wellboreSetRepresentation = static_cast<resqml2_0_1::WellboreTrajectoryRepresentation*>(obj);
		result = wellboreSetRepresentation->getXyzPointCountOfAllPatches();
	}
	return result;
}
