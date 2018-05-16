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
	// delete uuidToVtkProperty
#if (defined(_WIN32) && _MSC_VER >= 1600)
	for (std::unordered_map< std::string, VtkProperty* >::const_iterator it = uuidToVtkProperty.begin(); it != uuidToVtkProperty.end(); ++it)
#else
	for (std::tr1::unordered_map< std::string, VtkProperty* >::const_iterator it = uuidToVtkProperty.begin(); it != uuidToVtkProperty.end(); ++it)
#endif
	{
	  delete it->second;
	}
	uuidToVtkProperty.clear();

	if (epcPackageRepresentation != nullptr) {
		epcPackageRepresentation = nullptr;
	}

	if (epcPackageSubRepresentation != nullptr) {
		epcPackageSubRepresentation = nullptr;
	}

	points = NULL;
}
//----------------------------------------------------------------------------
void VtkAbstractRepresentation::createTreeVtk(const std::string & uuid, const std::string & parent, const std::string & name, const Resqml2Type & resqmlType)
{
	if (subRepresentation){
		uuidToVtkProperty[uuid] = new VtkProperty(getFileName(), name, uuid, parent, epcPackageSubRepresentation);
	}
	else{
		uuidToVtkProperty[uuid] = new VtkProperty(getFileName(), name, uuid, parent, epcPackageRepresentation);
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

	if (localCRS->isDepthOriented())
	{
		zIndice = -1;
	}
	for (auto nodeIndex = 0; nodeIndex < pointCount * 3; nodeIndex += 3)
	{
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
