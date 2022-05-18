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
#include "VtkEpcDocument.h"

// SYSTEM
#include <algorithm>
#include <sstream>

// VTK
#include <vtkInformation.h>

// FESAPI
#include <fesapi/common/AbstractObject.h>
#include <fesapi/common/EpcDocument.h>
#include <fesapi/eml2/TimeSeries.h>
#include <fesapi/resqml2/AbstractValuesProperty.h>
#include <fesapi/resqml2/PolylineSetRepresentation.h>
#include <fesapi/resqml2/TriangulatedSetRepresentation.h>
#include <fesapi/resqml2/HorizonInterpretation.h>
#include <fesapi/resqml2/FaultInterpretation.h>
#include <fesapi/resqml2/PointSetRepresentation.h>
#include <fesapi/resqml2/Grid2dRepresentation.h>
#include <fesapi/resqml2/SubRepresentation.h>
#include <fesapi/resqml2/AbstractIjkGridRepresentation.h>
#include <fesapi/resqml2/UnstructuredGridRepresentation.h>
#include <fesapi/resqml2/WellboreMarker.h>
#include <fesapi/resqml2/WellboreMarkerFrameRepresentation.h>
#include <fesapi/resqml2/WellboreTrajectoryRepresentation.h>

// FESPP
#include "VtkEpcDocumentSet.h"
#include "VtkGrid2DRepresentation.h"
#include "VtkPolylineRepresentation.h"
#include "VtkTriangulatedRepresentation.h"
#include "VtkWellboreTrajectoryRepresentation.h"
#include "VtkUnstructuredGridRepresentation.h"
#include "VtkIjkGridRepresentation.h"
#include "VtkPartialRepresentation.h"
#include "VtkSetPatch.h"
#include "VtkEpcCommon.h"

#include <ctime>

// ----------------------------------------------------------------------------
VtkEpcDocument::VtkEpcDocument(const std::string & fileName, int idProc, int maxProc, VtkEpcDocumentSet* epcDocSet) :
VtkResqml2MultiBlockDataSet(fileName, fileName, fileName, "", idProc, maxProc), epcSet(epcDocSet)
{
	COMMON_NS::EpcDocument pck(fileName);
	repository = new common::DataObjectRepository();
	const std::string resqmlResult = pck.deserializeInto(*repository);
	pck.close();
	if (!resqmlResult.empty()) {
		epc_error = epc_error + resqmlResult;
	}
	// polylines
	searchFaultPolylines(fileName);
	searchHorizonPolylines(fileName);
	//unstructuredGrid
	searchUnstructuredGrid(fileName);
	// triangulated
	searchTriangulated(fileName);
	// grid2D
	searchGrid2d(fileName);
	// ijkGrid
	searchIjkGrid(fileName);
	// WellboreTrajectory
	searchWellboreTrajectory(fileName);
	// subRepresentation
	searchSubRepresentation(fileName);
	// TimeSeries
	searchTimeSeries(fileName);
}

// ----------------------------------------------------------------------------
VtkEpcDocument::~VtkEpcDocument()
{
	for(auto i : uuidToVtkGrid2DRepresentation) {
		delete i.second;
	}

	for(auto i : uuidToVtkPolylineRepresentation) {
		delete i.second;
	}

	for(auto i : uuidToVtkTriangulatedRepresentation) {
		delete i.second;
	}

	for(auto i : uuidToVtkSetPatch) {
		delete i.second;
	}

	for(auto i : uuidToVtkWellboreTrajectoryRepresentation) {
		delete i.second;
	}

	for(auto i : uuidToVtkIjkGridRepresentation) {
		delete i.second;
	}

	for(auto i : uuidToVtkUnstructuredGridRepresentation) {
		delete i.second;
	}

	for(auto i : uuidToVtkPartialRepresentation) {
		delete i.second;
	}

	delete repository;
}

// ----------------------------------------------------------------------------
void VtkEpcDocument::createTreeVtk(const std::string & uuid, const std::string & parent, const std::string & name, VtkEpcCommon::Resqml2Type type)
{
	int return_code = 1;
	uuidIsChildOf[uuid].setType(type);
	uuidIsChildOf[uuid].setUuid(uuid);
	uuidIsChildOf[uuid].setParent(parent);
	uuidIsChildOf[uuid].setName(name);
	uuidIsChildOf[uuid].setTimeIndex(-1);
	uuidIsChildOf[uuid].setTimestamp(0);

	if(uuidIsChildOf[parent].getUuid().empty()) {
		if (type == VtkEpcCommon::Resqml2Type::GRID_2D ||
				type == VtkEpcCommon::Resqml2Type::POLYLINE_SET ||
				type == VtkEpcCommon::Resqml2Type::TRIANGULATED_SET) {
			uuidIsChildOf[uuid].setParentType(VtkEpcCommon::Resqml2Type::INTERPRETATION_2D);
		}
		else if (type == VtkEpcCommon::Resqml2Type::WELL_TRAJ) {
			uuidIsChildOf[uuid].setParentType(VtkEpcCommon::Resqml2Type::INTERPRETATION_1D);
		}
		else if (type == VtkEpcCommon::Resqml2Type::IJK_GRID ||
				type == VtkEpcCommon::Resqml2Type::UNSTRUC_GRID ||
				type == VtkEpcCommon::Resqml2Type::SUB_REP) {
			uuidIsChildOf[uuid].setParentType(VtkEpcCommon::Resqml2Type::INTERPRETATION_3D);
		}
	}
	else {
		uuidIsChildOf[uuid].setParentType( uuidIsChildOf[parent].getType());
	}

	switch (type) {
	case VtkEpcCommon::Resqml2Type::GRID_2D: {
		addGrid2DTreeVtk(uuid, parent, name);
		break;
	}
	case VtkEpcCommon::Resqml2Type::POLYLINE_SET: {
		addPolylineSetTreeVtk(uuid, parent, name);
		break;
	}
	case VtkEpcCommon::Resqml2Type::TRIANGULATED_SET: {
		addTriangulatedSetTreeVtk(uuid, parent, name);
		break;
	}
	case VtkEpcCommon::Resqml2Type::WELL_TRAJ: {
		addWellTrajTreeVtk(uuid, parent, name);
		break;
	}
	case VtkEpcCommon::Resqml2Type::WELL_FRAME:
	case VtkEpcCommon::Resqml2Type::WELL_MARKER_FRAME: {
		addWellFrameTreeVtk(uuid, parent, name);
		break;
	}
	case VtkEpcCommon::Resqml2Type::WELL_MARKER: {
		addWellMarkerTreeVtk(uuid, parent, name);
		break;
	}
	case VtkEpcCommon::Resqml2Type::IJK_GRID: {
		addIjkGridTreeVtk(uuid, parent, name);
		break;
	}
	case VtkEpcCommon::Resqml2Type::UNSTRUC_GRID: {
		addUnstrucGridTreeVtk(uuid, parent, name);
		break;
	}
	case VtkEpcCommon::Resqml2Type::SUB_REP: {
		return_code = addSubRepTreeVtk(uuid, parent, name);
		break;
	}
	case VtkEpcCommon::Resqml2Type::PROPERTY: {
		return_code = addPropertyTreeVtk(uuid, parent, name);
	}
	default:
		break;
	}

	if (return_code != 0){
		uuidRep.push_back(uuid);
	}
}

// ----------------------------------------------------------------------------
void VtkEpcDocument::addGrid2DTreeVtk(const std::string & uuid, const std::string & parent, const std::string & name)
{
	uuidToVtkGrid2DRepresentation[uuid] = new VtkGrid2DRepresentation(getFileName(), name, uuid, parent, repository, nullptr);
}

// ----------------------------------------------------------------------------
void VtkEpcDocument::addPolylineSetTreeVtk(const std::string & uuid, const std::string & parent, const std::string & name)
{
	if (repository->getDataObjectByUuid(uuid)->getXmlTag() == "PolylineRepresentation") {
		uuidToVtkPolylineRepresentation[uuid] = new VtkPolylineRepresentation(getFileName(), name, uuid, parent, 0, repository, nullptr);
	}
	else {
		uuidToVtkSetPatch[uuid] = new VtkSetPatch(getFileName(), name, uuid, parent, repository, getIdProc(), getMaxProc());
	}
}

// ----------------------------------------------------------------------------
void VtkEpcDocument::addTriangulatedSetTreeVtk(const std::string & uuid, const std::string & parent, const std::string & name)
{
	if (repository->getDataObjectByUuid(uuid)->getXmlTag() == "TriangulatedRepresentation")	{
		uuidToVtkTriangulatedRepresentation[uuid] = new VtkTriangulatedRepresentation(getFileName(), name, uuid, parent, 0, repository, nullptr);
	}
	else {
		uuidToVtkSetPatch[uuid] = new VtkSetPatch(getFileName(), name, uuid, parent, repository, getIdProc(), getMaxProc());
	}
}

// ----------------------------------------------------------------------------
void VtkEpcDocument::addWellTrajTreeVtk(const std::string & uuid, const std::string & parent, const std::string & name)
{
	uuidToVtkWellboreTrajectoryRepresentation[uuid] = new VtkWellboreTrajectoryRepresentation(getFileName(), name, uuid, parent, repository, nullptr);
}

// ----------------------------------------------------------------------------
void VtkEpcDocument::addWellFrameTreeVtk(const std::string & uuid, const std::string & parent, const std::string & name)
{
	uuidToVtkWellboreTrajectoryRepresentation[parent]->createTreeVtk(uuid, parent, name, VtkEpcCommon::Resqml2Type::WELL_FRAME);
}
// ----------------------------------------------------------------------------
void VtkEpcDocument::addWellMarkerTreeVtk(const std::string & uuid, const std::string & parent, const std::string & name)
{
	uuidToVtkWellboreTrajectoryRepresentation[uuidIsChildOf[parent].getParent()]->createTreeVtk(uuid, parent, name, VtkEpcCommon::Resqml2Type::WELL_MARKER);
}

