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
#include "VtkSetPatch.h"

// system
#include <algorithm>
#include <sstream>

// VTK
#include <vtkInformation.h>

// FESAPI
#include <fesapi/resqml2//SubRepresentation.h>
#include <fesapi/resqml2//PolylineSetRepresentation.h>
#include <fesapi/resqml2//TriangulatedSetRepresentation.h>

// FESPP
#include "VtkPolylineRepresentation.h"
#include "VtkTriangulatedRepresentation.h"
#include "VtkProperty.h"

//----------------------------------------------------------------------------
VtkSetPatch::VtkSetPatch(const std::string & fileName, const std::string & name, const std:: string & uuid, const std::string & uuidParent, COMMON_NS::DataObjectRepository *pck, int idProc, int maxProc):
	VtkResqml2MultiBlockDataSet(fileName, name, uuid, uuidParent, idProc, maxProc), epcPackage(pck)
{
	//	uuidIsChildOf[uuid] = new VtkEpcCommon();
	RESQML2_NS::PolylineSetRepresentation* polylineSetRep = nullptr;
	RESQML2_NS::TriangulatedSetRepresentation * triangulatedSetRep = nullptr;
	COMMON_NS::AbstractObject *object = epcPackage->getDataObjectByUuid(uuid);
	std::string xmlTag = object->getXmlTag();
	if (xmlTag == "PolylineSetRepresentation") {
		polylineSetRep = static_cast<RESQML2_NS::PolylineSetRepresentation*>(object);
		if (polylineSetRep == nullptr) {
			cout << "Not found an PolylineSetRepresentation with the right uuid." << endl;
		}
		for (unsigned int patchIndex = 0; patchIndex < polylineSetRep->getPatchCount(); ++patchIndex) {
			std::stringstream sstm;
			sstm << "Patch " << patchIndex;
			uuidToVtkPolylineRepresentation[uuid].push_back(new VtkPolylineRepresentation(fileName, sstm.str(), uuid, uuidParent, patchIndex, epcPackage, nullptr));
		}
		uuidIsChildOf[uuid].setType( VtkEpcCommon::Resqml2Type::POLYLINE_SET);
		uuidIsChildOf[uuid].setUuid( uuid);
	}
	else if (xmlTag == "TriangulatedSetRepresentation") {
		triangulatedSetRep = static_cast<RESQML2_NS::TriangulatedSetRepresentation*>(object);
		if (triangulatedSetRep == nullptr) {
			cout << "Not found an ijk grid with the right uuid." << endl;
		}
		for (unsigned int patchIndex = 0; patchIndex < triangulatedSetRep->getPatchCount(); ++ patchIndex) {
			std::stringstream sstm;
			sstm << "Patch " << patchIndex;
			uuidToVtkTriangulatedRepresentation[uuid].push_back(new VtkTriangulatedRepresentation(fileName, sstm.str(), uuid, uuidParent, patchIndex, epcPackage, nullptr));
		}
		uuidIsChildOf[uuid].setType(VtkEpcCommon::Resqml2Type::TRIANGULATED_SET);
		uuidIsChildOf[uuid].setUuid(uuid);
	}
}

//----------------------------------------------------------------------------
VtkSetPatch::~VtkSetPatch()
{
	if (epcPackage != nullptr) {
		epcPackage = nullptr;
	}

	for(auto i : uuidToVtkPolylineRepresentation) {
		for (const auto& elem : i.second) {
			delete elem;
		}
	}
	uuidToVtkPolylineRepresentation.clear();

	for(auto i : uuidToVtkTriangulatedRepresentation) {
		for (const auto& elem : i.second) {
			delete elem;
		}
	}
	uuidToVtkTriangulatedRepresentation.clear();

	uuidToVtkProperty.clear();
}

