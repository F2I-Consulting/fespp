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
#include <resqml2_0_1/SubRepresentation.h>
#include <common/AbstractObject.h>

//----------------------------------------------------------------------------
VtkPartialRepresentation::VtkPartialRepresentation(const std::string & fileName, const std::string & uuid, VtkEpcDocument *vtkEpcDowumentWithCompleteRep, common::EpcDocument *pck) :
epcPackage(pck), vtkEpcDocumentSource(vtkEpcDowumentWithCompleteRep), vtkPartialReprUuid(uuid), fileName(fileName)
{
}

VtkPartialRepresentation::~VtkPartialRepresentation()
{
	uuidToVtkProperty.clear();

	if (epcPackage != nullptr) {
		epcPackage = nullptr;
	}

	if (vtkEpcDocumentSource != nullptr) {
		vtkEpcDocumentSource = nullptr;
	}

	vtkPartialReprUuid = "";
	fileName = "";
}
//----------------------------------------------------------------------------
void VtkPartialRepresentation::visualize(const std::string & uuid)
{
	if (uuid != vtkPartialReprUuid)
	{
		common::AbstractObject* obj = epcPackage->getResqmlAbstractObjectByUuid(vtkPartialReprUuid);
		if (obj != nullptr){
			std::vector<resqml2::AbstractValuesProperty*> valuesPropertySet;

			if (obj->getXmlTag() == "PolylineSetRepresentation") {
				auto polylineSetRepresentation = static_cast<resqml2_0_1::PolylineSetRepresentation*>(obj);
				valuesPropertySet = polylineSetRepresentation->getValuesPropertySet();
			}
			else {
				if (obj->getXmlTag() == "IjkGridRepresentation" || obj->getXmlTag() == "TruncatedIjkGridRepresentation") {
					auto ijkGridRepresentation = static_cast<resqml2_0_1::AbstractIjkGridRepresentation*>(obj);
					valuesPropertySet = ijkGridRepresentation->getValuesPropertySet();
				}			
				else {
					if (obj->getXmlTag() == "TriangulatedSetRepresentation") {
						auto triangulatedSetRepresentation = static_cast<resqml2_0_1::TriangulatedSetRepresentation*>(obj);
						valuesPropertySet = triangulatedSetRepresentation->getValuesPropertySet();
					}
					else {
						if (obj->getXmlTag() == "UnstructuredGridRepresentation") {
							auto unstructuredGridRep = static_cast<resqml2_0_1::UnstructuredGridRepresentation*>(obj);
							valuesPropertySet = unstructuredGridRep->getValuesPropertySet();
						}
						else {
							if (obj->getXmlTag() == "Grid2dRepresentation") {
								auto grid2dRepresentation = static_cast<resqml2_0_1::Grid2dRepresentation*>(obj);
								valuesPropertySet = grid2dRepresentation->getValuesPropertySet();
							}
							else {
								if (obj->getXmlTag() == "WellboreTrajectoryRepresentation") {
									auto wellboreTrajectoryRepresentation = static_cast<resqml2_0_1::WellboreTrajectoryRepresentation*>(obj);
									valuesPropertySet = wellboreTrajectoryRepresentation->getValuesPropertySet();
								}
								else {
									if (obj->getXmlTag() == "SubRepresentation") {
										auto subRepresentation = static_cast<resqml2_0_1::SubRepresentation*>(obj);
										valuesPropertySet = subRepresentation->getValuesPropertySet();
									}
								}
							}
						}
					}
				}
			}

			auto cellCount = vtkEpcDocumentSource->getAttachmentPropertyCount(vtkPartialReprUuid, VtkEpcCommon::CELLS);
			auto pointCount = vtkEpcDocumentSource->getAttachmentPropertyCount(vtkPartialReprUuid, VtkEpcCommon::POINTS);
			auto iCellCount = vtkEpcDocumentSource->getICellCount(vtkPartialReprUuid);
			auto jCellCount = vtkEpcDocumentSource->getJCellCount(vtkPartialReprUuid);
			auto kCellCount = vtkEpcDocumentSource->getKCellCount(vtkPartialReprUuid);
			auto initKIndex = vtkEpcDocumentSource->getInitKIndex(vtkPartialReprUuid);

			auto arrayProperty = uuidToVtkProperty[uuid]->loadValuesPropertySet(valuesPropertySet, cellCount, pointCount, iCellCount, jCellCount, kCellCount, initKIndex);
//			auto arrayProperty = uuidToVtkProperty[uuid]->loadValuesPropertySet(valuesPropertySet, cellCount, pointCount);

			vtkEpcDocumentSource->addProperty(vtkPartialReprUuid, arrayProperty);
		}
	}

}
	
//----------------------------------------------------------------------------
void VtkPartialRepresentation::createTreeVtk(const std::string & uuid, const std::string & parent, const std::string & name, const VtkEpcCommon::Resqml2Type & resqmlType)
{
	if (resqmlType == VtkEpcCommon::PROPERTY )
	{
		uuidToVtkProperty[uuid] = new VtkProperty(fileName, name, uuid, parent, epcPackage);
	}
}

//----------------------------------------------------------------------------
long VtkPartialRepresentation::getAttachmentPropertyCount(const std::string & uuid, const VtkEpcCommon::FesppAttachmentProperty propertyUnit)
{
	return vtkEpcDocumentSource->getAttachmentPropertyCount(uuid, propertyUnit);
}
//----------------------------------------------------------------------------
void VtkPartialRepresentation::remove(const std::string & uuid)
{
}

//----------------------------------------------------------------------------
VtkEpcCommon::Resqml2Type VtkPartialRepresentation::getType()
{
	return vtkEpcDocumentSource->getType(vtkPartialReprUuid);
}

//----------------------------------------------------------------------------
VtkEpcCommon* VtkPartialRepresentation::getInfoUuid()
{
	return vtkEpcDocumentSource->getInfoUuid(vtkPartialReprUuid);
}

//----------------------------------------------------------------------------
common::EpcDocument * VtkPartialRepresentation::getEpcSource()
{
	return vtkEpcDocumentSource->getEpcDocument();
}