// ----------------------------------------------------------------------------
void VtkEpcDocument::addIjkGridTreeVtk(const std::string & uuid, const std::string & parent, const std::string & name)
{
	uuidToVtkIjkGridRepresentation[uuid] = new VtkIjkGridRepresentation(getFileName(), name, uuid, parent, repository, nullptr, getIdProc(), getMaxProc());
}

// ----------------------------------------------------------------------------
void VtkEpcDocument::addUnstrucGridTreeVtk(const std::string & uuid, const std::string & parent, const std::string & name)
{
	uuidToVtkUnstructuredGridRepresentation[uuid] = new VtkUnstructuredGridRepresentation(getFileName(), name, uuid, parent, repository, nullptr);
}

// ----------------------------------------------------------------------------
int VtkEpcDocument::addSubRepTreeVtk(const std::string & uuid, const std::string & parent, const std::string & name)
{
	if (uuidIsChildOf[uuid].getParentType() == VtkEpcCommon::Resqml2Type::IJK_GRID)	{
		uuidToVtkIjkGridRepresentation[uuid] = new VtkIjkGridRepresentation(getFileName(), name, uuid, parent, repository, repository);
		return 1;
	}
	else if (uuidIsChildOf[uuid].getParentType() == VtkEpcCommon::Resqml2Type::UNSTRUC_GRID)	{
		uuidToVtkUnstructuredGridRepresentation[uuid] = new VtkUnstructuredGridRepresentation(getFileName(), name, uuid, parent, repository, repository);
		return 1;
	}
	else if (uuidIsChildOf[uuid].getParentType() == VtkEpcCommon::Resqml2Type::PARTIAL)	{
		auto parentUuidType = uuidIsChildOf[parent].getParentType();
		COMMON_NS::DataObjectRepository const * pckEPCsrc = uuidToVtkPartialRepresentation[parent]->getEpcSource();

		switch (parentUuidType)	{
		case VtkEpcCommon::Resqml2Type::GRID_2D:	{
			uuidToVtkGrid2DRepresentation[uuid] = new VtkGrid2DRepresentation(getFileName(), name, uuid, parent, pckEPCsrc, repository);
			return 1;
		}
		case VtkEpcCommon::Resqml2Type::WELL_TRAJ: {
			uuidToVtkWellboreTrajectoryRepresentation[uuid] = new VtkWellboreTrajectoryRepresentation(getFileName(), name, uuid, parent, pckEPCsrc, repository);
			return 1;
		}
		case VtkEpcCommon::Resqml2Type::IJK_GRID: {
			uuidToVtkIjkGridRepresentation[uuid] = new VtkIjkGridRepresentation(getFileName(), name, uuid, parent, pckEPCsrc, repository);
			return 1;
		}
		case VtkEpcCommon::Resqml2Type::UNSTRUC_GRID: {
			uuidToVtkUnstructuredGridRepresentation[uuid] = new VtkUnstructuredGridRepresentation(getFileName(), name, uuid, parent, pckEPCsrc, repository);
			return 1;
		}
		default: break;
		}
	}
	return 0;
}

// ----------------------------------------------------------------------------
int VtkEpcDocument::addPropertyTreeVtk(const std::string & uuid, const std::string & parent, const std::string & name)
{
	switch (uuidIsChildOf[parent].getType()) {
	case VtkEpcCommon::Resqml2Type::GRID_2D: {
		uuidToVtkGrid2DRepresentation[parent]->createTreeVtk(uuid, parent, name, uuidIsChildOf[uuid].getType());
		return 1;
	}
	case VtkEpcCommon::Resqml2Type::POLYLINE_SET: {
		if (repository->getDataObjectByUuid(parent)->getXmlTag() == "PolylineRepresentation")	{
			uuidToVtkPolylineRepresentation[parent]->createTreeVtk(uuid, parent, name, uuidIsChildOf[uuid].getType());
			return 1;
		}
		else {
			uuidToVtkSetPatch[parent]->createTreeVtk(uuid, parent, name, uuidIsChildOf[uuid].getType());
			return 1;
		}
	}
	case VtkEpcCommon::Resqml2Type::TRIANGULATED_SET: {
		if (repository->getDataObjectByUuid(uuid)->getXmlTag() == "TriangulatedRepresentation")	{
			uuidToVtkTriangulatedRepresentation[parent]->createTreeVtk(uuid, parent, name, uuidIsChildOf[uuid].getType());
			return 1;
		}
		else {
			uuidToVtkSetPatch[parent]->createTreeVtk(uuid, parent, name, uuidIsChildOf[uuid].getType());
			return 1;
		}
	}
	case VtkEpcCommon::Resqml2Type::WELL_TRAJ: {
		uuidToVtkWellboreTrajectoryRepresentation[parent]->createTreeVtk(uuid, parent, name, uuidIsChildOf[uuid].getType());
		return 1;
	}
	case VtkEpcCommon::Resqml2Type::WELL_FRAME: {
		uuidToVtkWellboreTrajectoryRepresentation[uuidIsChildOf[parent].getParent()]->createTreeVtk(uuid, parent, name, uuidIsChildOf[uuid].getType());
		return 1;
	}
	case VtkEpcCommon::Resqml2Type::IJK_GRID: {
		uuidToVtkIjkGridRepresentation[parent]->createTreeVtk(uuid, parent, name, uuidIsChildOf[uuid].getType());
		return 1;
	}
	case VtkEpcCommon::Resqml2Type::UNSTRUC_GRID: {
		uuidToVtkUnstructuredGridRepresentation[parent]->createTreeVtk(uuid, parent, name, uuidIsChildOf[uuid].getType());
		return 1;
	}
	case VtkEpcCommon::Resqml2Type::SUB_REP: 	{
		if (uuidIsChildOf[parent].getParentType() == VtkEpcCommon::Resqml2Type::IJK_GRID) {
			uuidToVtkIjkGridRepresentation[parent]->createTreeVtk(uuid, parent, name, uuidIsChildOf[uuid].getType());
			return 1;
		}
		else if (uuidIsChildOf[parent].getParentType() == VtkEpcCommon::Resqml2Type::UNSTRUC_GRID) {
			uuidToVtkUnstructuredGridRepresentation[parent]->createTreeVtk(uuid, parent, name, uuidIsChildOf[uuid].getType());
			return 1;
		}
		else if (uuidIsChildOf[parent].getParentType() == VtkEpcCommon::Resqml2Type::PARTIAL) {
			auto uuidPartial = uuidIsChildOf[parent].getParent();

			switch (uuidIsChildOf[uuidPartial].getParentType()) {
			case VtkEpcCommon::Resqml2Type::GRID_2D:	{
				uuidToVtkGrid2DRepresentation[parent]->createTreeVtk(uuid, parent, name, uuidIsChildOf[uuid].getType());
				return 1;
			}
			case VtkEpcCommon::Resqml2Type::WELL_TRAJ:{
				uuidToVtkWellboreTrajectoryRepresentation[parent]->createTreeVtk(uuid, parent, name, uuidIsChildOf[uuid].getType());
				return 1;
			}
			case VtkEpcCommon::Resqml2Type::IJK_GRID: {
				uuidToVtkIjkGridRepresentation[parent]->createTreeVtk(uuid, parent, name, uuidIsChildOf[uuid].getType());
				return 1;
			}
			case VtkEpcCommon::Resqml2Type::UNSTRUC_GRID: {
				uuidToVtkUnstructuredGridRepresentation[parent]->createTreeVtk(uuid, parent, name, uuidIsChildOf[uuid].getType());
				return 1;
			}
			default:break;
			}
		}
		break;
	}
	case VtkEpcCommon::Resqml2Type::PARTIAL:	{
		uuidToVtkPartialRepresentation[uuidIsChildOf[parent].getUuid()]->createTreeVtk(uuid, parent, name, uuidIsChildOf[uuid].getType());
		return 1;
	}
	default: {
		throw std::logic_error("The property to add has got an unknown or unsupported parent");
	}
	}
	return 0;

}

// ----------------------------------------------------------------------------
void VtkEpcDocument::createTreeVtkPartialRep(const std::string & uuid, VtkEpcDocument *vtkEpcDowumentWithCompleteRep)
{
	uuidIsChildOf[uuid].setType( VtkEpcCommon::Resqml2Type::PARTIAL);
	uuidIsChildOf[uuid].setUuid(uuid);
	uuidToVtkPartialRepresentation[uuid] = new VtkPartialRepresentation(getFileName(), uuid, vtkEpcDowumentWithCompleteRep, repository);
}