//----------------------------------------------------------------------------
void VtkSetPatch::createTreeVtk(const std::string & uuid, const std::string & parent, const std::string & name, VtkEpcCommon::Resqml2Type type)
{
	uuidIsChildOf[uuid].setType( type);
	uuidIsChildOf[uuid].setUuid( uuid);

	RESQML2_NS::PolylineSetRepresentation* polylineSetRep = nullptr;
	RESQML2_NS::TriangulatedSetRepresentation * triangulatedSetRep = nullptr;
	COMMON_NS::AbstractObject* obj = epcPackage->getDataObjectByUuid(getUuid());

	// PROPERTY
	uuidIsChildOf[uuid].setType( uuidIsChildOf[parent].getType());
	uuidIsChildOf[uuid].setUuid( uuidIsChildOf[parent].getUuid());
	if (uuidIsChildOf[parent].getType() == VtkEpcCommon::Resqml2Type::POLYLINE_SET) {
		if ((obj != nullptr && obj->getXmlTag() == "PolylineRepresentation") || (obj != nullptr && obj->getXmlTag() == "PolylineSetRepresentation")) {
			polylineSetRep = static_cast<RESQML2_NS::PolylineSetRepresentation*>(obj);
		}

		for (unsigned int patchIndex = 0; patchIndex < polylineSetRep->getPatchCount(); ++ patchIndex) {
			uuidToVtkPolylineRepresentation[uuidIsChildOf[uuid].getUuid()][patchIndex]->createTreeVtk(uuid, parent, name, type);
		}
	}
	if (uuidIsChildOf[parent].getType() == VtkEpcCommon::Resqml2Type::TRIANGULATED_SET) {
		if (obj != nullptr && obj->getXmlTag() ==  "TriangulatedSetRepresentation") {
			triangulatedSetRep = static_cast<RESQML2_NS::TriangulatedSetRepresentation*>(obj);
		}

		for (unsigned int patchIndex = 0; patchIndex < triangulatedSetRep->getPatchCount(); ++ patchIndex) {
			uuidToVtkTriangulatedRepresentation[uuidIsChildOf[uuid].getUuid()][patchIndex]->createTreeVtk(uuid, parent, name, type);
		}
	}
}

//----------------------------------------------------------------------------
void VtkSetPatch::visualize(const std::string & uuid)
{
	std::vector<VtkPolylineRepresentation *> vectPolyPatch;
	std::vector<VtkTriangulatedRepresentation *> vectTriPatch;
	switch (uuidIsChildOf[uuid].getType()) {
	case VtkEpcCommon::Resqml2Type::POLYLINE_SET: {
		vectPolyPatch = uuidToVtkPolylineRepresentation[uuidIsChildOf[uuid].getUuid()];
		for (size_t patchIndex = 0; patchIndex < vectPolyPatch.size(); ++patchIndex) {
			uuidToVtkPolylineRepresentation[uuidIsChildOf[uuid].getUuid()][patchIndex]->visualize(uuid);
		}
		break;
	}
	case VtkEpcCommon::Resqml2Type::TRIANGULATED_SET: {
		vectTriPatch = uuidToVtkTriangulatedRepresentation[uuidIsChildOf[uuid].getUuid()];
		for (size_t patchIndex = 0; patchIndex < vectTriPatch.size(); ++patchIndex) {
			uuidToVtkTriangulatedRepresentation[uuidIsChildOf[uuid].getUuid()][patchIndex]->visualize(uuid);
		}
		break;
	}
	default:
		break;
	}

	std::string parent = uuidIsChildOf[uuid].getUuid();
	if (std::find(attachUuids.begin(), attachUuids.end(), parent ) == attachUuids.end()) {
		detach();
		attachUuids.push_back(parent);
		attach();
	}
}

//----------------------------------------------------------------------------
void VtkSetPatch::remove(const std::string & uuid)
{
	switch (uuidIsChildOf[uuid].getType()) {
	case VtkEpcCommon::Resqml2Type::POLYLINE_SET: {
		for (size_t patchIndex = 0; patchIndex < uuidToVtkPolylineRepresentation[uuidIsChildOf[uuid].getUuid()].size(); ++patchIndex) {
			uuidToVtkPolylineRepresentation[uuidIsChildOf[uuid].getUuid()][patchIndex]->remove(uuid);
		}
		break;
	}
	case VtkEpcCommon::Resqml2Type::TRIANGULATED_SET: {
		for (size_t patchIndex = 0; patchIndex < uuidToVtkTriangulatedRepresentation[uuidIsChildOf[uuid].getUuid()].size(); ++patchIndex) {
			uuidToVtkTriangulatedRepresentation[uuidIsChildOf[uuid].getUuid()][patchIndex]->remove(uuid);
		}
		break;
	}
	default:
		break;
	}

	if (!(std::find(attachUuids.begin(), attachUuids.end(), uuid ) == attachUuids.end())) {
		detach();
		attachUuids.erase(std::find(attachUuids.begin(), attachUuids.end(), uuid ));
		attach();
	}
}

