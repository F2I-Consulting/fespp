#include "VtkPartialRepresentation.h"

#include"VtkEpcDocument.h"
#include "VtkProperty.h"

#include <vtkDataArray.h>

// include F2i-consulting Energistics Standards API
#include <resqml2_0_1/PolylineSetRepresentation.h>
#include <resqml2_0_1/Grid2dRepresentation.h>
#include <resqml2_0_1/TriangulatedSetRepresentation.h>
#include <resqml2_0_1/UnstructuredGridRepresentation.h>
#include <resqml2_0_1/WellboreTrajectoryRepresentation.h>
#include <resqml2_0_1/AbstractIjkGridRepresentation.h>
#include <common/AbstractObject.h>

//----------------------------------------------------------------------------
VtkPartialRepresentation::VtkPartialRepresentation(const std::string & fileName, const std::string & uuid, VtkEpcDocument *vtkEpcDowumentWithCompleteRep, common::EpcDocument *pck) :
epcPackage(pck), vtkEpcDocumentSource(vtkEpcDowumentWithCompleteRep), vtkPartialReprUuid(uuid), fileName(fileName)
{
}

//----------------------------------------------------------------------------
void VtkPartialRepresentation::visualize(const std::string & uuid)
{
	if (uuid != vtkPartialReprUuid)
	{
		common::AbstractObject* obj = epcPackage->getResqmlAbstractObjectByUuid(vtkPartialReprUuid);
		if (obj != nullptr){
			long cellCount = 0;
			long pointCount = 0;
			std::vector<resqml2::AbstractValuesProperty*> valuesPropertySet;

			if (obj->getXmlTag() == "PolylineSetRepresentation") {
				resqml2_0_1::PolylineSetRepresentation* polylineSetRepresentation = nullptr;
				polylineSetRepresentation = static_cast<resqml2_0_1::PolylineSetRepresentation*>(obj);
				valuesPropertySet = polylineSetRepresentation->getValuesPropertySet();
				pointCount = vtkEpcDocumentSource->getAttachmentPropertyCount(vtkPartialReprUuid, VtkAbstractObject::FesppAttachmentProperty::POINTS);
			}
			else {
				if (obj->getXmlTag() == "IjkGridRepresentation" || obj->getXmlTag() == "TruncatedIjkGridRepresentation") {
					resqml2_0_1::AbstractIjkGridRepresentation* ijkGridRepresentation = nullptr;
					ijkGridRepresentation = static_cast<resqml2_0_1::AbstractIjkGridRepresentation*>(obj);
					valuesPropertySet = ijkGridRepresentation->getValuesPropertySet();
					cellCount = vtkEpcDocumentSource->getAttachmentPropertyCount(vtkPartialReprUuid, VtkAbstractObject::FesppAttachmentProperty::CELLS);
				}			
				else {
					if (obj->getXmlTag() == "TriangulatedSetRepresentation") {
						resqml2_0_1::TriangulatedSetRepresentation* triangulatedSetRepresentation = nullptr;
						triangulatedSetRepresentation = static_cast<resqml2_0_1::TriangulatedSetRepresentation*>(obj);
						valuesPropertySet = triangulatedSetRepresentation->getValuesPropertySet();
						pointCount = vtkEpcDocumentSource->getAttachmentPropertyCount(vtkPartialReprUuid, VtkAbstractObject::FesppAttachmentProperty::POINTS);
					}
					else {
						if (obj->getXmlTag() == "UnstructuredGridRepresentation") {
							resqml2_0_1::UnstructuredGridRepresentation* unstructuredGridRep = nullptr;
							unstructuredGridRep = static_cast<resqml2_0_1::UnstructuredGridRepresentation*>(obj);
							valuesPropertySet = unstructuredGridRep->getValuesPropertySet();
							pointCount = vtkEpcDocumentSource->getAttachmentPropertyCount(vtkPartialReprUuid, VtkAbstractObject::FesppAttachmentProperty::POINTS);
							cellCount = vtkEpcDocumentSource->getAttachmentPropertyCount(vtkPartialReprUuid, VtkAbstractObject::FesppAttachmentProperty::CELLS);
						}
						else {
							if (obj->getXmlTag() == "Grid2dRepresentation") {
								resqml2_0_1::Grid2dRepresentation* grid2dRepresentation = nullptr;
								grid2dRepresentation = static_cast<resqml2_0_1::Grid2dRepresentation*>(obj);
								valuesPropertySet = grid2dRepresentation->getValuesPropertySet();
								pointCount = vtkEpcDocumentSource->getAttachmentPropertyCount(vtkPartialReprUuid, VtkAbstractObject::FesppAttachmentProperty::POINTS);
							}
							else {
								if (obj->getXmlTag() == "WellboreTrajectoryRepresentation") {
									resqml2_0_1::WellboreTrajectoryRepresentation* wellboreTrajectoryRepresentation = nullptr;
									wellboreTrajectoryRepresentation = static_cast<resqml2_0_1::WellboreTrajectoryRepresentation*>(obj);
									valuesPropertySet = wellboreTrajectoryRepresentation->getValuesPropertySet();
									pointCount = vtkEpcDocumentSource->getAttachmentPropertyCount(vtkPartialReprUuid, VtkAbstractObject::FesppAttachmentProperty::POINTS);
								}
							}
						}
					}
				}
			}
			vtkEpcDocumentSource->addProperty(vtkPartialReprUuid, uuidToVtkProperty[uuid]->loadValuesPropertySet(valuesPropertySet, cellCount, pointCount));
		}
	}

}
	
//----------------------------------------------------------------------------
void VtkPartialRepresentation::createTreeVtk(const std::string & uuid, const std::string & parent, const std::string & name, const VtkAbstractObject::Resqml2Type & resqmlType)
{
	if (resqmlType == VtkAbstractObject::Resqml2Type::SUB_REP )
	{
		;
	}
	else{
		uuidToVtkProperty[uuid] = new VtkProperty(fileName, name, uuid, parent, epcPackage);
	}
}

//----------------------------------------------------------------------------
void VtkPartialRepresentation::remove(const std::string & uuid)
{
}

//----------------------------------------------------------------------------
VtkAbstractObject::Resqml2Type VtkPartialRepresentation::getType()
{
	return vtkEpcDocumentSource->getType(vtkPartialReprUuid);
}

//----------------------------------------------------------------------------
common::EpcDocument * VtkPartialRepresentation::getEpcSource()
{
	return vtkEpcDocumentSource->getEpcDocument();
}