// ----------------------------------------------------------------------------
void VtkEpcDocument::visualize(const std::string & uuid)
{
	auto fesapiObject = repository->getDataObjectByUuid(uuid);
	if (dynamic_cast<RESQML2_NS::AbstractRepresentation*>(fesapiObject) == nullptr &&
			dynamic_cast<RESQML2_NS::AbstractValuesProperty*>(fesapiObject) == nullptr &&
			dynamic_cast<RESQML2_NS::WellboreMarker*>(fesapiObject) == nullptr) {
		// Cannot visualize anything other than a RESQML property or a RESQML representation.
		return;
	}

	auto dataObjInfo = uuidIsChildOf[uuid];
	auto uuidToAttach = dataObjInfo.getUuid();
	auto parentUuid = dataObjInfo.getParent();
	switch (uuidIsChildOf[uuid].getType())	{
	case VtkEpcCommon::Resqml2Type::GRID_2D:	{
		uuidToVtkGrid2DRepresentation[uuidToAttach]->visualize(uuid);
		break;
	}
	case VtkEpcCommon::Resqml2Type::POLYLINE_SET: {
		auto object = repository->getDataObjectByUuid(uuidToAttach);
		if (object->getXmlTag() == "PolylineRepresentation")	{
			uuidToVtkPolylineRepresentation[uuidToAttach]->visualize(uuid);
		}
		else {
			uuidToVtkSetPatch[uuidToAttach]->visualize(uuid);
		}
		break;
	}
	case VtkEpcCommon::Resqml2Type::TRIANGULATED_SET: {
		auto object = repository->getDataObjectByUuid(uuidToAttach);
		if (object->getXmlTag() == "TriangulatedRepresentation") {
			uuidToVtkTriangulatedRepresentation[uuidToAttach]->visualize(uuid);
		}
		else {
			uuidToVtkSetPatch[uuidToAttach]->visualize(uuid);
		}
		break;
	}
	case VtkEpcCommon::Resqml2Type::WELL_TRAJ: {
		uuidToVtkWellboreTrajectoryRepresentation[uuidToAttach]->visualize(uuid);
		break;
	}
	case VtkEpcCommon::Resqml2Type::WELL_MARKER: {
		uuidToVtkWellboreTrajectoryRepresentation[uuidIsChildOf[uuidIsChildOf[uuid].getParent()].getParent()]->visualize(uuid);
		uuidToAttach = uuidIsChildOf[uuidIsChildOf[uuid].getParent()].getParent();
		break;
	}
	case VtkEpcCommon::Resqml2Type::IJK_GRID: {
		uuidToVtkIjkGridRepresentation[uuidToAttach]->visualize(uuid);
		break;
	}
	case VtkEpcCommon::Resqml2Type::UNSTRUC_GRID: {
		uuidToVtkUnstructuredGridRepresentation[uuidToAttach]->visualize(uuid);
		break;
	}
	case VtkEpcCommon::Resqml2Type::SUB_REP:	{
		if (uuidIsChildOf[uuid].getParentType() == VtkEpcCommon::Resqml2Type::IJK_GRID) {
			uuidToVtkIjkGridRepresentation[uuidToAttach]->visualize(uuid);
		}
		else if (uuidIsChildOf[uuid].getParentType() == VtkEpcCommon::Resqml2Type::UNSTRUC_GRID) {
			uuidToVtkUnstructuredGridRepresentation[uuidToAttach]->visualize(uuid);
		}
		else if (uuidIsChildOf[uuid].getParentType() == VtkEpcCommon::Resqml2Type::PARTIAL) {
			auto parent = uuidIsChildOf[uuid].getParent();

			switch (uuidIsChildOf[parent].getParentType() ) {
			case VtkEpcCommon::Resqml2Type::GRID_2D: {
				uuidToVtkGrid2DRepresentation[uuid]->visualize(uuid);
				break;
			}
			case VtkEpcCommon::Resqml2Type::WELL_TRAJ: {
				uuidToVtkWellboreTrajectoryRepresentation[uuid]->visualize(uuid);
				break;
			}
			case VtkEpcCommon::Resqml2Type::IJK_GRID: {
				uuidToVtkIjkGridRepresentation[uuid]->visualize(uuid);
				break;
			}
			case VtkEpcCommon::Resqml2Type::UNSTRUC_GRID: {
				uuidToVtkUnstructuredGridRepresentation[uuid]->visualize(uuid);
				break;
			}
			default: break;
			}
		}
		break;
	}
	case VtkEpcCommon::Resqml2Type::PARTIAL:	{
		uuidToVtkPartialRepresentation[uuidToAttach]->visualize(uuid);
		break;
	}
	case (VtkEpcCommon::Resqml2Type::TIME_SERIES): {
		uuidToAttach = uuidIsChildOf[uuid].getParent();
		switch (uuidIsChildOf[uuid].getParentType()) {
		case VtkEpcCommon::Resqml2Type::GRID_2D: {
			uuidToVtkGrid2DRepresentation[uuidIsChildOf[uuid].getParent()]->visualize(uuid);
			break;
		}
		case VtkEpcCommon::Resqml2Type::POLYLINE_SET: {
			auto object = repository->getDataObjectByUuid(uuidIsChildOf[uuid].getParent());
			if (object->getXmlTag() == "PolylineRepresentation")	{
				uuidToVtkPolylineRepresentation[uuidIsChildOf[uuid].getParent()]->visualize(uuid);
			}
			else {
				uuidToVtkSetPatch[uuidIsChildOf[uuid].getParent()]->visualize(uuid);
			}
			break;
		}
		case VtkEpcCommon::Resqml2Type::TRIANGULATED_SET: {
			auto object = repository->getDataObjectByUuid(uuid);
			if (object->getXmlTag() == "TriangulatedRepresentation") {
				uuidToVtkTriangulatedRepresentation[uuidIsChildOf[uuid].getParent()]->visualize(uuid);
			}
			else {
				uuidToVtkSetPatch[uuidIsChildOf[uuid].getParent()]->visualize(uuid);
			}
			break;
		}
		case VtkEpcCommon::Resqml2Type::WELL_TRAJ: {
			uuidToVtkWellboreTrajectoryRepresentation[uuidIsChildOf[uuid].getParent()]->visualize(uuid);
			break;
		}
		case VtkEpcCommon::Resqml2Type::IJK_GRID: {
			uuidToVtkIjkGridRepresentation[uuidIsChildOf[uuid].getParent()]->visualize(uuid);
			break;
		}
		case VtkEpcCommon::Resqml2Type::UNSTRUC_GRID: {
			uuidToVtkUnstructuredGridRepresentation[uuidIsChildOf[uuid].getParent()]->visualize(uuid);
			break;
		}
		case VtkEpcCommon::Resqml2Type::SUB_REP: {
			if (uuidIsChildOf[uuidIsChildOf[uuid].getParent()].getParentType() == VtkEpcCommon::Resqml2Type::IJK_GRID)	{
				uuidToVtkIjkGridRepresentation[uuidIsChildOf[uuidIsChildOf[uuid].getParent()].getUuid()]->visualize(uuid);
			}
			else if (uuidIsChildOf[uuidIsChildOf[uuid].getParent()].getParentType() == VtkEpcCommon::Resqml2Type::UNSTRUC_GRID) {
				uuidToVtkUnstructuredGridRepresentation[uuidIsChildOf[uuidIsChildOf[uuid].getParent()].getUuid()]->visualize(uuid);
			}
			else if (uuidIsChildOf[uuidIsChildOf[uuid].getParent()].getParentType() == VtkEpcCommon::Resqml2Type::PARTIAL)	{
				if (uuidIsChildOf[uuidIsChildOf[uuidIsChildOf[uuid].getParent()].getParent()].getParentType() == VtkEpcCommon::Resqml2Type::IJK_GRID)	{
					uuidToVtkIjkGridRepresentation[uuidIsChildOf[uuidIsChildOf[uuid].getParent()].getUuid()]->visualize(uuid);
				}
				else if (uuidIsChildOf[uuidIsChildOf[uuidIsChildOf[uuid].getParent()].getParent()].getParentType() == VtkEpcCommon::Resqml2Type::UNSTRUC_GRID) {
					uuidToVtkUnstructuredGridRepresentation[uuidIsChildOf[uuidIsChildOf[uuid].getParent()].getUuid()]->visualize(uuid);
				}
			}
			break;
		}
		case VtkEpcCommon::Resqml2Type::PARTIAL: {
			uuidToVtkPartialRepresentation[uuidIsChildOf[uuid].getParent()]->visualize(uuid);
			break;
		}
		default: break;
		}
	}
	case VtkEpcCommon::Resqml2Type::PROPERTY: {
		uuidToAttach = parentUuid;

		switch (dataObjInfo.getParentType()) {
		case VtkEpcCommon::Resqml2Type::GRID_2D: {
			uuidToVtkGrid2DRepresentation[parentUuid]->visualize(uuid);
			break;
		}
		case VtkEpcCommon::Resqml2Type::POLYLINE_SET: {
			auto object = repository->getDataObjectByUuid(parentUuid);
			if (object->getXmlTag() == "PolylineRepresentation")	{
				uuidToVtkPolylineRepresentation[parentUuid]->visualize(uuid);
			}
			else {
				uuidToVtkSetPatch[parentUuid]->visualize(uuid);
			}
			break;
		}
		case VtkEpcCommon::Resqml2Type::TRIANGULATED_SET: {
			auto object = repository->getDataObjectByUuid(uuid);
			if (object->getXmlTag() == "TriangulatedRepresentation")	{
				uuidToVtkTriangulatedRepresentation[parentUuid]->visualize(uuid);
			}
			else {
				uuidToVtkSetPatch[parentUuid]->visualize(uuid);
			}
			break;
		}
		case VtkEpcCommon::Resqml2Type::WELL_TRAJ: {
			uuidToVtkWellboreTrajectoryRepresentation[parentUuid]->visualize(uuid);
			break;
		}
		case VtkEpcCommon::Resqml2Type::WELL_FRAME: {
			uuidToVtkWellboreTrajectoryRepresentation[uuidIsChildOf[parentUuid].getParent()]->visualize(uuid);
			break;
		}
		case VtkEpcCommon::Resqml2Type::IJK_GRID: {
			uuidToVtkIjkGridRepresentation[parentUuid]->visualize(uuid);
			break;
		}
		case VtkEpcCommon::Resqml2Type::UNSTRUC_GRID: {
			uuidToVtkUnstructuredGridRepresentation[parentUuid]->visualize(uuid);
			break;
		}
		case VtkEpcCommon::Resqml2Type::SUB_REP: {
			if (uuidIsChildOf[parentUuid].getParentType() == VtkEpcCommon::Resqml2Type::IJK_GRID)	{
				uuidToVtkIjkGridRepresentation[uuidIsChildOf[parentUuid].getUuid()]->visualize(uuid);
			}
			else if (uuidIsChildOf[parentUuid].getParentType() == VtkEpcCommon::Resqml2Type::UNSTRUC_GRID) {
				uuidToVtkUnstructuredGridRepresentation[uuidIsChildOf[parentUuid].getUuid()]->visualize(uuid);
			}
			else if (uuidIsChildOf[parentUuid].getParentType() == VtkEpcCommon::Resqml2Type::PARTIAL)	{
				if (uuidIsChildOf[uuidIsChildOf[parentUuid].getParent()].getParentType() == VtkEpcCommon::Resqml2Type::IJK_GRID)	{
					uuidToVtkIjkGridRepresentation[uuidIsChildOf[parentUuid].getUuid()]->visualize(uuid);
				}
				else if (uuidIsChildOf[uuidIsChildOf[parentUuid].getParent()].getParentType() == VtkEpcCommon::Resqml2Type::UNSTRUC_GRID) {
					uuidToVtkUnstructuredGridRepresentation[uuidIsChildOf[parentUuid].getUuid()]->visualize(uuid);
				}
			}
			break;
		}
		case VtkEpcCommon::Resqml2Type::PARTIAL:	{
			uuidToVtkPartialRepresentation[parentUuid]->visualize(uuid);
			break;
		}
		default: break;
		}
		default: break;
	}
	}

	try
	{
		// attach representation to EpcDocument VtkMultiBlockDataSet
		if (std::find(attachUuids.begin(), attachUuids.end(), uuidToAttach) == attachUuids.end()) {
			detach();
			attachUuids.push_back(uuidToAttach);
			attach();
		}
	}
	catch (const std::exception&)
	{
		cout << "ERROR with uuid attachment: " << uuid ;
	}
}