//----------------------------------------------------------------------------
void VtkSetPatch::attach()
{
	unsigned int indexTmp = 0;

	for (size_t newBlockIndex = 0; newBlockIndex < attachUuids.size(); ++newBlockIndex) {
		std::string uuid = attachUuids[newBlockIndex];
		switch (uuidIsChildOf[uuid].getType()) {
		case VtkEpcCommon::Resqml2Type::POLYLINE_SET: {
			for (size_t patchIndex = 0; patchIndex < uuidToVtkPolylineRepresentation[uuidIsChildOf[uuid].getUuid()].size(); ++patchIndex) {
				vtkOutput->SetBlock(indexTmp, uuidToVtkPolylineRepresentation[uuid][patchIndex]->getOutput());
				vtkOutput->GetMetaData(indexTmp)->Set(vtkCompositeDataSet::NAME(),uuidToVtkPolylineRepresentation[uuid][patchIndex]->getName().c_str());
				++indexTmp;
			}
			break;
		}
		case VtkEpcCommon::Resqml2Type::TRIANGULATED_SET: {
			for (size_t patchIndex = 0; patchIndex < uuidToVtkTriangulatedRepresentation[uuidIsChildOf[uuid].getUuid()].size(); ++patchIndex) {
				vtkOutput->SetBlock(indexTmp, uuidToVtkTriangulatedRepresentation[uuid][patchIndex]->getOutput());
				vtkOutput->GetMetaData(indexTmp)->Set(vtkCompositeDataSet::NAME(),uuidToVtkTriangulatedRepresentation[uuid][patchIndex]->getName().c_str());
				++indexTmp;
			}
			break;
		}
		default:
			break;
		}
	}
}

//----------------------------------------------------------------------------
void VtkSetPatch::addProperty(const std::string & uuidProperty, vtkDataArray* dataProperty)
{
	std::vector<VtkPolylineRepresentation *> vectPolyPatch;
	std::vector<VtkTriangulatedRepresentation *> vectTriPatch;
	switch (uuidIsChildOf[uuidProperty].getType()) {
	case VtkEpcCommon::Resqml2Type::POLYLINE_SET: {
		vectPolyPatch = uuidToVtkPolylineRepresentation[uuidIsChildOf[uuidProperty].getUuid()];
		for (size_t patchIndex = 0; patchIndex < vectPolyPatch.size(); ++patchIndex) {
			uuidToVtkPolylineRepresentation[uuidIsChildOf[uuidProperty].getUuid()][patchIndex]->addProperty(uuidProperty, dataProperty);
		}
		break;
	}
	case VtkEpcCommon::Resqml2Type::TRIANGULATED_SET: {
		vectTriPatch = uuidToVtkTriangulatedRepresentation[uuidIsChildOf[uuidProperty].getUuid()];
		for (size_t patchIndex = 0; patchIndex < vectTriPatch.size(); ++patchIndex) {
			uuidToVtkTriangulatedRepresentation[uuidIsChildOf[uuidProperty].getUuid()][patchIndex]->addProperty(uuidProperty, dataProperty);
		}
		break;
	}
	default:
		break;
	}

	std::string parent = uuidIsChildOf[uuidProperty].getUuid();
	if (std::find(attachUuids.begin(), attachUuids.end(), parent) == attachUuids.end()) {
		detach();
		attachUuids.push_back(parent);
		attach();
	}
}

//----------------------------------------------------------------------------
long VtkSetPatch::getAttachmentPropertyCount(const std::string & uuid, VtkEpcCommon::FesppAttachmentProperty propertyUnit)
{
	long result = 0;
	std::vector<VtkPolylineRepresentation *> vectPolyPatch;
	std::vector<VtkTriangulatedRepresentation *> vectTriPatch;
	switch (uuidIsChildOf[uuid].getType()) {
	case VtkEpcCommon::Resqml2Type::POLYLINE_SET: {
		vectPolyPatch = uuidToVtkPolylineRepresentation[uuidIsChildOf[uuid].getUuid()];
		for (size_t patchIndex = 0; patchIndex < vectPolyPatch.size(); ++patchIndex) {
			result=uuidToVtkPolylineRepresentation[uuidIsChildOf[uuid].getUuid()][patchIndex]->getAttachmentPropertyCount(uuid, propertyUnit);
		}
		break;
	}
	case VtkEpcCommon::Resqml2Type::TRIANGULATED_SET: {
		vectTriPatch = uuidToVtkTriangulatedRepresentation[uuidIsChildOf[uuid].getUuid()];
		for (size_t patchIndex = 0; patchIndex < vectTriPatch.size(); ++patchIndex) {
			result=uuidToVtkTriangulatedRepresentation[uuidIsChildOf[uuid].getUuid()][patchIndex]->getAttachmentPropertyCount(uuid, propertyUnit);
		}
		break;
	}
	default:
		break;
	}
	return result;
}
