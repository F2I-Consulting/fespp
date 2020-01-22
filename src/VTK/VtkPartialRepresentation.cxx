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
#include "VtkPartialRepresentation.h"

// FESPP
#include"VtkEpcDocument.h"
#include "VtkProperty.h"

// VTK
#include <vtkDataArray.h>

// FESAPI
#include <fesapi/resqml2_0_1/PolylineSetRepresentation.h>
#include <fesapi/resqml2_0_1/Grid2dRepresentation.h>
#include <fesapi/resqml2_0_1/TriangulatedSetRepresentation.h>
#include <fesapi/resqml2_0_1/UnstructuredGridRepresentation.h>
#include <fesapi/resqml2_0_1/WellboreTrajectoryRepresentation.h>
#include <fesapi/resqml2_0_1/AbstractIjkGridRepresentation.h>
#include <fesapi/resqml2_0_1/SubRepresentation.h>
#include <fesapi/common/AbstractObject.h>

//----------------------------------------------------------------------------
VtkPartialRepresentation::VtkPartialRepresentation(const std::string & fileName, const std::string & uuid, VtkEpcDocument *vtkEpcDowumentWithCompleteRep, COMMON_NS::DataObjectRepository *repo) :
repository(repo), vtkEpcDocumentSource(vtkEpcDowumentWithCompleteRep), vtkPartialReprUuid(uuid), fileName(fileName)
{
}

VtkPartialRepresentation::~VtkPartialRepresentation()
{
	for(auto i : uuidToVtkProperty) {
		delete i.second;
	}
	uuidToVtkProperty.clear();

	if (repository != nullptr) {
		repository = nullptr;
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
		COMMON_NS::AbstractObject* obj = repository->getDataObjectByUuid(vtkPartialReprUuid);
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
void VtkPartialRepresentation::createTreeVtk(const std::string & uuid, const std::string & parent, const std::string & name, VtkEpcCommon::Resqml2Type resqmlType)
{
	if (resqmlType == VtkEpcCommon::PROPERTY ) {
		uuidToVtkProperty[uuid] = new VtkProperty(fileName, name, uuid, parent, repository);
	}
}

//----------------------------------------------------------------------------
long VtkPartialRepresentation::getAttachmentPropertyCount(const std::string & uuid, VtkEpcCommon::FesppAttachmentProperty propertyUnit)
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
VtkEpcCommon VtkPartialRepresentation::getInfoUuid()
{
	return vtkEpcDocumentSource->getInfoUuid(vtkPartialReprUuid);
}

//----------------------------------------------------------------------------
COMMON_NS::DataObjectRepository * VtkPartialRepresentation::getEpcSource()
{
	return vtkEpcDocumentSource->getEpcDocument();
}