// ----------------------------------------------------------------------------
void VtkEpcDocument::visualizeFullWell()
{
	for (auto &vtkEpcCommon : getAllVtkEpcCommons()) {
		if (vtkEpcCommon->getType() == VtkEpcCommon::Resqml2Type::WELL_TRAJ) {
			uuidToVtkWellboreTrajectoryRepresentation[vtkEpcCommon->getUuid()]->visualize(vtkEpcCommon->getUuid());
			try
			{
				// attach representation to EpcDocument VtkMultiBlockDataSet
				if (std::find(attachUuids.begin(), attachUuids.end(), vtkEpcCommon->getUuid()) == attachUuids.end()) {
					detach();
					attachUuids.push_back(vtkEpcCommon->getUuid());
					attach();
				}
			}
			catch (const std::exception&)
			{
				cout << "ERROR with uuid attachment: " << vtkEpcCommon->getUuid() ;
			}
		}
	}
}

// ----------------------------------------------------------------------------
void VtkEpcDocument::unvisualizeFullWell()
{
	for (auto &vtkEpcCommon : getAllVtkEpcCommons()) {
		if (std::find(attachUuids.begin(), attachUuids.end(), vtkEpcCommon->getUuid()) != attachUuids.end()) {
			if (uuidIsChildOf[vtkEpcCommon->getUuid()].getType() == VtkEpcCommon::Resqml2Type::WELL_TRAJ) {
				uuidToVtkWellboreTrajectoryRepresentation[vtkEpcCommon->getUuid()]->remove(vtkEpcCommon->getUuid());
				detach();
				attachUuids.erase(std::find(attachUuids.begin(), attachUuids.end(), vtkEpcCommon->getUuid()));
				attach();
			}
		}
	}
}

// ----------------------------------------------------------------------------
void VtkEpcDocument::toggleMarkerOrientation(const bool orientation) {
	for (auto &uuid : uuidRep) {
		if (uuidIsChildOf[uuid].getType() == VtkEpcCommon::Resqml2Type::WELL_MARKER) {
			uuidToVtkWellboreTrajectoryRepresentation[uuidIsChildOf[uuidIsChildOf[uuid].getParent()].getParent()]->toggleMarkerOrientation(orientation);
		}
	}
}

// ----------------------------------------------------------------------------
void VtkEpcDocument::setMarkerSize(int size) {
	for (auto &uuid : uuidRep) {
		if (uuidIsChildOf[uuid].getType() == VtkEpcCommon::Resqml2Type::WELL_MARKER) {
			uuidToVtkWellboreTrajectoryRepresentation[uuidIsChildOf[uuidIsChildOf[uuid].getParent()].getParent()]->setMarkerSize(size);
		}
	}
}

// ----------------------------------------------------------------------------
void VtkEpcDocument::remove(const std::string & uuid)
{
	std::string uuidtoAttach = uuidIsChildOf[uuid].getType() == VtkEpcCommon::Resqml2Type::PROPERTY || uuidIsChildOf[uuid].getType() == VtkEpcCommon::Resqml2Type::WELL_MARKER
		? uuidIsChildOf[uuid].getParent()
		: uuid;

	if (uuidIsChildOf[uuidtoAttach].getType() == VtkEpcCommon::Resqml2Type::WELL_MARKER_FRAME ||
		uuidIsChildOf[uuidtoAttach].getType() == VtkEpcCommon::Resqml2Type::WELL_FRAME) {
		uuidtoAttach = uuidIsChildOf[uuidtoAttach].getParent();
	}

	if (std::find(attachUuids.begin(), attachUuids.end(), uuidtoAttach) != attachUuids.end()) {
		switch (uuidIsChildOf[uuidtoAttach].getType())	{
		case VtkEpcCommon::Resqml2Type::GRID_2D:	{
			uuidToVtkGrid2DRepresentation[uuidtoAttach]->remove(uuid);
			if (uuid == uuidtoAttach){
				detach();
				attachUuids.erase(std::find(attachUuids.begin(), attachUuids.end(), uuid));
				attach();
			}
			break;
		}
		case VtkEpcCommon::Resqml2Type::POLYLINE_SET: {
			if (repository->getDataObjectByUuid(uuidtoAttach)->getXmlTag() == "PolylineRepresentation") {
				uuidToVtkPolylineRepresentation[uuidIsChildOf[uuidtoAttach].getUuid()]->remove(uuid);
				if (uuid == uuidtoAttach){
					detach();
					attachUuids.erase(std::find(attachUuids.begin(), attachUuids.end(), uuid));
					attach();
				}
			}
			else  {
				uuidToVtkSetPatch[uuidtoAttach]->remove(uuid);
				if (uuid == uuidtoAttach){
					detach();
					attachUuids.erase(std::find(attachUuids.begin(), attachUuids.end(), uuid));
					attach();
				}
			}
			break;
		}
		case VtkEpcCommon::Resqml2Type::TRIANGULATED_SET: {
			if (repository->getDataObjectByUuid(uuidtoAttach)->getXmlTag() == "TriangulatedRepresentation")  {
				uuidToVtkTriangulatedRepresentation[uuidtoAttach]->remove(uuid);
				if (uuid == uuidtoAttach){
					detach();
					attachUuids.erase(std::find(attachUuids.begin(), attachUuids.end(), uuid));
					attach();
				}
			}
			else {
				uuidToVtkSetPatch[uuidtoAttach]->remove(uuid);
				if (uuid == uuidtoAttach){
					detach();
					attachUuids.erase(std::find(attachUuids.begin(), attachUuids.end(), uuid));
					attach();
				}
			}
			break;
		}
		case VtkEpcCommon::Resqml2Type::WELL_TRAJ: {
			uuidToVtkWellboreTrajectoryRepresentation[uuidtoAttach]->remove(uuid);
			if (uuid == uuidtoAttach){
				detach();
				attachUuids.erase(std::find(attachUuids.begin(), attachUuids.end(), uuid));
				attach();
			}
			break;
		}
		case VtkEpcCommon::Resqml2Type::IJK_GRID: {
			uuidToVtkIjkGridRepresentation[uuidtoAttach]->remove(uuid);
			if (uuid == uuidtoAttach) {
				detach();
				attachUuids.erase(std::find(attachUuids.begin(), attachUuids.end(), uuid));
				attach();
			}
			break;
		}
		case VtkEpcCommon::Resqml2Type::UNSTRUC_GRID: {
			uuidToVtkUnstructuredGridRepresentation[uuidtoAttach]->remove(uuid);
			if (uuid == uuidtoAttach) {
				detach();
				attachUuids.erase(std::find(attachUuids.begin(), attachUuids.end(), uuid));
				attach();
			}
			break;
		}
		case VtkEpcCommon::Resqml2Type::SUB_REP: {
			if (uuidIsChildOf[uuidIsChildOf[uuid].getParent()].getParentType() == VtkEpcCommon::Resqml2Type::IJK_GRID) {
				uuidToVtkIjkGridRepresentation[uuidIsChildOf[uuidIsChildOf[uuid].getParent()].getUuid()]->remove(uuid);
			}
			else if (uuidIsChildOf[uuidIsChildOf[uuid].getParent()].getParentType() == VtkEpcCommon::Resqml2Type::UNSTRUC_GRID) {
				uuidToVtkUnstructuredGridRepresentation[uuidIsChildOf[uuidIsChildOf[uuid].getParent()].getUuid()]->remove(uuid);
			}
			else if (uuidIsChildOf[uuidIsChildOf[uuid].getParent()].getParentType() == VtkEpcCommon::Resqml2Type::PARTIAL)	{
				if (uuidIsChildOf[uuidIsChildOf[uuidIsChildOf[uuid].getParent()].getParent()].getParentType() == VtkEpcCommon::Resqml2Type::IJK_GRID)	{
					uuidToVtkIjkGridRepresentation[uuidIsChildOf[uuidIsChildOf[uuid].getParent()].getUuid()]->remove(uuid);
				}
				if (uuidIsChildOf[uuidIsChildOf[uuidIsChildOf[uuid].getParent()].getParent()].getParentType() == VtkEpcCommon::Resqml2Type::UNSTRUC_GRID)	{
					uuidToVtkUnstructuredGridRepresentation[uuidIsChildOf[uuidIsChildOf[uuid].getParent()].getUuid()]->remove(uuid);
				}
			}
			if (uuid == uuidtoAttach) {
				detach();
				attachUuids.erase(std::find(attachUuids.begin(), attachUuids.end(), uuid));
				attach();
			}
			break;
		}
		case VtkEpcCommon::Resqml2Type::PARTIAL: {
			uuidToVtkPartialRepresentation[uuidtoAttach]->remove(uuid);
			break;
		}
		default: break;
		}
	}
}

