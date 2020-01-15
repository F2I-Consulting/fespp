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

#include "VtkPolylineRepresentation.h"

// include VTK library
#include <vtkPointData.h>
#include <vtkCellArray.h>
#include <vtkPolyLine.h>
#include <vtkSmartPointer.h>

// include F2i-consulting Energistics Standards API
#include <fesapi/resqml2_0_1/PolylineSetRepresentation.h>
#include <fesapi/common/EpcDocument.h>

// include F2i-consulting Energistics Standards ParaView Plugin
#include "VtkProperty.h"

//----------------------------------------------------------------------------
VtkPolylineRepresentation::VtkPolylineRepresentation(const std::string & fileName, const std::string & name, const std::string & uuid, const std::string & uuidParent, const unsigned int & patchNo, COMMON_NS::DataObjectRepository *repoRepresentation, COMMON_NS::DataObjectRepository *repoSubRepresentation) :
VtkResqml2PolyData(fileName, name, uuid, uuidParent, repoRepresentation, repoSubRepresentation), patchIndex(patchNo)
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
		common::AbstractObject* obj = epcPackageRepresentation->getDataObjectByUuid(getUuid());
		if (obj != nullptr && obj->getXmlTag() == "PolylineSetRepresentation") {
			polylineSetRepresentation = static_cast<resqml2_0_1::PolylineSetRepresentation*>(obj);
		}

		if (!vtkOutput) {
			VtkResqml2PolyData::vtkOutput = vtkSmartPointer<vtkPolyData>::New();

			// POINT
			unsigned int nodeCount = polylineSetRepresentation->getXyzPointCountOfPatch(patchIndex);

			double * allPoint = new double[nodeCount * 3];
			polylineSetRepresentation->getXyzPointsOfPatch(patchIndex, allPoint);

			createVtkPoints(nodeCount, allPoint, polylineSetRepresentation->getLocalCrs(0));
			vtkOutput->SetPoints(points);

			delete[] allPoint;

			// POLYLINE
			vtkSmartPointer<vtkCellArray> setPolylineRepresentationLines = vtkSmartPointer<vtkCellArray>::New();

			unsigned int countPolyline = polylineSetRepresentation->getPolylineCountOfPatch(patchIndex);

			unsigned int* countNodePolylineInPatch = new unsigned int[countPolyline];
			polylineSetRepresentation->getNodeCountPerPolylineInPatch(patchIndex, countNodePolylineInPatch);

			unsigned int idPoint = 0;
			for (unsigned int polylineIndex = 0; polylineIndex < countPolyline; ++polylineIndex) {
				vtkSmartPointer<vtkPolyLine> polylineRepresentation = vtkSmartPointer<vtkPolyLine>::New();
				polylineRepresentation->GetPointIds()->SetNumberOfIds(countNodePolylineInPatch[polylineIndex]);
				for (unsigned int line = 0; line < countNodePolylineInPatch[polylineIndex]; ++line) {
					polylineRepresentation->GetPointIds()->SetId(line, idPoint);
					idPoint++;

				}
				setPolylineRepresentationLines->InsertNextCell(polylineRepresentation);
				vtkOutput->SetLines(setPolylineRepresentationLines);
			}
			points = nullptr;
			delete[] countNodePolylineInPatch;
		}
		else {
			if (uuid != getUuid()) {
				vtkDataArray* arrayProperty = uuidToVtkProperty[uuid]->visualize(uuid, polylineSetRepresentation);
				addProperty(uuid, arrayProperty);
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

//----------------------------------------------------------------------------
long VtkPolylineRepresentation::getAttachmentPropertyCount(const std::string & uuid, const VtkEpcCommon::FesppAttachmentProperty propertyUnit)
{
	long result = 0;
	resqml2_0_1::PolylineSetRepresentation* polylineSetRepresentation = nullptr;
	common::AbstractObject* obj = epcPackageRepresentation->getDataObjectByUuid(getUuid());
	if (obj != nullptr && obj->getXmlTag() == "PolylineSetRepresentation") {
		polylineSetRepresentation = static_cast<resqml2_0_1::PolylineSetRepresentation*>(obj);
		result = polylineSetRepresentation->getXyzPointCountOfPatch(0);
	}
	return result;
}
