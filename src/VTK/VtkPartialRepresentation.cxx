/*-----------------------------------------------------------------------
Licensed to the Apache Software Foundation (ASF) under one
or more contributor license agreements.  See the NOTICE file
distributed with this work for additional information
regarding copyright ownership.  The ASF licenses this file
to you under the Apache License, Version 2.0 (the
"License"; you may not use this file except in compliance
with the License.  You may obtain a copy of the License at

  http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing,
software distributed under the License is distributed on an
"AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
KIND, either express or implied.  See the License for the
specific language governing permissions and limitations
under the License.
-----------------------------------------------------------------------*/
#include "VtkPartialRepresentation.h"

// FESPP
#include "VtkEpcDocument.h"
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

			auto cellCount = vtkEpcDocumentSource->getAttachmentPropertyCount(vtkPartialReprUuid, VtkEpcCommon::FesppAttachmentProperty::CELLS);
			auto pointCount = vtkEpcDocumentSource->getAttachmentPropertyCount(vtkPartialReprUuid, VtkEpcCommon::FesppAttachmentProperty::POINTS);
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
	if (resqmlType == VtkEpcCommon::Resqml2Type::PROPERTY ) {
		uuidToVtkProperty[uuid] = new VtkProperty(fileName, name, uuid, parent, repository);
	}
}

//----------------------------------------------------------------------------
long VtkPartialRepresentation::getAttachmentPropertyCount(const std::string & uuid, VtkEpcCommon::FesppAttachmentProperty propertyUnit)
{
	return vtkEpcDocumentSource->getAttachmentPropertyCount(uuid, propertyUnit);
}

//----------------------------------------------------------------------------
void VtkPartialRepresentation::remove(const std::string &)
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
const COMMON_NS::DataObjectRepository& VtkPartialRepresentation::getEpcSource() const
{
	return vtkEpcDocumentSource->getDataObjectRepository();
}