// ----------------------------------------------------------------------------
void VtkEpcDocument::attach()
{
	if (attachUuids.size() > (std::numeric_limits<unsigned int>::max)()) {
		throw std::range_error("Too much attached uuids");
	}
	unsigned int newBlockIndex = 0;
	for (std::string& uuid : attachUuids) {
		if (uuidIsChildOf[uuid].getType() == VtkEpcCommon::Resqml2Type::GRID_2D)	{
			vtkOutput->SetBlock(newBlockIndex, uuidToVtkGrid2DRepresentation[uuid]->getOutput());
			vtkOutput->GetMetaData(newBlockIndex++)->Set(vtkCompositeDataSet::NAME(), uuidToVtkGrid2DRepresentation[uuid]->getUuid().c_str());
		}
		else if (uuidIsChildOf[uuid].getType() == VtkEpcCommon::Resqml2Type::POLYLINE_SET) {
			if (repository->getDataObjectByUuid(uuid)->getXmlTag() == "PolylineRepresentation") {
				vtkOutput->SetBlock(newBlockIndex, uuidToVtkPolylineRepresentation[uuid]->getOutput());
				vtkOutput->GetMetaData(newBlockIndex++)->Set(vtkCompositeDataSet::NAME(), uuidToVtkPolylineRepresentation[uuid]->getUuid().c_str());
			}
			else  {
				vtkOutput->SetBlock(newBlockIndex, uuidToVtkSetPatch[uuid]->getOutput());
				vtkOutput->GetMetaData(newBlockIndex++)->Set(vtkCompositeDataSet::NAME(), uuidToVtkSetPatch[uuid]->getUuid().c_str());
			}
		}
		else if (uuidIsChildOf[uuid].getType() == VtkEpcCommon::Resqml2Type::TRIANGULATED_SET) {
			if (repository->getDataObjectByUuid(uuid)->getXmlTag() == "TriangulatedRepresentation")  {
				vtkOutput->SetBlock(newBlockIndex, uuidToVtkTriangulatedRepresentation[uuid]->getOutput());
				vtkOutput->GetMetaData(newBlockIndex++)->Set(vtkCompositeDataSet::NAME(), uuidToVtkTriangulatedRepresentation[uuid]->getUuid().c_str());
			}
			else  {
				vtkOutput->SetBlock(newBlockIndex, uuidToVtkSetPatch[uuid]->getOutput());
				vtkOutput->GetMetaData(newBlockIndex++)->Set(vtkCompositeDataSet::NAME(), uuidToVtkSetPatch[uuid]->getUuid().c_str());
			}
		}
		else if (uuidIsChildOf[uuid].getType() == VtkEpcCommon::Resqml2Type::WELL_TRAJ) {
			vtkOutput->SetBlock(newBlockIndex, uuidToVtkWellboreTrajectoryRepresentation[uuid]->getOutput());
			vtkOutput->GetMetaData(newBlockIndex++)->Set(vtkCompositeDataSet::NAME(), uuidToVtkWellboreTrajectoryRepresentation[uuid]->getUuid().c_str());
		}
		else if (uuidIsChildOf[uuid].getType() == VtkEpcCommon::Resqml2Type::IJK_GRID) {
			vtkOutput->SetBlock(newBlockIndex, uuidToVtkIjkGridRepresentation[uuid]->getOutput());
			vtkOutput->GetMetaData(newBlockIndex++)->Set(vtkCompositeDataSet::NAME(), uuidToVtkIjkGridRepresentation[uuid]->getUuid().c_str());
		}
		else if (uuidIsChildOf[uuid].getType() == VtkEpcCommon::Resqml2Type::UNSTRUC_GRID) {
			vtkOutput->SetBlock(newBlockIndex, uuidToVtkUnstructuredGridRepresentation[uuid]->getOutput());
			vtkOutput->GetMetaData(newBlockIndex++)->Set(vtkCompositeDataSet::NAME(), uuidToVtkUnstructuredGridRepresentation[uuid]->getUuid().c_str());
		}
		else if (uuidIsChildOf[uuid].getType() == VtkEpcCommon::Resqml2Type::SUB_REP) {
			if (uuidIsChildOf[uuid].getParentType() == VtkEpcCommon::Resqml2Type::IJK_GRID) {
				vtkOutput->SetBlock(newBlockIndex, uuidToVtkIjkGridRepresentation[uuidIsChildOf[uuid].getUuid()]->getOutput());
				vtkOutput->GetMetaData(newBlockIndex++)->Set(vtkCompositeDataSet::NAME(), uuidToVtkIjkGridRepresentation[uuid]->getUuid().c_str());
			}
			else if (uuidIsChildOf[uuid].getParentType() == VtkEpcCommon::Resqml2Type::UNSTRUC_GRID) {
				vtkOutput->SetBlock(newBlockIndex, uuidToVtkUnstructuredGridRepresentation[uuidIsChildOf[uuid].getUuid()]->getOutput());
				vtkOutput->GetMetaData(newBlockIndex++)->Set(vtkCompositeDataSet::NAME(), uuidToVtkUnstructuredGridRepresentation[uuidIsChildOf[uuid].getUuid()]->getUuid().c_str());
			}
			else if (uuidIsChildOf[uuid].getParentType() == VtkEpcCommon::Resqml2Type::PARTIAL) {
				switch (uuidToVtkPartialRepresentation[uuidIsChildOf[uuid].getParent()]->getType())	{
				case VtkEpcCommon::Resqml2Type::GRID_2D:	{
					vtkOutput->SetBlock(newBlockIndex, uuidToVtkGrid2DRepresentation[uuid]->getOutput());
					vtkOutput->GetMetaData(newBlockIndex++)->Set(vtkCompositeDataSet::NAME(), uuidToVtkGrid2DRepresentation[uuid]->getUuid().c_str());
					break;
				}
				case VtkEpcCommon::Resqml2Type::WELL_TRAJ: {
					vtkOutput->SetBlock(newBlockIndex, uuidToVtkWellboreTrajectoryRepresentation[uuid]->getOutput());
					vtkOutput->GetMetaData(newBlockIndex++)->Set(vtkCompositeDataSet::NAME(), uuidToVtkWellboreTrajectoryRepresentation[uuid]->getUuid().c_str());
					break;
				}
				case VtkEpcCommon::Resqml2Type::IJK_GRID: {
					vtkOutput->SetBlock(newBlockIndex, uuidToVtkIjkGridRepresentation[uuidIsChildOf[uuid].getUuid()]->getOutput());
					vtkOutput->GetMetaData(newBlockIndex++)->Set(vtkCompositeDataSet::NAME(), uuidToVtkIjkGridRepresentation[uuid]->getUuid().c_str());
					break;
				}
				case VtkEpcCommon::Resqml2Type::UNSTRUC_GRID: {
					vtkOutput->SetBlock(newBlockIndex, uuidToVtkUnstructuredGridRepresentation[uuidIsChildOf[uuid].getUuid()]->getOutput());
					vtkOutput->GetMetaData(newBlockIndex++)->Set(vtkCompositeDataSet::NAME(), uuidToVtkUnstructuredGridRepresentation[uuidIsChildOf[uuid].getUuid()]->getUuid().c_str());
					break;
				}
				default:
					break;
				}
			}
		}
	}
}

// ----------------------------------------------------------------------------
void VtkEpcDocument::addProperty(const std::string & uuidProperty, vtkDataArray* dataProperty)
{
	switch (uuidIsChildOf[uuidProperty].getType())	{
	case VtkEpcCommon::Resqml2Type::GRID_2D:	{
		uuidToVtkGrid2DRepresentation[uuidIsChildOf[uuidProperty].getUuid()]->addProperty(uuidProperty, dataProperty);
		break;
	}
	case VtkEpcCommon::Resqml2Type::POLYLINE_SET: {
		auto object = repository->getDataObjectByUuid(uuidIsChildOf[uuidProperty].getUuid());
		auto typeRepresentation = object->getXmlTag();
		if (typeRepresentation == "PolylineRepresentation")	{
			uuidToVtkPolylineRepresentation[uuidIsChildOf[uuidProperty].getUuid()]->addProperty(uuidProperty, dataProperty);
		}
		else {
			uuidToVtkSetPatch[uuidIsChildOf[uuidProperty].getUuid()]->addProperty(uuidProperty, dataProperty);
		}
		break;
	}
	case VtkEpcCommon::Resqml2Type::TRIANGULATED_SET: {
		auto object = repository->getDataObjectByUuid(uuidIsChildOf[uuidProperty].getUuid());
		auto typeRepresentation = object->getXmlTag();
		if (typeRepresentation == "TriangulatedRepresentation") {
			uuidToVtkTriangulatedRepresentation[uuidIsChildOf[uuidProperty].getUuid()]->addProperty(uuidProperty, dataProperty);
		}
		else {
			uuidToVtkSetPatch[uuidIsChildOf[uuidProperty].getUuid()]->addProperty(uuidProperty, dataProperty);
		}
		break;
	}
	case VtkEpcCommon::Resqml2Type::WELL_TRAJ: {
		uuidToVtkWellboreTrajectoryRepresentation[uuidIsChildOf[uuidProperty].getUuid()]->addProperty(uuidProperty, dataProperty);
		break;
	}
	case VtkEpcCommon::Resqml2Type::IJK_GRID: {
		uuidToVtkIjkGridRepresentation[uuidIsChildOf[uuidProperty].getUuid()]->addProperty(uuidProperty, dataProperty);
		break;
	}
	case VtkEpcCommon::Resqml2Type::UNSTRUC_GRID: {
		uuidToVtkUnstructuredGridRepresentation[uuidIsChildOf[uuidProperty].getUuid()]->addProperty(uuidProperty, dataProperty);
		break;
	}
	case VtkEpcCommon::Resqml2Type::SUB_REP: {
		if (uuidIsChildOf[uuidProperty].getParentType() == VtkEpcCommon::Resqml2Type::IJK_GRID) {
			uuidToVtkIjkGridRepresentation[uuidIsChildOf[uuidProperty].getUuid()]->addProperty(uuidProperty, dataProperty);
		}
		if (uuidIsChildOf[uuidProperty].getParentType() == VtkEpcCommon::Resqml2Type::UNSTRUC_GRID) {
			uuidToVtkUnstructuredGridRepresentation[uuidIsChildOf[uuidProperty].getUuid()]->addProperty(uuidProperty, dataProperty);
		}
		if (uuidIsChildOf[uuidProperty].getParentType() == VtkEpcCommon::Resqml2Type::PARTIAL) {
			auto parentUuidType = uuidToVtkPartialRepresentation[uuidIsChildOf[uuidProperty].getParent()]->getType();

			switch (parentUuidType) {
			case VtkEpcCommon::Resqml2Type::GRID_2D: {
				uuidToVtkGrid2DRepresentation[uuidIsChildOf[uuidProperty].getUuid()]->addProperty(uuidProperty, dataProperty);
				break;
			}
			case VtkEpcCommon::Resqml2Type::WELL_TRAJ: {
				uuidToVtkWellboreTrajectoryRepresentation[uuidIsChildOf[uuidProperty].getUuid()]->addProperty(uuidProperty, dataProperty);
				break;
			}
			case VtkEpcCommon::Resqml2Type::IJK_GRID: {
				uuidToVtkIjkGridRepresentation[uuidIsChildOf[uuidProperty].getUuid()]->addProperty(uuidProperty, dataProperty);
				break;
			}
			case VtkEpcCommon::Resqml2Type::UNSTRUC_GRID: {
				uuidToVtkUnstructuredGridRepresentation[uuidIsChildOf[uuidProperty].getUuid()]->addProperty(uuidProperty, dataProperty);
				break;
			}
			default:
				break;
			}
		}
	}
	default: break;
	}
	// attach representation to EpcDocument VtkMultiBlockDataSet
	std::string parent = uuidIsChildOf[uuidProperty].getUuid();
	if (std::find(attachUuids.begin(), attachUuids.end(), parent) == attachUuids.end()) {
		detach();
		attachUuids.push_back(parent);
		attach();
	}
}

