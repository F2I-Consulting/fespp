#include "VtkWellboreTrajectoryRepresentationDatum.h"

// include VTK library
#include <vtkSmartPointer.h>
#include <vtkCellArray.h>
#include <vtkPyramid.h>
#include <vtkPoints.h>

// include F2i-consulting Energistics Standards API 
#include <EpcDocument.h>
#include <resqml2_0_1/WellboreTrajectoryRepresentation.h>
#include <resqml2/MdDatum.h>

//----------------------------------------------------------------------------
VtkWellboreTrajectoryRepresentationDatum::VtkWellboreTrajectoryRepresentationDatum(const std::string & fileName, const std::string & name, const std::string & uuid, const std::string & uuidParent, common::EpcDocument *pckRep, common::EpcDocument *pckSubRep) :
VtkResqml2UnstructuredGrid(fileName, name, uuid, uuidParent, pckRep, pckSubRep)
{
}

//----------------------------------------------------------------------------
void VtkWellboreTrajectoryRepresentationDatum::createOutput(const std::string & uuid)
{
	vtkOutput = vtkSmartPointer<vtkUnstructuredGrid>::New();
	resqml2_0_1::WellboreTrajectoryRepresentation* wellboreSetRepresentation = nullptr;
	resqml2::AbstractObject* obj = epcPackageRepresentation->getResqmlAbstractObjectByUuid(uuid);
	if (obj != nullptr && obj->getXmlTag() ==  "WellboreTrajectoryRepresentation")
	{
		wellboreSetRepresentation = static_cast<resqml2_0_1::WellboreTrajectoryRepresentation*>(obj);
	}

	resqml2::MdDatum* mdDatum = wellboreSetRepresentation->getMdDatum();

	const double xOrigin = mdDatum->getXInGlobalCrs();
	const double yOrigin = mdDatum->getYInGlobalCrs();
	const double zOrigin = mdDatum->getZInGlobalCrs();

	vtkSmartPointer<vtkPoints> points = vtkSmartPointer<vtkPoints>::New();
	double p0[3] = {xOrigin+1.5, yOrigin+1.5, zOrigin};
	double p1[3] = {xOrigin-1.5, yOrigin+1.5, zOrigin};
	double p2[3] = {xOrigin-1.5, yOrigin-1.5, zOrigin};
	double p3[3] = {xOrigin+1.5, yOrigin-1.5, zOrigin};
	double p4[3] = {xOrigin, yOrigin, zOrigin-10};

	points->InsertNextPoint(p0);
	points->InsertNextPoint(p1);
	points->InsertNextPoint(p2);
	points->InsertNextPoint(p3);
	points->InsertNextPoint(p4);

	vtkSmartPointer<vtkPyramid> pyramid = vtkSmartPointer<vtkPyramid>::New();
	pyramid->GetPointIds()->SetId(0,0);
	pyramid->GetPointIds()->SetId(1,1);
	pyramid->GetPointIds()->SetId(2,2);
	pyramid->GetPointIds()->SetId(3,3);
	pyramid->GetPointIds()->SetId(4,4);

	vtkSmartPointer<vtkCellArray> cells = vtkSmartPointer<vtkCellArray>::New();
	cells->InsertNextCell (pyramid);

	vtkOutput->SetPoints(points);
	vtkOutput->InsertNextCell(pyramid->GetCellType(), pyramid->GetPointIds());
}

//----------------------------------------------------------------------------
void VtkWellboreTrajectoryRepresentationDatum::addProperty(const std::string uuidProperty, vtkDataArray* dataProperty)
{
	;
}

long VtkWellboreTrajectoryRepresentationDatum::getAttachmentPropertyCount(const std::string & uuid, const FesppAttachmentProperty propertyUnit)
{
	return 0;
}
