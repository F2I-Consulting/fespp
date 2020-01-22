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
#include "VtkWellboreTrajectoryRepresentationDatum.h"

// VTK
#include <vtkSmartPointer.h>
#include <vtkCellArray.h>
#include <vtkPyramid.h>
#include <vtkPoints.h>

// FESAPI
#include <fesapi/common/EpcDocument.h>
#include <fesapi/resqml2_0_1/WellboreTrajectoryRepresentation.h>
#include <fesapi/resqml2/MdDatum.h>

//----------------------------------------------------------------------------
VtkWellboreTrajectoryRepresentationDatum::VtkWellboreTrajectoryRepresentationDatum(const std::string & fileName, const std::string & name, const std::string & uuid, const std::string & uuidParent, COMMON_NS::DataObjectRepository *repoRepresentation, COMMON_NS::DataObjectRepository *repoSubRepresentation) :
VtkResqml2UnstructuredGrid(fileName, name, uuid, uuidParent, repoRepresentation, repoSubRepresentation)
{
}

//----------------------------------------------------------------------------
void VtkWellboreTrajectoryRepresentationDatum::createOutput(const std::string & uuid)
{
	vtkOutput = vtkSmartPointer<vtkUnstructuredGrid>::New();
	resqml2_0_1::WellboreTrajectoryRepresentation* wellboreSetRepresentation = nullptr;
	common::AbstractObject* obj = epcPackageRepresentation->getDataObjectByUuid(uuid);
	if (obj != nullptr && obj->getXmlTag() ==  "WellboreTrajectoryRepresentation") {
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
void VtkWellboreTrajectoryRepresentationDatum::addProperty(const std::string & uuidProperty, vtkDataArray* dataProperty)
{
	;
}

long VtkWellboreTrajectoryRepresentationDatum::getAttachmentPropertyCount(const std::string & uuid, VtkEpcCommon::FesppAttachmentProperty propertyUnit)
{
	return 0;
}