// ----------------------------------------------------------------------------
long VtkEpcDocument::getAttachmentPropertyCount(const std::string & uuid, VtkEpcCommon::FesppAttachmentProperty propertyUnit)
{
	long result = 0;
	switch (uuidIsChildOf[uuid].getType()) {
	case VtkEpcCommon::Resqml2Type::IJK_GRID: {
		result = uuidToVtkIjkGridRepresentation[uuidIsChildOf[uuid].getUuid()]->getAttachmentPropertyCount(uuid, propertyUnit);
		break;
	}
	case VtkEpcCommon::Resqml2Type::UNSTRUC_GRID: {
		result = uuidToVtkUnstructuredGridRepresentation[uuidIsChildOf[uuid].getUuid()]->getAttachmentPropertyCount(uuid, propertyUnit);
		break;
	}
	case VtkEpcCommon::Resqml2Type::SUB_REP: {
		if (uuidIsChildOf[uuid].getParentType() == VtkEpcCommon::Resqml2Type::IJK_GRID) {
			result = uuidToVtkIjkGridRepresentation[uuidIsChildOf[uuid].getUuid()]->getAttachmentPropertyCount(uuid, propertyUnit);
		}
		if (uuidIsChildOf[uuid].getParentType() == VtkEpcCommon::Resqml2Type::UNSTRUC_GRID) {
			result = uuidToVtkUnstructuredGridRepresentation[uuidIsChildOf[uuid].getUuid()]->getAttachmentPropertyCount(uuid, propertyUnit);
		}
		if (uuidIsChildOf[uuid].getParentType() == VtkEpcCommon::Resqml2Type::PARTIAL) {
			auto uuidPartial = uuidIsChildOf[uuid].getParent();

			if (uuidIsChildOf[uuidPartial].getParentType() == VtkEpcCommon::Resqml2Type::IJK_GRID) {
				result = uuidToVtkIjkGridRepresentation[uuid]->getAttachmentPropertyCount(uuid, propertyUnit);
			}
			if (uuidIsChildOf[uuid].getParentType() == VtkEpcCommon::Resqml2Type::UNSTRUC_GRID) {
				result = uuidToVtkUnstructuredGridRepresentation[uuid]->getAttachmentPropertyCount(uuid, propertyUnit);
			}
		}
		break;
	}
	default: break;
	}
	return result;
}

// ----------------------------------------------------------------------------
int VtkEpcDocument::getICellCount(const std::string & uuid)
{
	long result = 0;
	if (uuidIsChildOf[uuid].getType() == VtkEpcCommon::Resqml2Type::IJK_GRID) {
		result = uuidToVtkIjkGridRepresentation[uuidIsChildOf[uuid].getUuid()]->getICellCount(uuid);
	}
	else if (uuidIsChildOf[uuid].getType() == VtkEpcCommon::Resqml2Type::SUB_REP) {
		if (uuidIsChildOf[uuid].getParentType() == VtkEpcCommon::Resqml2Type::IJK_GRID) {
			result = uuidToVtkIjkGridRepresentation[uuidIsChildOf[uuid].getUuid()]->getICellCount(uuid);
		}
		if (uuidIsChildOf[uuid].getParentType() == VtkEpcCommon::Resqml2Type::PARTIAL) {
			auto uuidPartial = uuidIsChildOf[uuid].getParent();

			if (uuidIsChildOf[uuidPartial].getParentType() == VtkEpcCommon::Resqml2Type::IJK_GRID) {
				result = uuidToVtkIjkGridRepresentation[uuid]->getICellCount(uuid);
			}
		}
	}
	return result;
}

// ----------------------------------------------------------------------------
int VtkEpcDocument::getJCellCount(const std::string & uuid)
{
	long result = 0;
	if (uuidIsChildOf[uuid].getType() == VtkEpcCommon::Resqml2Type::IJK_GRID) {
		result = uuidToVtkIjkGridRepresentation[uuidIsChildOf[uuid].getUuid()]->getJCellCount(uuid);
	}
	else if (uuidIsChildOf[uuid].getType() == VtkEpcCommon::Resqml2Type::SUB_REP) {
		if (uuidIsChildOf[uuid].getParentType() == VtkEpcCommon::Resqml2Type::IJK_GRID) {
			result = uuidToVtkIjkGridRepresentation[uuidIsChildOf[uuid].getUuid()]->getJCellCount(uuid);
		}
		if (uuidIsChildOf[uuid].getParentType() == VtkEpcCommon::Resqml2Type::PARTIAL) {
			auto uuidPartial = uuidIsChildOf[uuid].getParent();

			if (uuidIsChildOf[uuidPartial].getParentType() == VtkEpcCommon::Resqml2Type::IJK_GRID) {
				result = uuidToVtkIjkGridRepresentation[uuid]->getJCellCount(uuid);
			}
		}
	}
	return result;
}

// ----------------------------------------------------------------------------
int VtkEpcDocument::getKCellCount(const std::string & uuid)
{
	long result = 0;
	if (uuidIsChildOf[uuid].getType() == VtkEpcCommon::Resqml2Type::IJK_GRID) {
		result = uuidToVtkIjkGridRepresentation[uuidIsChildOf[uuid].getUuid()]->getKCellCount(uuid);
	}
	else if (uuidIsChildOf[uuid].getType() == VtkEpcCommon::Resqml2Type::SUB_REP) {
		if (uuidIsChildOf[uuid].getParentType() == VtkEpcCommon::Resqml2Type::IJK_GRID) {
			result = uuidToVtkIjkGridRepresentation[uuidIsChildOf[uuid].getUuid()]->getKCellCount(uuid);
		}
		if (uuidIsChildOf[uuid].getParentType() == VtkEpcCommon::Resqml2Type::PARTIAL) {
			auto uuidPartial = uuidIsChildOf[uuid].getParent();

			if (uuidIsChildOf[uuidPartial].getParentType() == VtkEpcCommon::Resqml2Type::IJK_GRID) {
				result = uuidToVtkIjkGridRepresentation[uuid]->getKCellCount(uuid);
			}
		}
	}
	return result;
}

// ----------------------------------------------------------------------------
int VtkEpcDocument::getInitKIndex(const std::string & uuid)
{
	long result = 0;
	if (uuidIsChildOf[uuid].getType() == VtkEpcCommon::Resqml2Type::IJK_GRID) {
		result = uuidToVtkIjkGridRepresentation[uuidIsChildOf[uuid].getUuid()]->getInitKIndex(uuid);
	}
	else if (uuidIsChildOf[uuid].getType() == VtkEpcCommon::Resqml2Type::SUB_REP) {
		if (uuidIsChildOf[uuid].getParentType() == VtkEpcCommon::Resqml2Type::IJK_GRID) {
			result = uuidToVtkIjkGridRepresentation[uuidIsChildOf[uuid].getUuid()]->getInitKIndex(uuid);
		}
		if (uuidIsChildOf[uuid].getParentType() == VtkEpcCommon::Resqml2Type::PARTIAL) {
			auto uuidPartial = uuidIsChildOf[uuid].getParent();

			if (uuidIsChildOf[uuidPartial].getParentType() == VtkEpcCommon::Resqml2Type::IJK_GRID) {
				result = uuidToVtkIjkGridRepresentation[uuid]->getInitKIndex(uuid);
			}
		}
	}
	return result;
}

// ----------------------------------------------------------------------------
VtkEpcCommon::Resqml2Type VtkEpcDocument::getType(std::string uuid)
{
	return uuidIsChildOf[uuid].getType();
}

// ----------------------------------------------------------------------------
VtkEpcCommon VtkEpcDocument::getInfoUuid(std::string uuid)
{
	return uuidIsChildOf[uuid];
}

// ----------------------------------------------------------------------------
const COMMON_NS::DataObjectRepository* VtkEpcDocument::getDataObjectRepository() const
{
	return repository;
}

// ----------------------------------------------------------------------------
std::vector<std::string> VtkEpcDocument::getListUuid()
{
	return uuidRep;
}

// ----------------------------------------------------------------------------
std::vector<VtkEpcCommon const *> VtkEpcDocument::getAllVtkEpcCommons() const
{
	std::vector<VtkEpcCommon const *> result;

	for (const std::string& iter : uuidRep) {
		result.push_back(&uuidIsChildOf.at(iter));
	}

	return result;
}

// ----------------------------------------------------------------------------
std::string VtkEpcDocument::getError()
{
	return epc_error;
}

// PRIVATE

