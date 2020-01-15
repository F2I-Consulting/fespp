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
#include "VtkWellboreTrajectoryRepresentationPolyLine.h"

// VTK
#include <vtkPointData.h>
#include <vtkCellArray.h>
#include <vtkPolyLine.h>
#include <vtkSmartPointer.h>
#include <vtkDataArray.h>

// FESAPI
#include <fesapi/common/EpcDocument.h>
#include <fesapi/resqml2_0_1/WellboreTrajectoryRepresentation.h>

// FESPP
#include "VtkProperty.h"

//----------------------------------------------------------------------------
VtkWellboreTrajectoryRepresentationPolyLine::VtkWellboreTrajectoryRepresentationPolyLine(const std::string & fileName, const std::string & name, const std::string & uuid, const std::string & uuidParent, COMMON_NS::DataObjectRepository *repoRepresentation, COMMON_NS::DataObjectRepository *repoSubRepresentation) :
VtkResqml2PolyData(fileName, name, uuid, uuidParent, repoRepresentation, repoSubRepresentation)
{
}

//----------------------------------------------------------------------------
void VtkWellboreTrajectoryRepresentationPolyLine::createOutput(const std::string & uuid)
{
	if (!subRepresentation)	{

		resqml2_0_1::WellboreTrajectoryRepresentation* wellboreSetRepresentation = nullptr;
		common::AbstractObject* obj = epcPackageRepresentation->getDataObjectByUuid(getUuid().substr(0, 36));
		if (obj != nullptr && obj->getXmlTag() == "WellboreTrajectoryRepresentation") {
			wellboreSetRepresentation = static_cast<resqml2_0_1::WellboreTrajectoryRepresentation*>(obj);
		}

		if (!vtkOutput) {
			vtkOutput = vtkSmartPointer<vtkPolyData>::New();

			// POINT
			unsigned int pointCount = wellboreSetRepresentation->getXyzPointCountOfPatch(0);
			double * allXyzPoints = new double[pointCount * 3];
			wellboreSetRepresentation->getXyzPointsOfAllPatchesInGlobalCrs(allXyzPoints);
			createVtkPoints(pointCount, allXyzPoints, wellboreSetRepresentation->getLocalCrs(0));
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
		else {
			if (uuid != getUuid().substr(0, 36)) {
				vtkDataArray* arrayProperty = uuidToVtkProperty[uuid]->visualize(uuid, wellboreSetRepresentation);
				addProperty(uuid, arrayProperty);
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
void VtkWellboreTrajectoryRepresentationPolyLine::addProperty(const std::string & uuidProperty, vtkDataArray* dataProperty)
{
	vtkOutput->Modified();
	vtkOutput->GetPointData()->AddArray(dataProperty);
	lastProperty = uuidProperty;
}

long VtkWellboreTrajectoryRepresentationPolyLine::getAttachmentPropertyCount(const std::string & uuid, const VtkEpcCommon::FesppAttachmentProperty propertyUnit)
{
	long result = 0;
	resqml2_0_1::WellboreTrajectoryRepresentation* wellboreSetRepresentation = nullptr;
	common::AbstractObject* obj = epcPackageRepresentation->getDataObjectByUuid(getUuid().substr(0, 36));
	if (obj != nullptr && obj->getXmlTag() == "WellboreTrajectoryRepresentation") {
		wellboreSetRepresentation = static_cast<resqml2_0_1::WellboreTrajectoryRepresentation*>(obj);
		result = wellboreSetRepresentation->getXyzPointCountOfAllPatches();
	}
	return result;
}
