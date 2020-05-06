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
#include <fesapi/common/EpcDocument.h>
#include <fesapi/resqml2_0_1/PolylineSetRepresentation.h>
#include <fesapi/resqml2_0_1/TriangulatedSetRepresentation.h>
#include <fesapi/resqml2_0_1/Horizon.h>
#include <fesapi/resqml2_0_1/TectonicBoundaryFeature.h>
#include <fesapi/resqml2_0_1/HorizonInterpretation.h>
#include <fesapi/resqml2_0_1/FaultInterpretation.h>
#include <fesapi/resqml2_0_1/PointSetRepresentation.h>
#include <fesapi/resqml2_0_1/Grid2dRepresentation.h>
#include <fesapi/common/AbstractObject.h>
#include <fesapi/resqml2_0_1/SubRepresentation.h>
#include <fesapi/resqml2_0_1/AbstractIjkGridRepresentation.h>
#include <fesapi/resqml2/AbstractValuesProperty.h>
#include <fesapi/resqml2_0_1/UnstructuredGridRepresentation.h>
#include <fesapi/resqml2_0_1/WellboreTrajectoryRepresentation.h>
#include <fesapi/resqml2_0_1/PropertyKindMapper.h>
#include <fesapi/resqml2_0_1/SubRepresentation.h>
#include <fesapi/resqml2_0_1/TimeSeries.h>

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
	std::string resqmlResult = pck.deserializeInto(repository);
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

	for (auto &iter : uuidRep)	{
		treeView.push_back(uuidIsChildOf[iter]);
	}
}

// ----------------------------------------------------------------------------
VtkEpcDocument::~VtkEpcDocument()
{
	for(auto i : uuidToVtkGrid2DRepresentation) {
		delete i.second;
	}
	uuidToVtkGrid2DRepresentation.clear();

	for(auto i : uuidToVtkPolylineRepresentation) {
		delete i.second;
	}
	uuidToVtkPolylineRepresentation.clear();

	for(auto i : uuidToVtkTriangulatedRepresentation) {
		delete i.second;
	}
	uuidToVtkTriangulatedRepresentation.clear();

	for(auto i : uuidToVtkSetPatch) {
		delete i.second;
	}
	uuidToVtkSetPatch.clear();

	for(auto i : uuidToVtkWellboreTrajectoryRepresentation) {
		delete i.second;
	}
	uuidToVtkWellboreTrajectoryRepresentation.clear();

	for(auto i : uuidToVtkIjkGridRepresentation) {
		delete i.second;
	}
	uuidToVtkIjkGridRepresentation.clear();

	for(auto i : uuidToVtkUnstructuredGridRepresentation) {
		delete i.second;
	}
	uuidToVtkUnstructuredGridRepresentation.clear();

	for(auto i : uuidToVtkPartialRepresentation) {
		delete i.second;
	}
	uuidToVtkPartialRepresentation.clear();

	uuidPartialRep.clear();
	uuidRep.clear();

	epcSet = nullptr;
}