void VtkEpcDocument::searchFaultPolylines(const std::string & fileName) {
	std::vector<RESQML2_NS::PolylineSetRepresentation*> polylines;
	try	{
		polylines = repository->getFaultPolylineSetRepSet();
	}
	catch  (const std::exception & e) {
		cout << "EXCEPTION in fesapi when calling getFaultPolylineSetRepSet with file: " << fileName << " : " << e.what();
	}
	for (size_t idx = 0; idx < polylines.size(); ++idx)	{
		if (polylines[idx]->isPartial()) {
			auto vtkEpcSrc = epcSet->getVtkEpcDocument(polylines[idx]->getUuid());
			if (vtkEpcSrc != nullptr) {
				auto type_in_epcdoc = epcSet->getTypeInEpcDocument(polylines[idx]->getUuid());
				if (type_in_epcdoc == VtkEpcCommon::Resqml2Type::POLYLINE_SET){
					createTreeVtkPartialRep(polylines[idx]->getUuid(), vtkEpcSrc);
					uuidIsChildOf[polylines[idx]->getUuid()].setParentType( VtkEpcCommon::Resqml2Type::POLYLINE_SET);
				}
				else {
					epc_error = epc_error + " Partial UUID (" + polylines[idx]->getUuid() + ") is PolylineSet and is incorrect \n";
					continue;
				}
			}
			else {
				epc_error = epc_error + " Partial UUID: (" + polylines[idx]->getUuid() + ") is not loaded \n";
				continue;
			}
		}
		else {
			std::string uuidParent= fileName;
			const auto interpretation = polylines[idx]->getInterpretation();
			if (interpretation != nullptr) {
				uuidParent = interpretation->getUuid();
				createTreeVtk(uuidParent, fileName, interpretation->getTitle().c_str(), VtkEpcCommon::Resqml2Type::INTERPRETATION_2D);
			}
			createTreeVtk(polylines[idx]->getUuid(), uuidParent, polylines[idx]->getTitle().c_str(), VtkEpcCommon::Resqml2Type::POLYLINE_SET);
		}

		//property
		for (const auto prop : polylines[idx]->getValuesPropertySet()) {
			createTreeVtk(prop->getUuid(), polylines[idx]->getUuid(), prop->getTitle().c_str(), VtkEpcCommon::Resqml2Type::PROPERTY);
		}
	}
}

void VtkEpcDocument::searchHorizonPolylines(const std::string & fileName) {
	std::vector<RESQML2_NS::PolylineSetRepresentation*> polylines;
	try	{
		polylines = repository->getHorizonPolylineSetRepSet();
	}
	catch  (const std::exception & e) {
		cout << "EXCEPTION in fesapi when calling getHorizonPolylineSetRepSet with file: " << fileName << " : " << e.what();
	}
	for (size_t idx = 0; idx < polylines.size(); ++idx)	{
		if (polylines[idx]->isPartial()) {
			auto vtkEpcSrc = epcSet->getVtkEpcDocument(polylines[idx]->getUuid());
			if (vtkEpcSrc)	{
				auto type_in_epcdoc = epcSet->getTypeInEpcDocument(polylines[idx]->getUuid());
				if (type_in_epcdoc == VtkEpcCommon::Resqml2Type::POLYLINE_SET){
					createTreeVtkPartialRep(polylines[idx]->getUuid(), vtkEpcSrc);
					uuidIsChildOf[polylines[idx]->getUuid()].setParentType( VtkEpcCommon::Resqml2Type::POLYLINE_SET);
				}
				else {
					epc_error = epc_error + " Partial UUID (" + polylines[idx]->getUuid() + ") is PolylineSet and is incorrect \n";
					continue;
				}
			}
			else {
				epc_error = epc_error + " Partial UUID: (" + polylines[idx]->getUuid() + ") is not loaded \n";
				continue;
			}
		} else {
			std::string uuidParent = fileName;
			const auto interpretation = polylines[idx]->getInterpretation();
			if (interpretation) {
				uuidParent = interpretation->getUuid();
				createTreeVtk(uuidParent, fileName, interpretation->getTitle().c_str(), VtkEpcCommon::Resqml2Type::INTERPRETATION_2D);
			}
			createTreeVtk(polylines[idx]->getUuid(), uuidParent, polylines[idx]->getTitle().c_str(), VtkEpcCommon::Resqml2Type::POLYLINE_SET);
		}

		//property
		for (const auto prop : polylines[idx]->getValuesPropertySet()) {
			createTreeVtk(prop->getUuid(), polylines[idx]->getUuid(), prop->getTitle().c_str(), VtkEpcCommon::Resqml2Type::PROPERTY);
		}
	}
}

void VtkEpcDocument::searchUnstructuredGrid(const std::string & fileName) {
	for (RESQML2_NS::UnstructuredGridRepresentation* unstructuredGrid : repository->getUnstructuredGridRepresentationSet()) {
		if (unstructuredGrid->isPartial())	{
			auto vtkEpcSrc = epcSet->getVtkEpcDocument(unstructuredGrid->getUuid());
			if (vtkEpcSrc){
				auto type_in_epcdoc = epcSet->getTypeInEpcDocument(unstructuredGrid->getUuid());
				if (type_in_epcdoc == VtkEpcCommon::Resqml2Type::UNSTRUC_GRID){
					createTreeVtkPartialRep(unstructuredGrid->getUuid(), vtkEpcSrc);
					uuidIsChildOf[unstructuredGrid->getUuid()].setParentType( VtkEpcCommon::Resqml2Type::UNSTRUC_GRID);
				}
				else {
					epc_error = epc_error + " Partial UUID (" + unstructuredGrid->getUuid() + ") is UnstructuredGrid type and is incorrect \n";
					continue;
				}
			}
			else {
				epc_error = epc_error + " Partial UUID: (" + unstructuredGrid->getUuid() + ") is not loaded \n";
				continue;
			}
		}
		else {
			std::string uuidParent = fileName;
			const auto interpretation = unstructuredGrid->getInterpretation();
			if (interpretation != nullptr) {
				uuidParent = interpretation->getUuid();
				createTreeVtk(uuidParent, fileName, interpretation->getTitle().c_str(), VtkEpcCommon::Resqml2Type::INTERPRETATION_3D);
			}
			createTreeVtk(unstructuredGrid->getUuid(), uuidParent, unstructuredGrid->getTitle().c_str(), VtkEpcCommon::Resqml2Type::UNSTRUC_GRID);
		}

		//properties
		for (const auto prop : unstructuredGrid->getValuesPropertySet()) {
			createTreeVtk(prop->getUuid(), unstructuredGrid->getUuid(), prop->getTitle().c_str(), VtkEpcCommon::Resqml2Type::PROPERTY);
		}
	}
}

void VtkEpcDocument::searchTriangulated(const std::string & fileName) {
	std::vector<RESQML2_NS::TriangulatedSetRepresentation*> triangulated;
	try	{
		triangulated = repository->getAllTriangulatedSetRepSet();
	} catch  (const std::exception & e)	{
		cout << "EXCEPTION in fesapi when calling getAllTriangulatedSetRepSet with file: " << fileName << " : " << e.what();
	}
	for (size_t iter = 0; iter < triangulated.size(); ++iter)	{
		if (triangulated[iter]->isPartial()) {
			auto vtkEpcSrc = epcSet->getVtkEpcDocument(triangulated[iter]->getUuid());
			if (vtkEpcSrc)	{
				auto type_in_epcdoc = epcSet->getTypeInEpcDocument(triangulated[iter]->getUuid());
				if (type_in_epcdoc == VtkEpcCommon::Resqml2Type::TRIANGULATED_SET){
					createTreeVtkPartialRep(triangulated[iter]->getUuid(), vtkEpcSrc);
					uuidIsChildOf[triangulated[iter]->getUuid()].setParentType( VtkEpcCommon::Resqml2Type::TRIANGULATED_SET);
				}
				else {
					epc_error = epc_error + " Partial UUID (" + triangulated[iter]->getUuid() + ") is Triangulated type and is incorrect \n";
					continue;
				}
			}
			else {
				epc_error = epc_error + " Partial UUID: (" + triangulated[iter]->getUuid() + ") is not loaded \n";
				continue;
			}
		}
		else {
			std::string uuidParent= fileName;
			const auto interpretation = triangulated[iter]->getInterpretation();
			if (interpretation) {
				uuidParent = interpretation->getUuid();
				createTreeVtk(uuidParent, fileName, interpretation->getTitle().c_str(), VtkEpcCommon::Resqml2Type::INTERPRETATION_2D);
			}
			createTreeVtk(triangulated[iter]->getUuid(), uuidParent, triangulated[iter]->getTitle().c_str(), VtkEpcCommon::Resqml2Type::TRIANGULATED_SET);
		}
		//property
		for (const auto prop : triangulated[iter]->getValuesPropertySet()) {
			createTreeVtk(prop->getUuid(), triangulated[iter]->getUuid(), prop->getTitle().c_str(), VtkEpcCommon::Resqml2Type::PROPERTY);
		}
	}
}

void VtkEpcDocument::searchGrid2d(const std::string & fileName) {
	std::vector<RESQML2_NS::Grid2dRepresentation*> grid2D;
	try	{
		grid2D = repository->getHorizonGrid2dRepSet();
	}
	catch  (const std::exception & e)	{
		cout << "EXCEPTION in fesapi when calling getHorizonGrid2dRepSet with file: " << fileName << " : " << e.what();
	}
	for (size_t iter = 0; iter < grid2D.size(); ++iter)	{
		if (grid2D[iter]->isPartial())	{
			auto vtkEpcSrc = epcSet->getVtkEpcDocument(grid2D[iter]->getUuid());
			if (vtkEpcSrc) {
				auto type_in_epcdoc = epcSet->getTypeInEpcDocument(grid2D[iter]->getUuid());
				if (type_in_epcdoc == VtkEpcCommon::Resqml2Type::GRID_2D){
					createTreeVtkPartialRep(grid2D[iter]->getUuid(), vtkEpcSrc);
					uuidIsChildOf[grid2D[iter]->getUuid()].setParentType( VtkEpcCommon::Resqml2Type::GRID_2D);
				}
				else {
					epc_error = epc_error + " Partial UUID (" + grid2D[iter]->getUuid() + ") is Grid2d type and is incorrect \n";
					continue;
				}
			}
			else {
				epc_error = epc_error + " Partial UUID: (" + grid2D[iter]->getUuid() + ") is not loaded \n";
				continue;
			}
		} 
		else {
			const auto interpretation = grid2D[iter]->getInterpretation();
			std::string uuidParent = fileName;
			if (interpretation) {
				uuidParent = interpretation->getUuid();
				createTreeVtk(uuidParent, fileName, interpretation->getTitle().c_str(), VtkEpcCommon::Resqml2Type::INTERPRETATION_2D);
			}
			createTreeVtk(grid2D[iter]->getUuid(), uuidParent, grid2D[iter]->getTitle().c_str(), VtkEpcCommon::Resqml2Type::GRID_2D);
		}
		//property
		for (const auto prop : grid2D[iter]->getValuesPropertySet()) {
			createTreeVtk(prop->getUuid(), grid2D[iter]->getUuid(), prop->getTitle().c_str(), VtkEpcCommon::Resqml2Type::PROPERTY);
		}
	}
}

