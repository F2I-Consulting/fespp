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
#include "VtkAbstractRepresentation.h"
#include "VtkProperty.h"

// FESAPI
#include <common/EpcDocument.h>

#include <vtkDataArray.h>

//----------------------------------------------------------------------------
VtkAbstractRepresentation::VtkAbstractRepresentation(const std::string & fileName, const std::string & name, const std::string & uuid, const std::string & uuidParent, common::EpcDocument *pckEPCRep, common::EpcDocument *pckEPCSubRep, const int & idProc, const int & maxProc) :
				VtkAbstractObject(fileName, name, uuid, uuidParent, idProc, maxProc), epcPackageRepresentation(pckEPCRep), epcPackageSubRepresentation(pckEPCSubRep)
{
	if (!pckEPCSubRep){
		subRepresentation = false;
	}
	else{
		subRepresentation = true;
	}
}

VtkAbstractRepresentation::~VtkAbstractRepresentation()
{
	if (epcPackageRepresentation != nullptr) {
		epcPackageRepresentation = nullptr;
	}

	if (epcPackageSubRepresentation != nullptr) {
		epcPackageSubRepresentation = nullptr;
	}

	for(auto i : uuidToVtkProperty) {
		delete i.second;
	}
	uuidToVtkProperty.clear();

	points = NULL;
}
//----------------------------------------------------------------------------
void VtkAbstractRepresentation::createTreeVtk(const std::string & uuid, const std::string & parent, const std::string & name, const VtkEpcCommon::Resqml2Type & resqmlType)
{
	if (resqmlType==VtkEpcCommon::PROPERTY)	{
		if (uuidToVtkProperty.find(uuid) == uuidToVtkProperty.end()) {
			uuidToVtkProperty[uuid] = new VtkProperty(getFileName(), name, uuid, parent, subRepresentation ? epcPackageSubRepresentation : epcPackageRepresentation);
		}
	}
}

//----------------------------------------------------------------------------
void VtkAbstractRepresentation::visualize(const std::string & uuid)
{
	this->createOutput(uuid);
}

vtkSmartPointer<vtkPoints> VtkAbstractRepresentation::createVtkPoints(const ULONG64 & pointCount, const double * allXyzPoints, const resqml2::AbstractLocal3dCrs * localCRS)
{
	points = vtkSmartPointer<vtkPoints>::New();

	double zIndice = 1;

	if (localCRS->isDepthOriented()) {

		zIndice = -1;
	}

	for (ULONG64 nodeIndex = 0; nodeIndex < pointCount * 3; nodeIndex += 3) {
		points->InsertNextPoint(allXyzPoints[nodeIndex], allXyzPoints[nodeIndex + 1], allXyzPoints[nodeIndex + 2] * zIndice);
	}
	return points;
}

vtkSmartPointer<vtkPoints> VtkAbstractRepresentation::getVtkPoints()
{
	return points;
}

bool VtkAbstractRepresentation::vtkPointsIsCreated()
{
	return (points != nullptr);
}