// ----------------------------------------------------------------------------
void VtkEpcDocument::createTreeVtk(const std::string & uuid, const std::string & parent, const std::string & name, VtkEpcCommon::Resqml2Type type)
{
	int return_code = 1;
	uuidIsChildOf[uuid].setType( type);
	uuidIsChildOf[uuid].setUuid(uuid);
	uuidIsChildOf[uuid].setParent(parent);
	uuidIsChildOf[uuid].setName(name);
	uuidIsChildOf[uuid].setTimeIndex(-1);
	uuidIsChildOf[uuid].setTimestamp(0);

	if(uuidIsChildOf[parent].getUuid().empty()) {
		if (type == VtkEpcCommon::Resqml2Type::GRID_2D ||
			type == VtkEpcCommon::Resqml2Type::POLYLINE_SET ||
			type == VtkEpcCommon::Resqml2Type::TRIANGULATED_SET) {
			uuidIsChildOf[uuid].setParentType( VtkEpcCommon::INTERPRETATION_2D);
		}
		else if (type == VtkEpcCommon::Resqml2Type::WELL_TRAJ) {
			uuidIsChildOf[uuid].setParentType( VtkEpcCommon::INTERPRETATION_1D);
		}
		else if (type == VtkEpcCommon::Resqml2Type::IJK_GRID ||
			type == VtkEpcCommon::Resqml2Type::UNSTRUC_GRID ||
			type == VtkEpcCommon::Resqml2Type::SUB_REP) {
			uuidIsChildOf[uuid].setParentType( VtkEpcCommon::INTERPRETATION_3D);
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
		case VtkEpcCommon::Resqml2Type::WELL_FRAME: {
			addWellTrajTreeVtk(uuid, parent, name);
			break;
		}
		case VtkEpcCommon::Resqml2Type::WELL_MARKER: {
			addWellTrajTreeVtk(uuid, parent, name);
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
	uuidToVtkGrid2DRepresentation[uuid] = new VtkGrid2DRepresentation(getFileName(), name, uuid, parent, &repository, nullptr);
}

// ----------------------------------------------------------------------------
void VtkEpcDocument::addPolylineSetTreeVtk(const std::string & uuid, const std::string & parent, const std::string & name)
{
	if (repository.getDataObjectByUuid(uuid)->getXmlTag() == "PolylineRepresentation") {
		uuidToVtkPolylineRepresentation[uuid] = new VtkPolylineRepresentation(getFileName(), name, uuid, parent, 0, &repository, nullptr);
	}
	else {
		uuidToVtkSetPatch[uuid] = new VtkSetPatch(getFileName(), name, uuid, parent, &repository, getIdProc(), getMaxProc());
	}
}

// ----------------------------------------------------------------------------
void VtkEpcDocument::addTriangulatedSetTreeVtk(const std::string & uuid, const std::string & parent, const std::string & name)
{
	if (repository.getDataObjectByUuid(uuid)->getXmlTag() == "TriangulatedRepresentation")	{
		uuidToVtkTriangulatedRepresentation[uuid] = new VtkTriangulatedRepresentation(getFileName(), name, uuid, parent, 0, &repository, nullptr);
	}
	else {
		uuidToVtkSetPatch[uuid] = new VtkSetPatch(getFileName(), name, uuid, parent, &repository, getIdProc(), getMaxProc());
	}
}

// ----------------------------------------------------------------------------
void VtkEpcDocument::addWellTrajTreeVtk(const std::string & uuid, const std::string & parent, const std::string & name)
{
	uuidToVtkWellboreTrajectoryRepresentation[uuid] = new VtkWellboreTrajectoryRepresentation(getFileName(), name, uuid, parent, &repository, nullptr);
}

// ----------------------------------------------------------------------------
void VtkEpcDocument::addWellFrameTreeVtk(const std::string & uuid, const std::string & parent, const std::string & name)
{
	uuidToVtkWellboreTrajectoryRepresentation[parent]->addWellboreFrame(uuid);
}
// ----------------------------------------------------------------------------
void VtkEpcDocument::addWellMarkerTreeVtk(const std::string & uuid, const std::string & parent, const std::string & name)
{
	uuidToVtkWellboreTrajectoryRepresentation[parent]->addWellboreMarker(uuid);
}

// ----------------------------------------------------------------------------
void VtkEpcDocument::addIjkGridTreeVtk(const std::string & uuid, const std::string & parent, const std::string & name)
{
	uuidToVtkIjkGridRepresentation[uuid] = new VtkIjkGridRepresentation(getFileName(), name, uuid, parent, &repository, nullptr, getIdProc(), getMaxProc());
}

// ----------------------------------------------------------------------------
void VtkEpcDocument::addUnstrucGridTreeVtk(const std::string & uuid, const std::string & parent, const std::string & name)
{
	uuidToVtkUnstructuredGridRepresentation[uuid] = new VtkUnstructuredGridRepresentation(getFileName(), name, uuid, parent, &repository, nullptr);
}

// ----------------------------------------------------------------------------
int VtkEpcDocument::addSubRepTreeVtk(const std::string & uuid, const std::string & parent, const std::string & name)
{
	if (uuidIsChildOf[uuid].getParentType() == VtkEpcCommon::IJK_GRID)	{
		uuidToVtkIjkGridRepresentation[uuid] = new VtkIjkGridRepresentation(getFileName(), name, uuid, parent, &repository, &repository);
		return 1;
	}
	else if (uuidIsChildOf[uuid].getParentType() == VtkEpcCommon::UNSTRUC_GRID)	{
		uuidToVtkUnstructuredGridRepresentation[uuid] = new VtkUnstructuredGridRepresentation(getFileName(), name, uuid, parent, &repository, &repository);
		return 1;
	}
	else if (uuidIsChildOf[uuid].getParentType() == VtkEpcCommon::PARTIAL)	{
		auto parentUuidType = uuidIsChildOf[parent].getParentType();
		auto pckEPCsrc = uuidToVtkPartialRepresentation[parent]->getEpcSource();

		switch (parentUuidType)	{
		case VtkEpcCommon::GRID_2D:	{
			uuidToVtkGrid2DRepresentation[uuid] = new VtkGrid2DRepresentation(getFileName(), name, uuid, parent, pckEPCsrc, &repository);
			return 1;
			break;
		}
		case VtkEpcCommon::WELL_TRAJ: {
			uuidToVtkWellboreTrajectoryRepresentation[uuid] = new VtkWellboreTrajectoryRepresentation(getFileName(), name, uuid, parent, pckEPCsrc, &repository);
			return 1;
			break;
		}
		case VtkEpcCommon::IJK_GRID: {
			uuidToVtkIjkGridRepresentation[uuid] = new VtkIjkGridRepresentation(getFileName(), name, uuid, parent, pckEPCsrc, &repository);
			return 1;
			break;
		}
		case VtkEpcCommon::UNSTRUC_GRID: {
			uuidToVtkUnstructuredGridRepresentation[uuid] = new VtkUnstructuredGridRepresentation(getFileName(), name, uuid, parent, pckEPCsrc, &repository);
			return 1;
			break;
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
	case VtkEpcCommon::GRID_2D:	{
		uuidToVtkGrid2DRepresentation[parent]->createTreeVtk(uuid, parent, name, uuidIsChildOf[uuid].getType());
		return 1;
		break;
	}
	case VtkEpcCommon::POLYLINE_SET: {
		if (repository.getDataObjectByUuid(parent)->getXmlTag() == "PolylineRepresentation")	{
			uuidToVtkPolylineRepresentation[parent]->createTreeVtk(uuid, parent, name, uuidIsChildOf[uuid].getType());
			return 1;
		}
		else {
			uuidToVtkSetPatch[parent]->createTreeVtk(uuid, parent, name, uuidIsChildOf[uuid].getType());
			return 1;
		}
		break;
	}
	case VtkEpcCommon::TRIANGULATED_SET: {
		if (repository.getDataObjectByUuid(uuid)->getXmlTag() == "TriangulatedRepresentation")	{
			uuidToVtkTriangulatedRepresentation[parent]->createTreeVtk(uuid, parent, name, uuidIsChildOf[uuid].getType());
			return 1;
		}
		else {
			uuidToVtkSetPatch[parent]->createTreeVtk(uuid, parent, name, uuidIsChildOf[uuid].getType());
			return 1;
		}
		break;
	}
	case VtkEpcCommon::WELL_TRAJ: {
		uuidToVtkWellboreTrajectoryRepresentation[parent]->createTreeVtk(uuid, parent, name, uuidIsChildOf[uuid].getType());
		return 1;
		break;
	}
	case VtkEpcCommon::WELL_FRAME: {
		uuidToVtkWellboreTrajectoryRepresentation[parent]->createTreeVtk(uuid, uuidIsChildOf[parent].getParent(), name, uuidIsChildOf[parent].getType());
		return 1;
		break;
	}
	case VtkEpcCommon::IJK_GRID: {
		uuidToVtkIjkGridRepresentation[parent]->createTreeVtk(uuid, parent, name, uuidIsChildOf[uuid].getType());
		return 1;
		break;
	}
	case VtkEpcCommon::UNSTRUC_GRID: {
		uuidToVtkUnstructuredGridRepresentation[parent]->createTreeVtk(uuid, parent, name, uuidIsChildOf[uuid].getType());
		return 1;
		break;
	}
	case VtkEpcCommon::SUB_REP: 	{
		if (uuidIsChildOf[parent].getParentType() == VtkEpcCommon::IJK_GRID) {
			uuidToVtkIjkGridRepresentation[parent]->createTreeVtk(uuid, parent, name, uuidIsChildOf[uuid].getType());
			return 1;
		}
		else if (uuidIsChildOf[parent].getParentType() == VtkEpcCommon::UNSTRUC_GRID) {
			uuidToVtkUnstructuredGridRepresentation[parent]->createTreeVtk(uuid, parent, name, uuidIsChildOf[uuid].getType());
			return 1;
		}
		else if (uuidIsChildOf[parent].getParentType() == VtkEpcCommon::PARTIAL) {
			auto uuidPartial = uuidIsChildOf[parent].getParent();

			switch (uuidIsChildOf[uuidPartial].getParentType()) {
			case VtkEpcCommon::GRID_2D:	{
				uuidToVtkGrid2DRepresentation[parent]->createTreeVtk(uuid, parent, name, uuidIsChildOf[uuid].getType());
				return 1;
				break;
			}
			case VtkEpcCommon::WELL_TRAJ:{
				uuidToVtkWellboreTrajectoryRepresentation[parent]->createTreeVtk(uuid, parent, name, uuidIsChildOf[uuid].getType());
				return 1;
				break;
			}
			case VtkEpcCommon::IJK_GRID: {
				uuidToVtkIjkGridRepresentation[parent]->createTreeVtk(uuid, parent, name, uuidIsChildOf[uuid].getType());
				return 1;
				break;
			}
			case VtkEpcCommon::UNSTRUC_GRID: {
				uuidToVtkUnstructuredGridRepresentation[parent]->createTreeVtk(uuid, parent, name, uuidIsChildOf[uuid].getType());
				return 1;
				break;
			}
			default:break;
			}
		}
		break;
	}
	case VtkEpcCommon::PARTIAL:	{
		uuidToVtkPartialRepresentation[uuidIsChildOf[parent].getUuid()]->createTreeVtk(uuid, parent, name, uuidIsChildOf[uuid].getType());
		return 1;
		break;
	}
	default: {
		cout << " parent type unknown " << uuidIsChildOf[uuid].getParentType() << endl;
		break;
	}
	}
	return 0;

}

// ----------------------------------------------------------------------------
void VtkEpcDocument::createTreeVtkPartialRep(const std::string & uuid, VtkEpcDocument *vtkEpcDowumentWithCompleteRep)
{
	uuidIsChildOf[uuid].setType( VtkEpcCommon::PARTIAL);
	uuidIsChildOf[uuid].setUuid(uuid);
	uuidToVtkPartialRepresentation[uuid] = new VtkPartialRepresentation(getFileName(), uuid, vtkEpcDowumentWithCompleteRep, &repository);
}

// ----------------------------------------------------------------------------
void VtkEpcDocument::visualize(const std::string & uuid)
{
	auto uuidToAttach = uuidIsChildOf[uuid].getUuid();
	switch (uuidIsChildOf[uuid].getType())	{
	case VtkEpcCommon::GRID_2D:	{
		uuidToVtkGrid2DRepresentation[uuidIsChildOf[uuid].getUuid()]->visualize(uuid);
		break;
	}
	case VtkEpcCommon::POLYLINE_SET: {
		auto object = repository.getDataObjectByUuid(uuidIsChildOf[uuid].getUuid());
		auto typeRepresentation = object->getXmlTag();
		if (typeRepresentation == "PolylineRepresentation")	{
			uuidToVtkPolylineRepresentation[uuidIsChildOf[uuid].getUuid()]->visualize(uuid);
		}
		else {
			uuidToVtkSetPatch[uuidIsChildOf[uuid].getUuid()]->visualize(uuid);
		}
		break;
	}
	case VtkEpcCommon::TRIANGULATED_SET: {
		auto object = repository.getDataObjectByUuid(uuidIsChildOf[uuid].getUuid());
		auto typeRepresentation = object->getXmlTag();
		if (typeRepresentation == "TriangulatedRepresentation") {
			uuidToVtkTriangulatedRepresentation[uuidIsChildOf[uuid].getUuid()]->visualize(uuid);
		}
		else {
			uuidToVtkSetPatch[uuidIsChildOf[uuid].getUuid()]->visualize(uuid);
		}
		break;
	}
	case VtkEpcCommon::WELL_TRAJ: {
		uuidToVtkWellboreTrajectoryRepresentation[uuidIsChildOf[uuid].getUuid()]->visualize(uuid);
		break;
	}
	case VtkEpcCommon::IJK_GRID: {
		uuidToVtkIjkGridRepresentation[uuidIsChildOf[uuid].getUuid()]->visualize(uuid);
		break;
	}
	case VtkEpcCommon::UNSTRUC_GRID: {
		uuidToVtkUnstructuredGridRepresentation[uuidIsChildOf[uuid].getUuid()]->visualize(uuid);
		break;
	}
	case VtkEpcCommon::SUB_REP:	{
		if (uuidIsChildOf[uuid].getParentType() == VtkEpcCommon::IJK_GRID) {
			uuidToVtkIjkGridRepresentation[uuidIsChildOf[uuid].getUuid()]->visualize(uuid);
		}
		else if (uuidIsChildOf[uuid].getParentType() == VtkEpcCommon::UNSTRUC_GRID) {
			uuidToVtkUnstructuredGridRepresentation[uuidIsChildOf[uuid].getUuid()]->visualize(uuid);
		}
		else if (uuidIsChildOf[uuid].getParentType() == VtkEpcCommon::PARTIAL) {
			auto parent = uuidIsChildOf[uuid].getParent();

			switch (uuidIsChildOf[parent].getParentType() ) {
			case VtkEpcCommon::GRID_2D: {
				uuidToVtkGrid2DRepresentation[uuid]->visualize(uuid);
				break;
			}
			case VtkEpcCommon::WELL_TRAJ: {
				uuidToVtkWellboreTrajectoryRepresentation[uuid]->visualize(uuid);
				break;
			}
			case VtkEpcCommon::IJK_GRID: {
				uuidToVtkIjkGridRepresentation[uuid]->visualize(uuid);
				break;
			}
			case VtkEpcCommon::UNSTRUC_GRID: {
				uuidToVtkUnstructuredGridRepresentation[uuid]->visualize(uuid);
				break;
			}
			default: break;
			}
		}
		break;
	}
	case VtkEpcCommon::PARTIAL:	{
		uuidToVtkPartialRepresentation[uuidIsChildOf[uuid].getUuid()]->visualize(uuid);
		break;
	}
	case (VtkEpcCommon::TIME_SERIES): {
		uuidToAttach = uuidIsChildOf[uuid].getParent();
		switch (uuidIsChildOf[uuid].getParentType()) {
		case VtkEpcCommon::GRID_2D: {
			uuidToVtkGrid2DRepresentation[uuidIsChildOf[uuid].getParent()]->visualize(uuid);
			break;
		}
		case VtkEpcCommon::POLYLINE_SET: {
			auto object = repository.getDataObjectByUuid(uuidIsChildOf[uuid].getParent());
			auto typeRepresentation = object->getXmlTag();
			if (typeRepresentation == "PolylineRepresentation")	{
				uuidToVtkPolylineRepresentation[uuidIsChildOf[uuid].getParent()]->visualize(uuid);
			}
			else {
				uuidToVtkSetPatch[uuidIsChildOf[uuid].getParent()]->visualize(uuid);
			}
			break;
		}
		case VtkEpcCommon::TRIANGULATED_SET: {
			auto object = repository.getDataObjectByUuid(uuid);
			auto typeRepresentation = object->getXmlTag();
			if (typeRepresentation == "TriangulatedRepresentation") {
				uuidToVtkTriangulatedRepresentation[uuidIsChildOf[uuid].getParent()]->visualize(uuid);
			}
			else {
				uuidToVtkSetPatch[uuidIsChildOf[uuid].getParent()]->visualize(uuid);
			}
			break;
		}
		case VtkEpcCommon::WELL_TRAJ: {
			uuidToVtkWellboreTrajectoryRepresentation[uuidIsChildOf[uuid].getParent()]->visualize(uuid);
			break;
		}
		case VtkEpcCommon::IJK_GRID: {
			uuidToVtkIjkGridRepresentation[uuidIsChildOf[uuid].getParent()]->visualize(uuid);
			break;
		}
		case VtkEpcCommon::UNSTRUC_GRID: {
			uuidToVtkUnstructuredGridRepresentation[uuidIsChildOf[uuid].getParent()]->visualize(uuid);
			break;
		}
		case VtkEpcCommon::SUB_REP: {
			if (uuidIsChildOf[uuidIsChildOf[uuid].getParent()].getParentType() == VtkEpcCommon::IJK_GRID)	{
				uuidToVtkIjkGridRepresentation[uuidIsChildOf[uuidIsChildOf[uuid].getParent()].getUuid()]->visualize(uuid);
			}
			else if (uuidIsChildOf[uuidIsChildOf[uuid].getParent()].getParentType() == VtkEpcCommon::UNSTRUC_GRID) {
				uuidToVtkUnstructuredGridRepresentation[uuidIsChildOf[uuidIsChildOf[uuid].getParent()].getUuid()]->visualize(uuid);
			}
			else if (uuidIsChildOf[uuidIsChildOf[uuid].getParent()].getParentType() == VtkEpcCommon::PARTIAL)	{
				if (uuidIsChildOf[uuidIsChildOf[uuidIsChildOf[uuid].getParent()].getParent()].getParentType() == VtkEpcCommon::IJK_GRID)	{
					uuidToVtkIjkGridRepresentation[uuidIsChildOf[uuidIsChildOf[uuid].getParent()].getUuid()]->visualize(uuid);
				}
				else if (uuidIsChildOf[uuidIsChildOf[uuidIsChildOf[uuid].getParent()].getParent()].getParentType() == VtkEpcCommon::UNSTRUC_GRID) {
					uuidToVtkUnstructuredGridRepresentation[uuidIsChildOf[uuidIsChildOf[uuid].getParent()].getUuid()]->visualize(uuid);
				}
			}
			break;
		}
		case VtkEpcCommon::PARTIAL: {
			uuidToVtkPartialRepresentation[uuidIsChildOf[uuid].getParent()]->visualize(uuid);
			break;
		}
		default:
			break;
		}
	}
	case VtkEpcCommon::PROPERTY: {
		uuidToAttach = uuidIsChildOf[uuid].getParent();
		switch (uuidIsChildOf[uuid].getParentType()) {
		case VtkEpcCommon::GRID_2D: {
			uuidToVtkGrid2DRepresentation[uuidIsChildOf[uuid].getParent()]->visualize(uuid);
			break;
		}
		case VtkEpcCommon::POLYLINE_SET: {
			auto object = repository.getDataObjectByUuid(uuidIsChildOf[uuid].getParent());
			auto typeRepresentation = object->getXmlTag();
			if (typeRepresentation == "PolylineRepresentation")	{
				uuidToVtkPolylineRepresentation[uuidIsChildOf[uuid].getParent()]->visualize(uuid);
			}
			else {
				uuidToVtkSetPatch[uuidIsChildOf[uuid].getParent()]->visualize(uuid);
			}
			break;
		}
		case VtkEpcCommon::TRIANGULATED_SET: {
			auto object = repository.getDataObjectByUuid(uuid);
			auto typeRepresentation = object->getXmlTag();
			if (typeRepresentation == "TriangulatedRepresentation")	{
				uuidToVtkTriangulatedRepresentation[uuidIsChildOf[uuid].getParent()]->visualize(uuid);
			}
			else {
				uuidToVtkSetPatch[uuidIsChildOf[uuid].getParent()]->visualize(uuid);
			}
			break;
		}
		case VtkEpcCommon::WELL_TRAJ: {
			uuidToVtkWellboreTrajectoryRepresentation[uuidIsChildOf[uuid].getParent()]->visualize(uuid);
			break;
		}
		case VtkEpcCommon::IJK_GRID: {
			uuidToVtkIjkGridRepresentation[uuidIsChildOf[uuid].getParent()]->visualize(uuid);
			break;
		}
		case VtkEpcCommon::UNSTRUC_GRID: {
			uuidToVtkUnstructuredGridRepresentation[uuidIsChildOf[uuid].getParent()]->visualize(uuid);
			break;
		}
		case VtkEpcCommon::SUB_REP: {
			if (uuidIsChildOf[uuidIsChildOf[uuid].getParent()].getParentType() == VtkEpcCommon::IJK_GRID)	{
				uuidToVtkIjkGridRepresentation[uuidIsChildOf[uuidIsChildOf[uuid].getParent()].getUuid()]->visualize(uuid);
			}
			else if (uuidIsChildOf[uuidIsChildOf[uuid].getParent()].getParentType() == VtkEpcCommon::UNSTRUC_GRID) {
				uuidToVtkUnstructuredGridRepresentation[uuidIsChildOf[uuidIsChildOf[uuid].getParent()].getUuid()]->visualize(uuid);
			}
			else if (uuidIsChildOf[uuidIsChildOf[uuid].getParent()].getParentType() == VtkEpcCommon::PARTIAL)	{
				if (uuidIsChildOf[uuidIsChildOf[uuidIsChildOf[uuid].getParent()].getParent()].getParentType() == VtkEpcCommon::IJK_GRID)	{
					uuidToVtkIjkGridRepresentation[uuidIsChildOf[uuidIsChildOf[uuid].getParent()].getUuid()]->visualize(uuid);
				}
				else if (uuidIsChildOf[uuidIsChildOf[uuidIsChildOf[uuid].getParent()].getParent()].getParentType() == VtkEpcCommon::UNSTRUC_GRID) {
					uuidToVtkUnstructuredGridRepresentation[uuidIsChildOf[uuidIsChildOf[uuid].getParent()].getUuid()]->visualize(uuid);
				}
			}
			break;
		}
		case VtkEpcCommon::PARTIAL:	{
			uuidToVtkPartialRepresentation[uuidIsChildOf[uuid].getParent()]->visualize(uuid);
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
	for (auto &vtkEpcCommon : treeView) {
		if (vtkEpcCommon.getType() == VtkEpcCommon::WELL_TRAJ) {
			uuidToVtkWellboreTrajectoryRepresentation[vtkEpcCommon.getUuid()]->visualize(vtkEpcCommon.getUuid());
			try
			{
				// attach representation to EpcDocument VtkMultiBlockDataSet
				if (std::find(attachUuids.begin(), attachUuids.end(), vtkEpcCommon.getUuid()) == attachUuids.end()) {
					detach();
					attachUuids.push_back(vtkEpcCommon.getUuid());
					attach();
				}
			}
			catch (const std::exception&)
			{
				cout << "ERROR with uuid attachment: " << vtkEpcCommon.getUuid() ;
			}
		}
	}
}

// ----------------------------------------------------------------------------
void VtkEpcDocument::unvisualizeFullWell()
{
	for (auto &vtkEpcCommon : treeView) {
		if (std::find(attachUuids.begin(), attachUuids.end(), vtkEpcCommon.getUuid()) != attachUuids.end()) {
			if (uuidIsChildOf[vtkEpcCommon.getUuid()].getType() == VtkEpcCommon::WELL_TRAJ) {
				uuidToVtkWellboreTrajectoryRepresentation[vtkEpcCommon.getUuid()]->remove(vtkEpcCommon.getUuid());
				detach();
				attachUuids.erase(std::find(attachUuids.begin(), attachUuids.end(), vtkEpcCommon.getUuid()));
				attach();
			}
		}
	}
}

// ----------------------------------------------------------------------------
void VtkEpcDocument::remove(const std::string & uuid)
{
	auto uuidtoAttach = uuid;
	if (uuidIsChildOf[uuid].getType() == VtkEpcCommon::PROPERTY) {
		uuidtoAttach = uuidIsChildOf[uuid].getParent();
	}

	if (std::find(attachUuids.begin(), attachUuids.end(), uuidtoAttach) != attachUuids.end()) {
		switch (uuidIsChildOf[uuidtoAttach].getType())	{
		case VtkEpcCommon::GRID_2D:	{
			uuidToVtkGrid2DRepresentation[uuidtoAttach]->remove(uuid);
			if (uuid == uuidtoAttach){
				detach();
				attachUuids.erase(std::find(attachUuids.begin(), attachUuids.end(), uuid));
				attach();
			}
			break;
		}
		case VtkEpcCommon::POLYLINE_SET: {
			auto object = repository.getDataObjectByUuid(uuidtoAttach);
			auto typeRepresentation = object->getXmlTag();
			if (typeRepresentation == "PolylineRepresentation") {
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
		case VtkEpcCommon::TRIANGULATED_SET: {
			auto object = repository.getDataObjectByUuid(uuidtoAttach);
			auto typeRepresentation = object->getXmlTag();
			if (typeRepresentation == "TriangulatedRepresentation")  {
				uuidToVtkTriangulatedRepresentation[uuidtoAttach]->remove(uuid);
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
		case VtkEpcCommon::WELL_TRAJ: {
			uuidToVtkWellboreTrajectoryRepresentation[uuidtoAttach]->remove(uuid);
			if (uuid == uuidtoAttach){
				detach();
				attachUuids.erase(std::find(attachUuids.begin(), attachUuids.end(), uuid));
				attach();
			}
			break;
		}
		case VtkEpcCommon::IJK_GRID: {
			uuidToVtkIjkGridRepresentation[uuidtoAttach]->remove(uuid);
			if (uuid == uuidtoAttach) {
				detach();
				attachUuids.erase(std::find(attachUuids.begin(), attachUuids.end(), uuid));
				attach();
			}
			break;
		}
		case VtkEpcCommon::UNSTRUC_GRID: {
			uuidToVtkUnstructuredGridRepresentation[uuidtoAttach]->remove(uuid);
			if (uuid == uuidtoAttach) {
				detach();
				attachUuids.erase(std::find(attachUuids.begin(), attachUuids.end(), uuid));
				attach();
			}
			break;
		}
		case VtkEpcCommon::SUB_REP:	{
			if (uuidIsChildOf[uuidIsChildOf[uuid].getParent()].getParentType() == VtkEpcCommon::Resqml2Type::IJK_GRID) {
				uuidToVtkIjkGridRepresentation[uuidIsChildOf[uuidIsChildOf[uuid].getParent()].getUuid()]->remove(uuid);
			}
			else if (uuidIsChildOf[uuidIsChildOf[uuid].getParent()].getParentType() == VtkEpcCommon::Resqml2Type::UNSTRUC_GRID) {
				uuidToVtkUnstructuredGridRepresentation[uuidIsChildOf[uuidIsChildOf[uuid].getParent()].getUuid()]->remove(uuid);
			}
			else if (uuidIsChildOf[uuidIsChildOf[uuid].getParent()].getParentType() == VtkEpcCommon::PARTIAL)	{
				if (uuidIsChildOf[uuidIsChildOf[uuidIsChildOf[uuid].getParent()].getParent()].getParentType() == VtkEpcCommon::IJK_GRID)	{
					uuidToVtkIjkGridRepresentation[uuidIsChildOf[uuidIsChildOf[uuid].getParent()].getUuid()]->remove(uuid);
				}
				if (uuidIsChildOf[uuidIsChildOf[uuidIsChildOf[uuid].getParent()].getParent()].getParentType() == VtkEpcCommon::UNSTRUC_GRID)	{
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
		case VtkEpcCommon::PARTIAL:	{
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

	for (unsigned int newBlockIndex = 0; newBlockIndex < attachUuids.size(); ++newBlockIndex) {
		std::string uuid = attachUuids[newBlockIndex];
		if (uuidIsChildOf[uuid].getType() == VtkEpcCommon::GRID_2D)	{
			vtkOutput->SetBlock(newBlockIndex, uuidToVtkGrid2DRepresentation[uuid]->getOutput());
			vtkOutput->GetMetaData(newBlockIndex)->Set(vtkCompositeDataSet::NAME(), uuidToVtkGrid2DRepresentation[uuid]->getUuid().c_str());
		}
		if (uuidIsChildOf[uuid].getType() == VtkEpcCommon::POLYLINE_SET) {
			auto object = repository.getDataObjectByUuid(uuid);
			auto typeRepresentation = object->getXmlTag();
			if (typeRepresentation == "PolylineRepresentation") {
				vtkOutput->SetBlock(newBlockIndex, uuidToVtkPolylineRepresentation[uuid]->getOutput());
				vtkOutput->GetMetaData(newBlockIndex)->Set(vtkCompositeDataSet::NAME(), uuidToVtkPolylineRepresentation[uuid]->getUuid().c_str());
			}
			else  {
				vtkOutput->SetBlock(newBlockIndex, uuidToVtkSetPatch[uuid]->getOutput());
				vtkOutput->GetMetaData(newBlockIndex)->Set(vtkCompositeDataSet::NAME(), uuidToVtkSetPatch[uuid]->getUuid().c_str());
			}
		}
		if (uuidIsChildOf[uuid].getType() == VtkEpcCommon::TRIANGULATED_SET) {
			auto object = repository.getDataObjectByUuid(uuid);
			auto typeRepresentation = object->getXmlTag();
			if (typeRepresentation == "TriangulatedRepresentation")  {
				vtkOutput->SetBlock(newBlockIndex, uuidToVtkTriangulatedRepresentation[uuid]->getOutput());
				vtkOutput->GetMetaData(newBlockIndex)->Set(vtkCompositeDataSet::NAME(), uuidToVtkTriangulatedRepresentation[uuid]->getUuid().c_str());
			}
			else  {
				vtkOutput->SetBlock(newBlockIndex, uuidToVtkSetPatch[uuid]->getOutput());
				vtkOutput->GetMetaData(newBlockIndex)->Set(vtkCompositeDataSet::NAME(), uuidToVtkSetPatch[uuid]->getUuid().c_str());
			}
		}
		if (uuidIsChildOf[uuid].getType() == VtkEpcCommon::WELL_TRAJ) {
			vtkOutput->SetBlock(newBlockIndex, uuidToVtkWellboreTrajectoryRepresentation[uuid]->getOutput());
			vtkOutput->GetMetaData(newBlockIndex)->Set(vtkCompositeDataSet::NAME(), uuidToVtkWellboreTrajectoryRepresentation[uuid]->getUuid().c_str());
		}
		if (uuidIsChildOf[uuid].getType() == VtkEpcCommon::IJK_GRID) {
			vtkOutput->SetBlock(newBlockIndex, uuidToVtkIjkGridRepresentation[uuid]->getOutput());
			vtkOutput->GetMetaData(newBlockIndex)->Set(vtkCompositeDataSet::NAME(), uuidToVtkIjkGridRepresentation[uuid]->getUuid().c_str());
		}
		if (uuidIsChildOf[uuid].getType() == VtkEpcCommon::UNSTRUC_GRID) {
			vtkOutput->SetBlock(newBlockIndex, uuidToVtkUnstructuredGridRepresentation[uuid]->getOutput());
			vtkOutput->GetMetaData(newBlockIndex)->Set(vtkCompositeDataSet::NAME(), uuidToVtkUnstructuredGridRepresentation[uuid]->getUuid().c_str());
		}
		if (uuidIsChildOf[uuid].getType() == VtkEpcCommon::SUB_REP) {
			if (uuidIsChildOf[uuid].getParentType() == VtkEpcCommon::IJK_GRID) {
				vtkOutput->SetBlock(newBlockIndex, uuidToVtkIjkGridRepresentation[uuidIsChildOf[uuid].getUuid()]->getOutput());
				vtkOutput->GetMetaData(newBlockIndex)->Set(vtkCompositeDataSet::NAME(), uuidToVtkIjkGridRepresentation[uuid]->getUuid().c_str());
			}
			if (uuidIsChildOf[uuid].getParentType() == VtkEpcCommon::UNSTRUC_GRID) {
				vtkOutput->SetBlock(newBlockIndex, uuidToVtkUnstructuredGridRepresentation[uuidIsChildOf[uuid].getUuid()]->getOutput());
				vtkOutput->GetMetaData(newBlockIndex)->Set(vtkCompositeDataSet::NAME(), uuidToVtkUnstructuredGridRepresentation[uuidIsChildOf[uuid].getUuid()]->getUuid().c_str());
			}
			if (uuidIsChildOf[uuid].getParentType() == VtkEpcCommon::PARTIAL) {
				auto parentUuidType = uuidToVtkPartialRepresentation[uuidIsChildOf[uuid].getParent()]->getType();

				switch (parentUuidType)	{
				case VtkEpcCommon::GRID_2D:	{
					vtkOutput->SetBlock(newBlockIndex, uuidToVtkGrid2DRepresentation[uuid]->getOutput());
					vtkOutput->GetMetaData(newBlockIndex)->Set(vtkCompositeDataSet::NAME(), uuidToVtkGrid2DRepresentation[uuid]->getUuid().c_str());
					break;
				}
				case VtkEpcCommon::WELL_TRAJ: {
					vtkOutput->SetBlock(newBlockIndex, uuidToVtkWellboreTrajectoryRepresentation[uuid]->getOutput());
					vtkOutput->GetMetaData(newBlockIndex)->Set(vtkCompositeDataSet::NAME(), uuidToVtkWellboreTrajectoryRepresentation[uuid]->getUuid().c_str());
					break;
				}
				case VtkEpcCommon::IJK_GRID: {
					vtkOutput->SetBlock(newBlockIndex, uuidToVtkIjkGridRepresentation[uuidIsChildOf[uuid].getUuid()]->getOutput());
					vtkOutput->GetMetaData(newBlockIndex)->Set(vtkCompositeDataSet::NAME(), uuidToVtkIjkGridRepresentation[uuid]->getUuid().c_str());
					break;
				}
				case VtkEpcCommon::UNSTRUC_GRID: {
					vtkOutput->SetBlock(newBlockIndex, uuidToVtkUnstructuredGridRepresentation[uuidIsChildOf[uuid].getUuid()]->getOutput());
					vtkOutput->GetMetaData(newBlockIndex)->Set(vtkCompositeDataSet::NAME(), uuidToVtkUnstructuredGridRepresentation[uuidIsChildOf[uuid].getUuid()]->getUuid().c_str());
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
	case VtkEpcCommon::GRID_2D:	{
		uuidToVtkGrid2DRepresentation[uuidIsChildOf[uuidProperty].getUuid()]->addProperty(uuidProperty, dataProperty);
		break;
	}
	case VtkEpcCommon::POLYLINE_SET: {
		auto object = repository.getDataObjectByUuid(uuidIsChildOf[uuidProperty].getUuid());
		auto typeRepresentation = object->getXmlTag();
		if (typeRepresentation == "PolylineRepresentation")	{
			uuidToVtkPolylineRepresentation[uuidIsChildOf[uuidProperty].getUuid()]->addProperty(uuidProperty, dataProperty);
		}
		else {
			uuidToVtkSetPatch[uuidIsChildOf[uuidProperty].getUuid()]->addProperty(uuidProperty, dataProperty);
		}
		break;
	}
	case VtkEpcCommon::TRIANGULATED_SET: {
		auto object = repository.getDataObjectByUuid(uuidIsChildOf[uuidProperty].getUuid());
		auto typeRepresentation = object->getXmlTag();
		if (typeRepresentation == "TriangulatedRepresentation") {
			uuidToVtkTriangulatedRepresentation[uuidIsChildOf[uuidProperty].getUuid()]->addProperty(uuidProperty, dataProperty);
		}
		else {
			uuidToVtkSetPatch[uuidIsChildOf[uuidProperty].getUuid()]->addProperty(uuidProperty, dataProperty);
		}
		break;
	}
	case VtkEpcCommon::WELL_TRAJ: {
		uuidToVtkWellboreTrajectoryRepresentation[uuidIsChildOf[uuidProperty].getUuid()]->addProperty(uuidProperty, dataProperty);
		break;
	}
	case VtkEpcCommon::IJK_GRID: {
		uuidToVtkIjkGridRepresentation[uuidIsChildOf[uuidProperty].getUuid()]->addProperty(uuidProperty, dataProperty);
		break;
	}
	case VtkEpcCommon::UNSTRUC_GRID: {
		uuidToVtkUnstructuredGridRepresentation[uuidIsChildOf[uuidProperty].getUuid()]->addProperty(uuidProperty, dataProperty);
		break;
	}
	case VtkEpcCommon::SUB_REP: {
		if (uuidIsChildOf[uuidProperty].getParentType() == VtkEpcCommon::IJK_GRID) {
			uuidToVtkIjkGridRepresentation[uuidIsChildOf[uuidProperty].getUuid()]->addProperty(uuidProperty, dataProperty);
		}
		if (uuidIsChildOf[uuidProperty].getParentType() == VtkEpcCommon::UNSTRUC_GRID) {
			uuidToVtkUnstructuredGridRepresentation[uuidIsChildOf[uuidProperty].getUuid()]->addProperty(uuidProperty, dataProperty);
		}
		if (uuidIsChildOf[uuidProperty].getParentType() == VtkEpcCommon::PARTIAL) {
			auto parentUuidType = uuidToVtkPartialRepresentation[uuidIsChildOf[uuidProperty].getParent()]->getType();

			switch (parentUuidType) {
			case VtkEpcCommon::GRID_2D: {
				uuidToVtkGrid2DRepresentation[uuidIsChildOf[uuidProperty].getUuid()]->addProperty(uuidProperty, dataProperty);
				break;
			}
			case VtkEpcCommon::WELL_TRAJ: {
				uuidToVtkWellboreTrajectoryRepresentation[uuidIsChildOf[uuidProperty].getUuid()]->addProperty(uuidProperty, dataProperty);
				break;
			}
			case VtkEpcCommon::IJK_GRID: {
				uuidToVtkIjkGridRepresentation[uuidIsChildOf[uuidProperty].getUuid()]->addProperty(uuidProperty, dataProperty);
				break;
			}
			case VtkEpcCommon::UNSTRUC_GRID: {
				uuidToVtkUnstructuredGridRepresentation[uuidIsChildOf[uuidProperty].getUuid()]->addProperty(uuidProperty, dataProperty);
				break;
			}
			default:
				break;
			}
		}
	}
	default:
		break;
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
		if (uuidIsChildOf[uuid].getParentType() == VtkEpcCommon::IJK_GRID) {
			result = uuidToVtkIjkGridRepresentation[uuidIsChildOf[uuid].getUuid()]->getAttachmentPropertyCount(uuid, propertyUnit);
		}
		if (uuidIsChildOf[uuid].getParentType() == VtkEpcCommon::UNSTRUC_GRID) {
			result = uuidToVtkUnstructuredGridRepresentation[uuidIsChildOf[uuid].getUuid()]->getAttachmentPropertyCount(uuid, propertyUnit);
		}
		if (uuidIsChildOf[uuid].getParentType() == VtkEpcCommon::PARTIAL) {
			auto uuidPartial = uuidIsChildOf[uuid].getParent();

			if (uuidIsChildOf[uuidPartial].getParentType() == VtkEpcCommon::IJK_GRID) {
				result = uuidToVtkIjkGridRepresentation[uuid]->getAttachmentPropertyCount(uuid, propertyUnit);
			}
			if (uuidIsChildOf[uuid].getParentType() == VtkEpcCommon::UNSTRUC_GRID) {
				result = uuidToVtkUnstructuredGridRepresentation[uuid]->getAttachmentPropertyCount(uuid, propertyUnit);
			}
		}
		break;
	}
	default:
		break;
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
		if (uuidIsChildOf[uuid].getParentType() == VtkEpcCommon::IJK_GRID) {
			result = uuidToVtkIjkGridRepresentation[uuidIsChildOf[uuid].getUuid()]->getICellCount(uuid);
		}
		if (uuidIsChildOf[uuid].getParentType() == VtkEpcCommon::PARTIAL) {
			auto uuidPartial = uuidIsChildOf[uuid].getParent();

			if (uuidIsChildOf[uuidPartial].getParentType() == VtkEpcCommon::IJK_GRID) {
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
		if (uuidIsChildOf[uuid].getParentType() == VtkEpcCommon::IJK_GRID) {
			result = uuidToVtkIjkGridRepresentation[uuidIsChildOf[uuid].getUuid()]->getJCellCount(uuid);
		}
		if (uuidIsChildOf[uuid].getParentType() == VtkEpcCommon::PARTIAL) {
			auto uuidPartial = uuidIsChildOf[uuid].getParent();

			if (uuidIsChildOf[uuidPartial].getParentType() == VtkEpcCommon::IJK_GRID) {
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
		if (uuidIsChildOf[uuid].getParentType() == VtkEpcCommon::IJK_GRID) {
			result = uuidToVtkIjkGridRepresentation[uuidIsChildOf[uuid].getUuid()]->getKCellCount(uuid);
		}
		if (uuidIsChildOf[uuid].getParentType() == VtkEpcCommon::PARTIAL) {
			auto uuidPartial = uuidIsChildOf[uuid].getParent();

			if (uuidIsChildOf[uuidPartial].getParentType() == VtkEpcCommon::IJK_GRID) {
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
		if (uuidIsChildOf[uuid].getParentType() == VtkEpcCommon::IJK_GRID) {
			result = uuidToVtkIjkGridRepresentation[uuidIsChildOf[uuid].getUuid()]->getInitKIndex(uuid);
		}
		if (uuidIsChildOf[uuid].getParentType() == VtkEpcCommon::PARTIAL) {
			auto uuidPartial = uuidIsChildOf[uuid].getParent();

			if (uuidIsChildOf[uuidPartial].getParentType() == VtkEpcCommon::IJK_GRID) {
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
COMMON_NS::DataObjectRepository* VtkEpcDocument::getEpcDocument()
{
	return &repository;
}

// ----------------------------------------------------------------------------
std::vector<std::string> VtkEpcDocument::getListUuid()
{
	return uuidRep;
}


// ----------------------------------------------------------------------------
std::vector<VtkEpcCommon> VtkEpcDocument::getTreeView() const
{
	return treeView;
}

// ----------------------------------------------------------------------------
std::string VtkEpcDocument::getError()
{

	return epc_error;
}


// PRIVATE

void VtkEpcDocument::searchFaultPolylines(const std::string & fileName) {
	std::vector<resqml2_0_1::PolylineSetRepresentation*> polylines;
	try	{
		polylines = repository.getFaultPolylineSetRepSet();
	}
	catch  (const std::exception & e) {
		cout << "EXCEPTION in fesapi when call getFaultPolylineSetRepSet with file: " << fileName << " : " << e.what();
	}
	for (size_t idx = 0; idx < polylines.size(); ++idx)	{
		if (polylines[idx]->isPartial()) {
			auto vtkEpcSrc = epcSet->getVtkEpcDocument(polylines[idx]->getUuid());
			if (vtkEpcSrc) {
				auto type_in_epcdoc = epcSet->getTypeInEpcDocument(polylines[idx]->getUuid());
				if (type_in_epcdoc == VtkEpcCommon::POLYLINE_SET){
					createTreeVtkPartialRep(polylines[idx]->getUuid(), vtkEpcSrc);
					uuidIsChildOf[polylines[idx]->getUuid()].setParentType( VtkEpcCommon::POLYLINE_SET);
				} else {
					epc_error = epc_error + " Partial UUID (" + polylines[idx]->getUuid() + ") is PolylineSet and is incorrect \n";
				}
			} else {
				epc_error = epc_error + " Partial UUID: (" + polylines[idx]->getUuid() + ") is not loaded \n";
			}
		} else {
			std::string uuidParent= fileName;
			auto interpretation = polylines[idx]->getInterpretation();
			if (interpretation) {
				uuidParent = interpretation->getUuid();
				createTreeVtk(uuidParent, fileName, interpretation->getTitle().c_str(), VtkEpcCommon::INTERPRETATION_2D);
			}
			createTreeVtk(polylines[idx]->getUuid(), uuidParent, polylines[idx]->getTitle().c_str(), VtkEpcCommon::POLYLINE_SET);
		}

		//property
		auto valuesPropertySet = polylines[idx]->getValuesPropertySet();
		for (size_t i = 0; i < valuesPropertySet.size(); ++i)	{
			//				uuidIsChildOf[valuesPropertySet[i]->getUuid()] = new VtkEpcCommon();
			createTreeVtk(valuesPropertySet[i]->getUuid(), polylines[idx]->getUuid(), valuesPropertySet[i]->getTitle().c_str(), VtkEpcCommon::PROPERTY);
		}
	}
}

void VtkEpcDocument::searchHorizonPolylines(const std::string & fileName) {
	std::vector<resqml2_0_1::PolylineSetRepresentation*> polylines;
	try	{
		polylines = repository.getHorizonPolylineSetRepSet();
	}
	catch  (const std::exception & e) {
		cout << "EXCEPTION in fesapi when call getHorizonPolylineSetRepSet with file: " << fileName << " : " << e.what();
	}
	for (size_t idx = 0; idx < polylines.size(); ++idx)	{
		if (polylines[idx]->isPartial()) {
			auto vtkEpcSrc = epcSet->getVtkEpcDocument(polylines[idx]->getUuid());
			if (vtkEpcSrc)	{
				auto type_in_epcdoc = epcSet->getTypeInEpcDocument(polylines[idx]->getUuid());
				if (type_in_epcdoc == VtkEpcCommon::POLYLINE_SET){
					createTreeVtkPartialRep(polylines[idx]->getUuid(), vtkEpcSrc);
					uuidIsChildOf[polylines[idx]->getUuid()].setParentType( VtkEpcCommon::POLYLINE_SET);
				} else {
					epc_error = epc_error + " Partial UUID (" + polylines[idx]->getUuid() + ") is PolylineSet and is incorrect \n";
				}
			} else {
				epc_error = epc_error + " Partial UUID: (" + polylines[idx]->getUuid() + ") is not loaded \n";
			}
		} else {
			std::string uuidParent = fileName;
			auto interpretation = polylines[idx]->getInterpretation();
			if (interpretation) {
				uuidParent = interpretation->getUuid();
				createTreeVtk(uuidParent, fileName, interpretation->getTitle().c_str(), VtkEpcCommon::INTERPRETATION_2D);
			}
			createTreeVtk(polylines[idx]->getUuid(), uuidParent, polylines[idx]->getTitle().c_str(), VtkEpcCommon::POLYLINE_SET);
		}

		//property
		auto valuesPropertySet = polylines[idx]->getValuesPropertySet();
		for (size_t i = 0; i < valuesPropertySet.size(); ++i)	{
			//				uuidIsChildOf[valuesPropertySet[i]->getUuid()] = new VtkEpcCommon();
			createTreeVtk(valuesPropertySet[i]->getUuid(), polylines[idx]->getUuid(), valuesPropertySet[i]->getTitle().c_str(), VtkEpcCommon::PROPERTY);
		}
	}
}

void VtkEpcDocument::searchUnstructuredGrid(const std::string & fileName) {
	std::vector<resqml2_0_1::UnstructuredGridRepresentation*> unstructuredGrid;
	try	{
		unstructuredGrid = repository.getUnstructuredGridRepresentationSet();
	}
	catch  (const std::exception & e)	{
		cout << "EXCEPTION in fesapi when call getUnstructuredGridRepresentationSet with file: " << fileName << " : " << e.what();
	}
	for (size_t iter = 0; iter < unstructuredGrid.size(); ++iter)	{
		if (unstructuredGrid[iter]->isPartial())	{
			auto vtkEpcSrc = epcSet->getVtkEpcDocument(unstructuredGrid[iter]->getUuid());
			if (vtkEpcSrc){
				auto type_in_epcdoc = epcSet->getTypeInEpcDocument(unstructuredGrid[iter]->getUuid());
				if (type_in_epcdoc == VtkEpcCommon::UNSTRUC_GRID){
					createTreeVtkPartialRep(unstructuredGrid[iter]->getUuid(), vtkEpcSrc);
					uuidIsChildOf[unstructuredGrid[iter]->getUuid()].setParentType( VtkEpcCommon::UNSTRUC_GRID);
				} else {
					epc_error = epc_error + " Partial UUID (" + unstructuredGrid[iter]->getUuid() + ") is UnstructuredGrid type and is incorrect \n";
				}
			} else {
				epc_error = epc_error + " Partial UUID: (" + unstructuredGrid[iter]->getUuid() + ") is not loaded \n";
			}
		} else	{
			std::string uuidParent = fileName;
			auto interpretation = unstructuredGrid[iter]->getInterpretation();
			if (interpretation) {
				uuidParent = interpretation->getUuid();
				createTreeVtk(uuidParent, fileName, interpretation->getTitle().c_str(), VtkEpcCommon::INTERPRETATION_3D);
			}
			createTreeVtk(unstructuredGrid[iter]->getUuid(), fileName, unstructuredGrid[iter]->getTitle().c_str(), VtkEpcCommon::UNSTRUC_GRID);
		}

		//property
		auto valuesPropertySet = unstructuredGrid[iter]->getValuesPropertySet();
		for (size_t i = 0; i < valuesPropertySet.size(); ++i)	{
			createTreeVtk(valuesPropertySet[i]->getUuid(), unstructuredGrid[iter]->getUuid(), valuesPropertySet[i]->getTitle().c_str(), VtkEpcCommon::PROPERTY);
		}
	}
}

void VtkEpcDocument::searchTriangulated(const std::string & fileName) {
	std::vector<resqml2_0_1::TriangulatedSetRepresentation*> triangulated;
	try	{
		triangulated = repository.getAllTriangulatedSetRepSet();
	} catch  (const std::exception & e)	{
		cout << "EXCEPTION in fesapi when call getAllTriangulatedSetRepSet with file: " << fileName << " : " << e.what();
	}
	for (size_t iter = 0; iter < triangulated.size(); ++iter)	{
		if (triangulated[iter]->isPartial()) {
			auto vtkEpcSrc = epcSet->getVtkEpcDocument(triangulated[iter]->getUuid());
			if (vtkEpcSrc)	{
				auto type_in_epcdoc = epcSet->getTypeInEpcDocument(triangulated[iter]->getUuid());
				if (type_in_epcdoc == VtkEpcCommon::TRIANGULATED_SET){
					createTreeVtkPartialRep(triangulated[iter]->getUuid(), vtkEpcSrc);
					uuidIsChildOf[triangulated[iter]->getUuid()].setParentType( VtkEpcCommon::TRIANGULATED_SET);
				} else {
					epc_error = epc_error + " Partial UUID (" + triangulated[iter]->getUuid() + ") is Triangulated type and is incorrect \n";
				}
			} else {
				epc_error = epc_error + " Partial UUID: (" + triangulated[iter]->getUuid() + ") is not loaded \n";
			}
		} else {
			std::string uuidParent= fileName;
			auto interpretation = triangulated[iter]->getInterpretation();
			if (interpretation) {
				uuidParent = interpretation->getUuid();
				createTreeVtk(uuidParent, fileName, interpretation->getTitle().c_str(), VtkEpcCommon::INTERPRETATION_2D);
			}
			createTreeVtk(triangulated[iter]->getUuid(), uuidParent, triangulated[iter]->getTitle().c_str(), VtkEpcCommon::TRIANGULATED_SET);
		}
		//property
		auto valuesPropertySet = triangulated[iter]->getValuesPropertySet();
		for (size_t i = 0; i < valuesPropertySet.size(); ++i) {
			createTreeVtk(valuesPropertySet[i]->getUuid(), triangulated[iter]->getUuid(), valuesPropertySet[i]->getTitle().c_str(), VtkEpcCommon::PROPERTY);
		}
	}
}

void VtkEpcDocument::searchGrid2d(const std::string & fileName) {
	std::vector<resqml2_0_1::Grid2dRepresentation*> grid2D;
	try	{
		grid2D = repository.getHorizonGrid2dRepSet();
	} catch  (const std::exception & e)	{
		cout << "EXCEPTION in fesapi when call getHorizonGrid2dRepSet with file: " << fileName << " : " << e.what();
	}
	for (size_t iter = 0; iter < grid2D.size(); ++iter)	{
		if (grid2D[iter]->isPartial())	{
			auto vtkEpcSrc = epcSet->getVtkEpcDocument(grid2D[iter]->getUuid());
			if (vtkEpcSrc) {
				auto type_in_epcdoc = epcSet->getTypeInEpcDocument(grid2D[iter]->getUuid());
				if (type_in_epcdoc == VtkEpcCommon::GRID_2D){
					createTreeVtkPartialRep(grid2D[iter]->getUuid(), vtkEpcSrc);
					uuidIsChildOf[grid2D[iter]->getUuid()].setParentType( VtkEpcCommon::GRID_2D);
				} else {
					epc_error = epc_error + " Partial UUID (" + grid2D[iter]->getUuid() + ") is Grid2d type and is incorrect \n";
				}
			} else {
				epc_error = epc_error + " Partial UUID: (" + grid2D[iter]->getUuid() + ") is not loaded \n";
			}
		} else	{
			auto interpretation = grid2D[iter]->getInterpretation();
			std::string uuidParent = fileName;
			if (interpretation) {
				uuidParent = interpretation->getUuid();
				createTreeVtk(uuidParent, fileName, interpretation->getTitle().c_str(), VtkEpcCommon::INTERPRETATION_2D);
			}
			createTreeVtk(grid2D[iter]->getUuid(), uuidParent, grid2D[iter]->getTitle().c_str(), VtkEpcCommon::GRID_2D);
		}
		//property
		auto valuesPropertySet = grid2D[iter]->getValuesPropertySet();
		for (size_t i = 0; i < valuesPropertySet.size(); ++i)	{
			createTreeVtk(valuesPropertySet[i]->getUuid(), grid2D[iter]->getUuid(), valuesPropertySet[i]->getTitle().c_str(), VtkEpcCommon::PROPERTY);
		}
	}
}

void VtkEpcDocument::searchIjkGrid(const std::string & fileName) {
	std::vector<resqml2_0_1::AbstractIjkGridRepresentation*> ijkGrid;
	try	{
		ijkGrid = repository.getIjkGridRepresentationSet();
	}
	catch  (const std::exception & e) {
		cout << "EXCEPTION in fesapi when call getIjkGridRepresentationSet with file: " << fileName << " : " << e.what();
	}
	for (size_t iter = 0; iter < ijkGrid.size(); ++iter) {
		if (ijkGrid[iter]->isPartial())	{
			auto vtkEpcSrc = epcSet->getVtkEpcDocument(ijkGrid[iter]->getUuid());
			if (vtkEpcSrc)	{
				auto type_in_epcdoc = epcSet->getTypeInEpcDocument(ijkGrid[iter]->getUuid());
				if (type_in_epcdoc == VtkEpcCommon::IJK_GRID){
					createTreeVtkPartialRep(ijkGrid[iter]->getUuid(), vtkEpcSrc);
					uuidIsChildOf[ijkGrid[iter]->getUuid()].setParentType( VtkEpcCommon::IJK_GRID);
				} else {
					epc_error = epc_error + " Partial UUID (" + ijkGrid[iter]->getUuid() + ") is IjkGrid type and is incorrect \n";
				}
			} else {
				epc_error = epc_error + " Partial UUID: (" + ijkGrid[iter]->getUuid() + ") is not loaded \n";
			}
		} else {
			auto interpretation = ijkGrid[iter]->getInterpretation();
			std::string uuidParent = fileName;
			if (interpretation) {
				uuidParent = interpretation->getUuid();
				createTreeVtk(uuidParent, fileName, interpretation->getTitle().c_str(), VtkEpcCommon::INTERPRETATION_3D);
			}
			createTreeVtk(ijkGrid[iter]->getUuid(), uuidParent, ijkGrid[iter]->getTitle().c_str(), VtkEpcCommon::IJK_GRID);
		}

		//property
		auto valuesPropertySet = ijkGrid[iter]->getValuesPropertySet();
		for (size_t i = 0; i < valuesPropertySet.size(); ++i)	{
			createTreeVtk(valuesPropertySet[i]->getUuid(), ijkGrid[iter]->getUuid(), valuesPropertySet[i]->getTitle().c_str(), VtkEpcCommon::PROPERTY);
		}
	}
}

void VtkEpcDocument::searchWellboreTrajectory(const std::string & fileName) {
	std::vector<resqml2_0_1::WellboreTrajectoryRepresentation*> wellboreTrajectory_set;
	try	{
		wellboreTrajectory_set = repository.getWellboreTrajectoryRepresentationSet();
	}
	catch  (const std::exception & e)	{
		cout << "EXCEPTION in fesapi when call getWellboreTrajectoryRepresentationSet with file: " << fileName << " : " << e.what();
	}
	for (const auto& wellboreTrajectory: wellboreTrajectory_set)	{
		if (wellboreTrajectory->isPartial()) {
			auto vtkEpcSrc = epcSet->getVtkEpcDocument(wellboreTrajectory->getUuid());
			if (vtkEpcSrc)	{
				auto type_in_epcdoc = epcSet->getTypeInEpcDocument(wellboreTrajectory->getUuid());
				if (type_in_epcdoc == VtkEpcCommon::WELL_TRAJ){
					createTreeVtkPartialRep(wellboreTrajectory->getUuid(), vtkEpcSrc);
					uuidIsChildOf[wellboreTrajectory->getUuid()].setParentType( VtkEpcCommon::WELL_TRAJ);
				} else {
					epc_error = epc_error + " Partial UUID (" + wellboreTrajectory->getUuid() + ") is WellboreTrajectory type and is incorrect \n";
				}
			} else {
				epc_error = epc_error + " Partial UUID: (" + wellboreTrajectory->getUuid() + ") is not loaded \n";
			}
		} else {
			auto interpretation = wellboreTrajectory->getInterpretation();
			std::string uuidParent = fileName;
			if (interpretation) {
				uuidParent = interpretation->getUuid();
				createTreeVtk(uuidParent, fileName, interpretation->getTitle().c_str(), VtkEpcCommon::INTERPRETATION_1D);
			}
			createTreeVtk(wellboreTrajectory->getUuid(), uuidParent, wellboreTrajectory->getTitle().c_str(), VtkEpcCommon::WELL_TRAJ);
		}

		//wellboreFrame
		auto wellboreFrame_set = wellboreTrajectory->getWellboreFrameRepresentationSet();
		for(const auto& wellboreFrame: wellboreFrame_set) {
			if (wellboreFrame->getXmlTag() == "WellboreFrameRepresentation") { // WellboreFrame
				createTreeVtk(wellboreFrame->getUuid(), wellboreTrajectory->getUuid(), wellboreFrame->getTitle().c_str(), VtkEpcCommon::WELL_FRAME);
				auto valuesPropertySet = wellboreFrame->getValuesPropertySet();
				for (const auto& property: valuesPropertySet)		{
					createTreeVtk(property->getUuid(), wellboreFrame->getUuid(), property->getTitle().c_str(), VtkEpcCommon::PROPERTY);
				}
			} else if (wellboreFrame->getXmlTag() == "WellboreMarkerFrameRepresentation") { // WellboreMarker
				createTreeVtk(wellboreFrame->getUuid(), wellboreTrajectory->getUuid(), wellboreFrame->getTitle().c_str(), VtkEpcCommon::WELL_MARKER_FRAME);
				auto wellboreMarkerFrame = static_cast<resqml2_0_1::WellboreMarkerFrameRepresentation*>(wellboreFrame);
				auto wellboreMarker_set = wellboreMarkerFrame->getWellboreMarkerSet();
				for(const auto& wellboreMarker: wellboreMarker_set) {
					createTreeVtk(wellboreMarker->getUuid(), wellboreMarkerFrame->getUuid(), wellboreMarker->getTitle().c_str(), VtkEpcCommon::WELL_MARKER);
				}
			}
		}
	}
}

void VtkEpcDocument::searchSubRepresentation(const std::string & fileName) {
	std::vector<resqml2::SubRepresentation*> subRepresentationSet;
	try		{
		subRepresentationSet = repository.getSubRepresentationSet();
	}
	catch  (const std::exception & e)		{
		cout << "EXCEPTION in fesapi when call getSubRepresentationSet with file: " << fileName << " : " << e.what();
	}
	for (auto& subRepresentation : subRepresentationSet) {
		if (subRepresentation->isPartial()){
			auto vtkEpcSrc = epcSet->getVtkEpcDocument(subRepresentation->getUuid());
			if (vtkEpcSrc)	{
				createTreeVtkPartialRep(subRepresentation->getUuid(), vtkEpcSrc);
				uuidIsChildOf[subRepresentation->getUuid()].setParentType( VtkEpcCommon::SUB_REP);
			}
			else {
				epc_error = epc_error + " Partial UUID: (" + subRepresentation->getUuid() + ") is not loaded \n";
			}
		}
		else {
			auto uuidParent = subRepresentation->getSupportingRepresentationUuid(0);
			createTreeVtk(subRepresentation->getUuid(), uuidParent, subRepresentation->getTitle().c_str(), VtkEpcCommon::SUB_REP);
		}

		//property
		auto valuesPropertySet = subRepresentation->getValuesPropertySet();
		for (auto& valuesProperty : valuesPropertySet) {
			createTreeVtk(valuesProperty->getUuid(), subRepresentation->getUuid(), valuesProperty->getTitle().c_str(), VtkEpcCommon::PROPERTY);
		}
	}
}

void VtkEpcDocument::searchTimeSeries(const std::string & fileName) {
	std::vector<resqml2::TimeSeries*> timeSeries;
	try	{
		timeSeries = repository.getTimeSeriesSet();
	}
	catch  (const std::exception & e)	{
		cout << "EXCEPTION in fesapi when call getTimeSeriesSet with file: " << fileName << " : " << e.what();
	}

	for (auto& timeSerie : timeSeries) {
		auto propSeries = timeSerie->getPropertySet();
		for (auto& propertie : propSeries) {
			if (propertie->getXmlTag() != "ContinuousPropertySeries") {
				auto prop_uuid = propertie->getUuid();
				if (uuidIsChildOf.find(prop_uuid) == uuidIsChildOf.end()) {
					std::cout << "The property " << prop_uuid << " is not supported and consequently cannot be associated to its time series." << std::endl;
					continue;
				}
				uuidIsChildOf[prop_uuid].setType(VtkEpcCommon::TIME_SERIES);
				uuidIsChildOf[prop_uuid].setTimeIndex(propertie->getTimeIndex());
				uuidIsChildOf[prop_uuid].setTimestamp(propertie->getTimestamp());
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