void VtkEpcDocument::searchIjkGrid(const std::string & fileName) {
	std::vector<RESQML2_NS::AbstractIjkGridRepresentation*> ijkGrid;
	try	{
		ijkGrid = repository->getIjkGridRepresentationSet();
	}
	catch (const std::exception & e) {
		cout << "EXCEPTION in fesapi when calling getIjkGridRepresentationSet with file: " << fileName << " : " << e.what();
	}
	for (size_t iter = 0; iter < ijkGrid.size(); ++iter) {
		if (ijkGrid[iter]->isPartial())	{
			auto vtkEpcSrc = epcSet->getVtkEpcDocument(ijkGrid[iter]->getUuid());
			if (vtkEpcSrc)	{
				auto type_in_epcdoc = epcSet->getTypeInEpcDocument(ijkGrid[iter]->getUuid());
				if (type_in_epcdoc == VtkEpcCommon::Resqml2Type::IJK_GRID){
					createTreeVtkPartialRep(ijkGrid[iter]->getUuid(), vtkEpcSrc);
					uuidIsChildOf[ijkGrid[iter]->getUuid()].setParentType(VtkEpcCommon::Resqml2Type::IJK_GRID);
				}
				else {
					epc_error = epc_error + " Partial UUID (" + ijkGrid[iter]->getUuid() + ") is IjkGrid type and is incorrect \n";
					continue;
				}
			}
			else {
				epc_error = epc_error + " Partial UUID: (" + ijkGrid[iter]->getUuid() + ") is not loaded \n";
				continue;
			}
		}
		else {
			const auto interpretation = ijkGrid[iter]->getInterpretation();
			std::string uuidParent = fileName;
			if (interpretation) {
				uuidParent = interpretation->getUuid();
				createTreeVtk(uuidParent, fileName, interpretation->getTitle().c_str(), VtkEpcCommon::Resqml2Type::INTERPRETATION_3D);
			}
			createTreeVtk(ijkGrid[iter]->getUuid(), uuidParent, ijkGrid[iter]->getTitle().c_str(), VtkEpcCommon::Resqml2Type::IJK_GRID);
		}

		//property
		for (const auto prop : ijkGrid[iter]->getValuesPropertySet()) {
			createTreeVtk(prop->getUuid(), ijkGrid[iter]->getUuid(), prop->getTitle().c_str(), VtkEpcCommon::Resqml2Type::PROPERTY);
		}
	}
}

void VtkEpcDocument::searchWellboreTrajectory(const std::string & fileName) {
	for (const auto wellboreTrajectory: repository->getWellboreTrajectoryRepresentationSet()) {
		if (wellboreTrajectory->isPartial()) {
			auto vtkEpcSrc = epcSet->getVtkEpcDocument(wellboreTrajectory->getUuid());
			if (vtkEpcSrc != nullptr)	{
				auto type_in_epcdoc = epcSet->getTypeInEpcDocument(wellboreTrajectory->getUuid());
				if (type_in_epcdoc == VtkEpcCommon::Resqml2Type::WELL_TRAJ){
					createTreeVtkPartialRep(wellboreTrajectory->getUuid(), vtkEpcSrc);
					uuidIsChildOf[wellboreTrajectory->getUuid()].setParentType(VtkEpcCommon::Resqml2Type::WELL_TRAJ);
				}
				else {
					epc_error = epc_error + " Partial UUID (" + wellboreTrajectory->getUuid() + ") is WellboreTrajectory type and is incorrect\n";
					continue;
				}
			}
			else {
				epc_error = epc_error + " Partial UUID: (" + wellboreTrajectory->getUuid() + ") is not loaded\n";
				continue;
			}
		}
		else {
			auto interpretation = wellboreTrajectory->getInterpretation();
			std::string uuidParent = fileName;
			if (interpretation != nullptr) {
				uuidParent = interpretation->getUuid();
				createTreeVtk(uuidParent, fileName, interpretation->getTitle().c_str(), VtkEpcCommon::Resqml2Type::INTERPRETATION_1D);
			}
			createTreeVtk(wellboreTrajectory->getUuid(), uuidParent, wellboreTrajectory->getTitle().c_str(), VtkEpcCommon::Resqml2Type::WELL_TRAJ);
		}

		//wellboreFrame
		for (const auto wellboreFrame: wellboreTrajectory->getWellboreFrameRepresentationSet()) {
			const auto wellboreMarkerFrame = dynamic_cast<RESQML2_NS::WellboreMarkerFrameRepresentation*>(wellboreFrame);
			if (wellboreMarkerFrame == nullptr) { // WellboreFrame
				createTreeVtk(wellboreFrame->getUuid(), wellboreTrajectory->getUuid(), wellboreFrame->getTitle().c_str(), VtkEpcCommon::Resqml2Type::WELL_FRAME);
				for (const auto property: wellboreFrame->getValuesPropertySet()) {
					createTreeVtk(property->getUuid(), wellboreFrame->getUuid(), property->getTitle().c_str(), VtkEpcCommon::Resqml2Type::PROPERTY);
				}
			}
			else { // WellboreMarkerFrame
				createTreeVtk(wellboreFrame->getUuid(), wellboreTrajectory->getUuid(), wellboreFrame->getTitle().c_str(), VtkEpcCommon::Resqml2Type::WELL_MARKER_FRAME);
				for (const auto wellboreMarker: wellboreMarkerFrame->getWellboreMarkerSet()) {
					createTreeVtk(wellboreMarker->getUuid(), wellboreMarkerFrame->getUuid(), wellboreMarker->getTitle().c_str(), VtkEpcCommon::Resqml2Type::WELL_MARKER);
				}
			}
		}
	}
}

void VtkEpcDocument::searchSubRepresentation(const std::string & fileName) {
	std::vector<RESQML2_NS::SubRepresentation*> subRepresentationSet;
	try		{
		subRepresentationSet = repository->getSubRepresentationSet();
	}
	catch  (const std::exception & e)		{
		epc_error = epc_error + "EXCEPTION in fesapi when calling getSubRepresentationSet with file: " + fileName + " : " + e.what() + ".\n";
	}
	for (const auto subRepresentation : subRepresentationSet) {
		if (subRepresentation->isPartial()){
			auto vtkEpcSrc = epcSet->getVtkEpcDocument(subRepresentation->getUuid());
			if (vtkEpcSrc)	{
				createTreeVtkPartialRep(subRepresentation->getUuid(), vtkEpcSrc);
				uuidIsChildOf[subRepresentation->getUuid()].setParentType( VtkEpcCommon::Resqml2Type::SUB_REP);
			}
			else {
				epc_error = epc_error + " Partial subrepresentation \"" + subRepresentation->getTitle() + "\" with UUID " + subRepresentation->getUuid() + " is not loaded. Please load it first.\n";
				continue;
			}
		}
		else {
			if (subRepresentation->getSupportingRepresentationCount() != 1) {
				epc_error = epc_error + "Cannot import a subrepresentation which does not rely on a single IJK grid for now.\n";
				continue;
			}

			auto uuidParent = subRepresentation->getSupportingRepresentationDor(0).getUuid();
			createTreeVtk(subRepresentation->getUuid(), uuidParent, subRepresentation->getTitle().c_str(), VtkEpcCommon::Resqml2Type::SUB_REP);
		}

		//property
		for (const auto prop : subRepresentation->getValuesPropertySet()) {
			createTreeVtk(prop->getUuid(), subRepresentation->getUuid(), prop->getTitle().c_str(), VtkEpcCommon::Resqml2Type::PROPERTY);
		}
	}
}

void VtkEpcDocument::searchTimeSeries(const std::string & fileName) {
	std::vector<EML2_NS::TimeSeries*> timeSeries;
	try	{
		timeSeries = repository->getTimeSeriesSet();
	}
	catch  (const std::exception & e)	{
		cout << "EXCEPTION in fesapi when calling getTimeSeriesSet with file: " << fileName << " : " << e.what();
	}

	for (const auto timeSerie : timeSeries) {
		auto propSeries = timeSerie->getPropertySet();
		for (const auto prop : propSeries) {
			if (prop->getXmlTag() != "ContinuousPropertySeries") {
				auto prop_uuid = prop->getUuid();
				if (uuidIsChildOf.find(prop_uuid) == uuidIsChildOf.end()) {
					std::cout << "The property " << prop_uuid << " is not supported and consequently cannot be associated to its time series." << std::endl;
					continue;
				}
				if (prop->getTimeIndicesCount() > 1) {
					std::cout << "The property " << prop_uuid << " correspond ot more than one time index. It is not supported yet." << std::endl;
					continue;
				}
				uuidIsChildOf[prop_uuid].setType(VtkEpcCommon::Resqml2Type::TIME_SERIES);
				uuidIsChildOf[prop_uuid].setTimeIndex(prop->getTimeIndexStart());
				uuidIsChildOf[prop_uuid].setTimestamp(prop->getTimeSeries()->getTimestamp(prop->getTimeIndexStart()));
				/*
			if (propSeries[i]->isAssociatedToOneStandardEnergisticsPropertyKind())
			{
				auto propKind = propSeries[i]->getEnergisticsPropertyKind();
			}
			else
			{
				auto propkindUuid = propSeries[i]->getLocalPropertyKindUuid();
				cout << "name : " << propSeries[i]->getTitle() << " - " <<propkindUuid << "\n";
			}
				 */
			}

		}
	}
}
