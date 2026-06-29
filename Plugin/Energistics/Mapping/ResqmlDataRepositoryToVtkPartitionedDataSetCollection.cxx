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

/*
Documentation:
==============

VtkAssembly => TreeView:
	- each node required 3 attributes:
		- id = uuid of resqml/witsml object
		- label = name to display in TreeView
		- type = type
*/
#include "ResqmlDataRepositoryToVtkPartitionedDataSetCollection.h"

#include <algorithm>
#include <chrono>
#include <vector>
#include <set>
#include <list>
#include <regex>
#include <numeric>
#include <limits>
#include <cmath>
#include <cstdlib>
#include <sstream>
#include <iostream>

// VTK includes
#include <vtkPartitionedDataSetCollection.h>
#include <vtkPartitionedDataSet.h>
#include <vtkInformation.h>
#include <vtkDataAssembly.h>
#include <vtkDataArraySelection.h>
#include <vtkResourceFileLocator.h>
#include <vtksys/SystemTools.hxx>
#include <vtkSMPropertyHelper.h>
#include <vtkOutputWindow.h>
#include <vtkCellData.h>
#include <vtkPointData.h>
#include <vtkSMProxyManager.h>
#include <vtkSMSessionProxyManager.h>
#include <vtkSMProxySelectionModel.h>
#include <vtkSMViewProxy.h>
#include <vtkSMProperty.h>
#include <vtkProcessModule.h>

// FESAPI includes
#include <fesapi/common/DataObjectRepository.h>
#include <fesapi/common/EpcDocument.h>
#include <fesapi/eml2/TimeSeries.h>
#include <fesapi/resqml2/Grid2dRepresentation.h>
#include <fesapi/resqml2/AbstractFeature.h>
#include <fesapi/resqml2/AbstractFeatureInterpretation.h>
#include <fesapi/resqml2/AbstractIjkGridRepresentation.h>
#include <fesapi/resqml2/PointSetRepresentation.h>
#include <fesapi/resqml2/PolylineSetRepresentation.h>
#include <fesapi/resqml2/PolylineRepresentation.h>
#include <fesapi/resqml2/SubRepresentation.h>
#include <fesapi/resqml2/TriangulatedSetRepresentation.h>
#include <fesapi/resqml2/UnstructuredGridRepresentation.h>
#include <fesapi/resqml2/WellboreMarkerFrameRepresentation.h>
#include <fesapi/resqml2/WellboreFrameRepresentation.h>
#include <fesapi/resqml2/BlockedWellboreRepresentation.h>
#include <fesapi/resqml2/AbstractGridRepresentation.h>
#include <fesapi/resqml2/WellboreMarker.h>
#include <fesapi/resqml2/WellboreTrajectoryRepresentation.h>
#include <fesapi/resqml2/ContinuousProperty.h>
#include <fesapi/resqml2/DiscreteProperty.h>
#include <fesapi/resqml2/CategoricalProperty.h>
#include <fesapi/resqml2/WellboreFeature.h>
#include <fesapi/resqml2/RepresentationSetRepresentation.h>
#include <fesapi/resqml2_0_1/PropertySet.h>
#include <fesapi/resqml2_0_1/MdDatum.h>
#include <fesapi/witsml2_1/WellboreCompletion.h>
#include <fesapi/witsml2_1/WellCompletion.h>
#include <fesapi/witsml2_1/Well.h>
#include <fesapi/eml2_3/GraphicalInformationSet.h>

#ifdef WITH_ETP_SSL
#include <thread>

#include <fetpapi/etp/fesapi/FesapiHdfProxy.h>

#include <fetpapi/etp/ProtocolHandlers/DataspaceHandlers.h>
#include <fetpapi/etp/ProtocolHandlers/DiscoveryHandlers.h>
#include <fetpapi/etp/ProtocolHandlers/StoreHandlers.h>
#include <fetpapi/etp/ProtocolHandlers/DataArrayHandlers.h>

// boost includes
#include <boost/uuid/random_generator.hpp>
#endif

#include "Mapping/ResqmlIjkGridToVtkExplicitStructuredGrid.h"
#include "Mapping/ResqmlIjkGridSubRepToVtkExplicitStructuredGrid.h"
#include "Mapping/ResqmlGrid2dToVtkStructuredGrid.h"
#include "Mapping/ResqmlPointSetToVtkPolyVertex.h"
#include "Mapping/ResqmlPolylineSetToVtkPolyData.h"
#include "Mapping/ResqmlPolylineToVtkPolyData.h"
#include "Mapping/ResqmlTriangulatedSetToVtkPartitionedDataSet.h"
#include "Mapping/ResqmlUnstructuredGridToVtkUnstructuredGrid.h"
#include "Mapping/ResqmlUnstructuredGridSubRepToVtkUnstructuredGrid.h"
#include "Mapping/ResqmlWellboreTrajectoryToVtkPolyData.h"
#include "Mapping/ResqmlWellboreMarkerFrameToVtkPartitionedDataSet.h"
#include "Mapping/ResqmlWellboreFrameToVtkPartitionedDataSet.h"
#include "Mapping/ResqmlBlockedWellboreToVtkUnstructuredGrid.h"
#include "Mapping/WitsmlWellboreCompletionToVtkPartitionedDataSet.h"
#include "Mapping/WitsmlWellboreCompletionPerforationToVtkPolyData.h"
#include "Mapping/CommonAbstractObjectSetToVtkPartitionedDataSetSet.h"

#include <vtkNew.h>
#include <vtkCollection.h>
#include <vtkSMPVRepresentationProxy.h>
#include <vtkSMColorMapEditorHelper.h>

extern "C" const char* GetEnergisticsVersion() {
	return PROJECT_VERSION;
}

ResqmlDataRepositoryToVtkPartitionedDataSetCollection::ResqmlDataRepositoryToVtkPartitionedDataSetCollection()
	: _markerOrientation(false),
	_markerSize(10),
	_output(vtkSmartPointer<vtkPartitionedDataSetCollection>::New()),
	_nodeIdToMapper(),
	_selection(),
	_currentSelection(),
	_oldSelection(),
	_selectionCleared(true)
{
	auto w_assembly = vtkSmartPointer<vtkDataAssembly>::New();
	w_assembly->SetRootNodeName("data");

	_output->SetDataAssembly(w_assembly);
	_timesStepIndex.clear();

	auto energistics_libs = vtkGetLibraryPathForSymbol(GetEnergisticsVersion);
	vtkNew<vtkResourceFileLocator> locator;
	auto path = locator->Locate(energistics_libs, "PropertyKindMapping.xml");
	if (path.empty())
	{
		vtkOutputWindowDisplayWarningText("Could not find PropertyKindMapping.xml\n");
		_repository = new COMMON_NS::DataObjectRepository();
	}
	else {
		_repository = new COMMON_NS::DataObjectRepository(path);
	}
}

ResqmlDataRepositoryToVtkPartitionedDataSetCollection::~ResqmlDataRepositoryToVtkPartitionedDataSetCollection()
{
	delete _repository;
	for (const auto& w_keyVal : _nodeIdToMapper)
	{
		delete w_keyVal.second;
	}
}

MapperType getMapperType(TreeViewNodeType p_type)
{
	switch (p_type) {
	case TreeViewNodeType::Properties:
	case TreeViewNodeType::Perforation:
	case TreeViewNodeType::WellboreChannel:
	case TreeViewNodeType::WellboreMarker:
	case TreeViewNodeType::TimeSeries:
	case TreeViewNodeType::MultiRealization:
	case TreeViewNodeType::MultiRealizationTimeSeries:
		return MapperType::Data;
	case TreeViewNodeType::Unknown:
	case TreeViewNodeType::Collection:
	case TreeViewNodeType::Wellbore:
	case TreeViewNodeType::Partial:
	// The grid container folder has no VTK object — it only groups the grid's
	// Full Geometry / SubRep / BlockedWellbore reps, so a checked folder renders
	// nothing and never drags the full grid into the view.
	case TreeViewNodeType::GridContainer:
		return MapperType::Folder;
	case TreeViewNodeType::Representation:
	case TreeViewNodeType::SubRepresentation:
	case TreeViewNodeType::WellboreTrajectory:
	// BlockedWellbore produces real geometry — the subset of supporting-grid cells
	// the wellbore is blocked in (see loadBlockedWellboreMapper).
	case TreeViewNodeType::BlockedWellbore:
		return MapperType::Mapper;
	case TreeViewNodeType::WellboreMarkerFrame:
	case TreeViewNodeType::WellboreFrame:
	case TreeViewNodeType::WellboreCompletion:
		return MapperType::MapperSet;
	default:
		return MapperType::Folder;
	}
}

// Map a RESQML AbstractProperty to the matching TreeView property kind name
// ("ContinuousProperty" / "DiscreteProperty" / "CategoricalProperty"). Takes
// the wider AbstractProperty base type so it accepts pointers from both
// `TimeSeries::getPropertySet()` (returns AbstractProperty*) and
// `Representation::getValuesPropertySet()` (returns AbstractValuesProperty*).
// dynamic_cast to a sibling subclass works fine as long as RTTI is enabled
// (which it is — the existing code at lines ~1054/1082 already uses the same
// pattern on AbstractValuesProperty pointers).
//
// Used to set the "propKind" attribute on synthetic TimeSeries /
// MultiRealization / MultiRealizationTimeSeries nodes — those collapse the
// per-time / per-realization sub-tree into a single leaf, so without this
// attribute the Python side could only see the synthetic kind ("TimeSeries"
// etc.) and not the underlying property type that drives the icon and the
// color editor (Categorical/Discrete need a list-of-values editor instead
// of the continuous LUT editor).
namespace {
const char* propKindName(RESQML2_NS::AbstractProperty const* prop)
{
	if (dynamic_cast<RESQML2_NS::CategoricalProperty const*>(prop)) return "CategoricalProperty";
	if (dynamic_cast<RESQML2_NS::DiscreteProperty const*>(prop)) return "DiscreteProperty";
	if (dynamic_cast<RESQML2_NS::ContinuousProperty const*>(prop)) return "ContinuousProperty";
	return "Property";
}
}

// This function replaces the VTK function this->MakeValidNodeName(),
// which has a bug in the sorted_valid_chars array. The '.' character is placed
// before the '-' character, which is incorrect. This function uses a valid_chars
// array that correctly sorts the characters. The function checks if each character
// in the input string is valid, and adds it to the output string if it is valid.
// If the first character of the output string is not valid, an underscore is added
// to the beginning of the output string. This function is designed to create a valid
// node name from a given string.
std::string ResqmlDataRepositoryToVtkPartitionedDataSetCollection::MakeValidNodeName(const char* p_name)
{
	if (p_name == nullptr || p_name[0] == '\0')
	{
		return std::string();
	}

	const char w_sortedValidChars[] =
		"-.0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ_abcdefghijklmnopqrstuvwxyz";
	const auto w_sortedValidCharsLen = strlen(w_sortedValidChars);

	std::string w_result;
	w_result.reserve(strlen(p_name));
	for (size_t w_cc = 0, max = strlen(p_name); w_cc < max; ++w_cc)
	{
		if (std::binary_search(
			w_sortedValidChars, w_sortedValidChars + w_sortedValidCharsLen, p_name[w_cc]))
		{
			w_result += p_name[w_cc];
		}
	}

	if (w_result.empty() ||
		((w_result[0] < 'a' || w_result[0] > 'z') && (w_result[0] < 'A' || w_result[0] > 'Z') &&
			w_result[0] != '_'))
	{
		return "_" + w_result;
	}
	return w_result;
}

std::string SimplifyXmlTag(std::string p_typeRepresentation)
{
	std::string w_suffix = "Representation";
	std::string w_prefix = "Wellbore";

	if (p_typeRepresentation.size() >= w_suffix.size() && p_typeRepresentation.substr(p_typeRepresentation.size() - w_suffix.size()) == w_suffix)
	{
		p_typeRepresentation = p_typeRepresentation.substr(0, p_typeRepresentation.size() - w_suffix.size());
	}

	if (p_typeRepresentation.size() >= w_prefix.size() && p_typeRepresentation.substr(0, w_prefix.size()) == w_prefix)
	{
		p_typeRepresentation = p_typeRepresentation.substr(w_prefix.size());
	}
	return p_typeRepresentation;
}

//----------------------------------------------------------------------------
std::vector<std::string> ResqmlDataRepositoryToVtkPartitionedDataSetCollection::connect(const std::string& p_etpUrl, const std::string& p_dataPartition, const std::string& p_authConnection, const std::string& p_proxyUrl, const std::string& p_proxyAuthConnection)
{

	std::vector<std::string> w_result;
#ifdef WITH_ETP_SSL
	boost::uuids::random_generator w_gen;
	ETP_NS::InitializationParameters w_initializationParams(w_gen(), p_etpUrl, p_proxyUrl);
	w_initializationParams.setAdditionalHandshakeHeaderFields({ {"data-partition-id", p_dataPartition} });

	_session = ETP_NS::ClientSessionLaunchers::createClientSession(&w_initializationParams, p_authConnection, p_proxyAuthConnection);
	try
	{
		_session->setDataspaceProtocolHandlers(std::make_shared<ETP_NS::DataspaceHandlers>(_session.get()));
	}
	catch (const std::exception& e)
	{
		vtkOutputWindowDisplayErrorText((std::string("FESAPI error > ") + e.what()).c_str());
	}
	try
	{
		_session->setDiscoveryProtocolHandlers(std::make_shared<ETP_NS::DiscoveryHandlers>(_session.get()));
	}
	catch (const std::exception& e)
	{
		vtkOutputWindowDisplayErrorText((std::string("FESAPI error > ") + e.what()).c_str());
	}
	try
	{
		_session->setStoreProtocolHandlers(std::make_shared<ETP_NS::StoreHandlers>(_session.get()));
	}
	catch (const std::exception& e)
	{
		vtkOutputWindowDisplayErrorText((std::string("FESAPI error > ") + e.what()).c_str());
	}
	try
	{
		_session->setDataArrayProtocolHandlers(std::make_shared<ETP_NS::DataArrayHandlers>(_session.get()));
	}
	catch (const std::exception& e)
	{
		vtkOutputWindowDisplayErrorText((std::string("FESAPI error > ") + e.what()).c_str());
	}

	_repository->setHdfProxyFactory(new ETP_NS::FesapiHdfProxyFactory(_session.get()));


	std::thread w_sessionThread(&ETP_NS::ClientSession::run, _session);
	w_sessionThread.detach();

	// Wait for the ETP session to be opened
	auto w_tStart = std::chrono::high_resolution_clock::now();
	while (_session->isEtpSessionClosed())
	{
		if (std::chrono::duration<double, std::milli>(std::chrono::high_resolution_clock::now() - w_tStart).count() > 5000)
		{
			throw std::invalid_argument("Did you forget to click apply button before to connect? Time out for websocket connection" +
				std::to_string(std::chrono::duration<double, std::milli>(std::chrono::high_resolution_clock::now() - w_tStart).count()) + "ms.\n");
		}
	}

	//************ LIST DATASPACES ************
	// Wait for dataspaces to be received from server (getDataspaces is asynchronous)
	// Try multiple times with delays to give the server time to send dataspaces
	std::vector<Energistics::Etp::v12::Datatypes::Object::Dataspace> w_dataspaces;
	int attempts = 0;
	const int maxAttempts = 10;  // Try for up to 2 seconds (10 * 200ms)

	while (attempts < maxAttempts)
	{
		w_dataspaces = _session->getDataspaces();
		if (!w_dataspaces.empty())
		{
			break;  // Dataspaces received successfully
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(200));
		attempts++;
	}

	std::transform(w_dataspaces.begin(), w_dataspaces.end(), std::back_inserter(w_result),
		[](const Energistics::Etp::v12::Datatypes::Object::Dataspace& w_ds)
		{ return w_ds.uri; });

#endif

	return w_result;
}

//----------------------------------------------------------------------------
void ResqmlDataRepositoryToVtkPartitionedDataSetCollection::disconnect()
{
#ifdef WITH_ETP_SSL
	_session->close();
#endif
}

//----------------------------------------------------------------------------
std::string ResqmlDataRepositoryToVtkPartitionedDataSetCollection::addFile(const char* p_fileName)
{
	// Defense-in-depth backstop (the outermost catch is at vtkEPCCollector
	// GetAllFiles, but rebuildAssembly / dataspace paths also reach here):
	// the EpcDocument ctor (bad zip/container) and deserializeInto (bad
	// schema/gsoap) THROW on a malformed EPC; never let that escape.
	try
	{
		COMMON_NS::EpcDocument w_pck(p_fileName);
		_repository->clearWarnings();
		std::string w_message = w_pck.deserializeInto(*_repository);
		_files.insert(p_fileName); // only AFTER a successful deserialize
		w_message += buildDataAssemblyFromDataObjectRepo(p_fileName);
		return w_message;
	}
	catch (const std::exception& e)
	{
		std::string w_err = std::string("FESAPI error loading '") + p_fileName + "': " + e.what() + "\n";
		vtkOutputWindowDisplayErrorText(w_err.c_str());
		return w_err;
	}
	catch (...)
	{
		std::string w_err = std::string("Unknown error loading '") + p_fileName + "'\n";
		vtkOutputWindowDisplayErrorText(w_err.c_str());
		return w_err;
	}
}

//----------------------------------------------------------------------------
void ResqmlDataRepositoryToVtkPartitionedDataSetCollection::closeHdfProxies()
{
	for (EML2_NS::AbstractHdfProxy* proxy : _repository->getDataObjects<EML2_NS::AbstractHdfProxy>()) {
		proxy->close();
	}
}

//----------------------------------------------------------------------------
std::string ResqmlDataRepositoryToVtkPartitionedDataSetCollection::addDataspace(const char* p_dataspace)
{
#ifdef WITH_ETP_SSL
	//************ LIST RESOURCES ************
	Energistics::Etp::v12::Datatypes::Object::ContextInfo w_ctxInfo;
	w_ctxInfo.uri = p_dataspace;
	w_ctxInfo.depth = 0;
	w_ctxInfo.navigableEdges = Energistics::Etp::v12::Datatypes::Object::RelationshipKind::Both;
	w_ctxInfo.includeSecondaryTargets = false;
	w_ctxInfo.includeSecondarySources = false;
	const auto w_resources = _session->getResources(w_ctxInfo, Energistics::Etp::v12::Datatypes::Object::ContextScopeKind::targets);

	//************ GET ALL DATAOBJECTS ************
	_repository->setHdfProxyFactory(new ETP_NS::FesapiHdfProxyFactory(_session.get()));
	if (!w_resources.empty())
	{
		std::map<std::string, std::string> w_query;
		for (size_t w_i = 0; w_i < w_resources.size(); ++w_i)
		{
			w_query[std::to_string(w_i)] = w_resources[w_i].uri;
		}
		const auto w_dataobjects = _session->getDataObjects(w_query);
		for (auto& w_datoObject : w_dataobjects)
		{
			_repository->addOrReplaceGsoapProxy(w_datoObject.second.data,
				ETP_NS::EtpHelpers::getDataObjectType(w_datoObject.second.resource.uri),
				ETP_NS::EtpHelpers::getDataspaceUri(w_datoObject.second.resource.uri));
		}
	}
	else
	{
		vtkOutputWindowDisplayWarningText(("There is no dataobject in the dataspace : " + std::string(p_dataspace) + "\n").c_str());
	}
#endif
	return buildDataAssemblyFromDataObjectRepo("");
}

namespace
{
	auto lexicographicalComparison = [](const COMMON_NS::AbstractObject* p_a, const COMMON_NS::AbstractObject* p_b) -> bool
		{
			return p_a->getTitle().compare(p_b->getTitle()) < 0;
		};

	template <typename T>
	void sortAndAdd(std::vector<T> p_source, std::vector<RESQML2_NS::AbstractRepresentation const*>& p_dest)
	{
		std::sort(p_source.begin(), p_source.end(), lexicographicalComparison);
		std::move(p_source.begin(), p_source.end(), std::inserter(p_dest, p_dest.end()));
	}
}

int ResqmlDataRepositoryToVtkPartitionedDataSetCollection::resolveGroupingParent(
	resqml2::AbstractRepresentation const* p_representation, int p_parent)
{
	// Flat mode (or rep without an interpretation) — nothing to insert.
	if (_treeHierarchyMode == TreeHierarchyMode::Flat || p_representation == nullptr)
	{
		return p_parent;
	}
	auto const* w_interp = p_representation->getInterpretation();
	if (w_interp == nullptr)
	{
		return p_parent;
	}

	auto* w_assembly = _output->GetDataAssembly();
	int w_effectiveParent = p_parent;

	// ByFeatureAndInterpretation prepends a Feature node, then nests
	// the Interpretation under it.
	if (_treeHierarchyMode == TreeHierarchyMode::ByFeatureAndInterpretation)
	{
		auto const* w_feature = w_interp->getInterpretedFeature();
		if (w_feature != nullptr)
		{
			const std::string w_fNodeName = "_feature_" + w_feature->getUuid();
			int w_fId = w_assembly->FindFirstNodeWithName(w_fNodeName.c_str());
			if (w_fId == -1)
			{
				w_fId = w_assembly->AddNode(w_fNodeName.c_str(), w_effectiveParent);
				w_assembly->SetAttribute(w_fId, "type",
					std::to_string(static_cast<int>(TreeViewNodeType::Feature)).c_str());
				w_assembly->SetAttribute(w_fId, "kind", "Feature");
				const std::string w_label = MakeValidNodeName(("Feature_" + w_feature->getTitle()).c_str());
				w_assembly->SetAttribute(w_fId, "label", w_label.c_str());
				w_assembly->SetAttribute(w_fId, "title", w_feature->getTitle().c_str());
			}
			w_effectiveParent = w_fId;
		}
	}

	// Both non-Flat modes insert an Interpretation node — under root
	// for ByInterpretation, under the Feature created above for
	// ByFeatureAndInterpretation.
	const std::string w_iNodeName = "_interp_" + w_interp->getUuid();
	int w_iId = w_assembly->FindFirstNodeWithName(w_iNodeName.c_str());
	if (w_iId == -1)
	{
		w_iId = w_assembly->AddNode(w_iNodeName.c_str(), w_effectiveParent);
		w_assembly->SetAttribute(w_iId, "type",
			std::to_string(static_cast<int>(TreeViewNodeType::Interpretation)).c_str());
		w_assembly->SetAttribute(w_iId, "kind", "Interpretation");
		const std::string w_label = MakeValidNodeName(("Interpretation_" + w_interp->getTitle()).c_str());
		w_assembly->SetAttribute(w_iId, "label", w_label.c_str());
		w_assembly->SetAttribute(w_iId, "title", w_interp->getTitle().c_str());
	}
	return w_iId;
}

std::string ResqmlDataRepositoryToVtkPartitionedDataSetCollection::rebuildAssembly()
{
	// Per-node-id caches are about to become invalid (every node id
	// changes once the assembly is reset) — drop them all.
	for (auto& kv : _nodeIdToMapper)
	{
		delete kv.second;
	}
	_nodeIdToMapper.clear();
	for (auto& kv : _nodeIdToMapperSet)
	{
		delete kv.second;
	}
	_nodeIdToMapperSet.clear();

	_selection.clear();
	_currentSelection.clear();
	_oldSelection.clear();
	_selectionCleared = true;

	_blocksColors.clear();
	_blockColorsMap.clear();

	// Drop the synth-consumed UUID cache so the first traversal of
	// the rebuilt assembly can repopulate it. Without this reset,
	// searchProperties / searchTimeSeries / searchRealization would
	// skip every previously-consumed property and leave the rebuilt
	// tree without its TimeSeries / Realization synthetic nodes.
	_consumedPropUuids.clear();
	_realizationTitleToIndexAndPropertiesUuid.clear();
	_realAndTimeSeriesToIndexAndPropertiesUuid.clear();
	_realAndTimeSeriesTsUuid.clear();
	_timeSeriesUuidAndTitleToIndexAndPropertiesUuid.clear();

	// Reset the assembly to a fresh empty tree (root only) and re-set
	// the root name to "data". Initialize() reverts it to
	// vtkDataAssembly's default ("DataAssembly"), which would shift
	// every NodePath from "/data/..." to the default-named root and
	// break Python code that hard-codes "/data" (e.g.
	// representation.BlockSelectors = ['/data'] in fespp_engine.py,
	// plus the FindFirstNodeWithName("data") lookups).
	auto* w_assembly = _output->GetDataAssembly();
	if (w_assembly != nullptr)
	{
		w_assembly->Initialize();
		w_assembly->SetRootNodeName("data");
	}

	// Re-traverse every previously-loaded file so the in-memory
	// fesapi repository is reflected under the new layout. Data is
	// NOT re-read from disk — we only rebuild the assembly from
	// objects already in _repository.
	std::string w_message;
	for (const auto& w_fileName : _files)
	{
		w_message += buildDataAssemblyFromDataObjectRepo(w_fileName.c_str());
	}
	return w_message;
}

std::string ResqmlDataRepositoryToVtkPartitionedDataSetCollection::buildDataAssemblyFromDataObjectRepo(const char* p_fileName)
{
	std::vector<RESQML2_NS::AbstractRepresentation const*> w_allReps;

	// create vtkDataAssembly: create treeView in property panel
	sortAndAdd(_repository->getHorizonGrid2dRepresentationSet(), w_allReps);
	sortAndAdd(_repository->getIjkGridRepresentationSet(), w_allReps);
	sortAndAdd(_repository->getPointSetRepresentationSet(), w_allReps);
	sortAndAdd(_repository->getAllPolylineSetRepresentationSet(), w_allReps);
	sortAndAdd(_repository->getAllPolylineRepresentationSet(), w_allReps);
	sortAndAdd(_repository->getAllTriangulatedSetRepresentationSet(), w_allReps);
	sortAndAdd(_repository->getUnstructuredGridRepresentationSet(), w_allReps);

	// See https://stackoverflow.com/questions/15347123/how-to-construct-a-stdstring-from-a-stdvectorstring
	// In non-Flat tree hierarchy modes, resolveGroupingParent inserts
	// Feature/Interpretation grouping nodes above each top-level rep.
	std::string w_message = std::accumulate(std::begin(w_allReps), std::end(w_allReps), std::string{},
		[&](std::string& message, RESQML2_NS::AbstractRepresentation const* rep) -> std::string&
		{
			// Per-representation isolation: a single malformed rep is
			// skipped + logged instead of aborting the whole file.
			try
			{
				const int parentNode = resolveGroupingParent(rep, 0);
				message += searchRepresentations(rep, parentNode);
			}
			catch (const std::exception& e)
			{
				const std::string w_uuid = (rep != nullptr ? rep->getUuid() : std::string("<null>"));
				vtkOutputWindowDisplayErrorText(("Skipping representation uuid=" + w_uuid + ": " + e.what() + "\n").c_str());
			}
			return message;
		});
	// get WellboreTrajectory
	w_message += searchWellboreTrajectory(p_fileName);

	// get TimeSeries
	w_message += searchTimeSeries(p_fileName);

	// get Realizations
	w_message += searchRealization();

	return w_message;
}

int ResqmlDataRepositoryToVtkPartitionedDataSetCollection::addNodeToDataAssembly(common::AbstractObject const* object, const TreeViewNodeType type, int nodeId_parent)
{
	const int new_nodeId = _output->GetDataAssembly()->AddNode(("_" + object->getUuid()).c_str(), nodeId_parent);
	addDefaultToDataAssemblyNode(object, type, new_nodeId);

	return new_nodeId;
}

void ResqmlDataRepositoryToVtkPartitionedDataSetCollection::addDefaultToDataAssemblyNode(common::AbstractObject const* object, const TreeViewNodeType type, int nodeId)
{
	// type attribute
	_output->GetDataAssembly()->SetAttribute(nodeId, "type", std::to_string(static_cast<int>(type)).c_str());

	// label attribute (composite "kind_title" — kept for backwards display compat)
	// + 'kind' (fine type Python uses) + 'title' (raw object title) attributes
	// to avoid Python having to parse the label.
	std::string w_kind;
	std::string w_title;
	if (type == TreeViewNodeType::Collection
		|| type == TreeViewNodeType::Partial
		|| type == TreeViewNodeType::Wellbore
		|| type == TreeViewNodeType::GridContainer)
	{
		// Enum-driven kinds (not FESAPI-derived): delegate the string to enum.h.
		w_kind = treeViewNodeTypeName(type);
		w_title = object->getTitle();
	}
	else if (type == TreeViewNodeType::SubRepresentation)
	{
		// Fine kind is FESAPI-derived (e.g. "Sub") — keep SimplifyXmlTag.
		auto const* w_subrep = static_cast<RESQML2_NS::SubRepresentation const*>(object);
		w_kind = SimplifyXmlTag(object->getXmlTag());
		// getSupportingRepresentation(0) can be null on a partial/dangling subrep.
		auto const* w_support = w_subrep->getSupportingRepresentation(0);
		w_title = (w_support != nullptr ? w_support->getTitle() : std::string("<no support>")) + "_" + object->getTitle();
	}
	else
	{
		// Fine kind is FESAPI-derived (e.g. "IjkGrid", "UnstructuredGrid",
		// "ContinuousProperty", "Trajectory", "Frame"…) — keep SimplifyXmlTag.
		w_kind = SimplifyXmlTag(object->getXmlTag());
		w_title = object->getTitle();
	}
	const std::string w_representationVtkValidName = MakeValidNodeName((w_kind + "_" + w_title).c_str());
	_output->GetDataAssembly()->SetAttribute(nodeId, "label", w_representationVtkValidName.c_str());
	_output->GetDataAssembly()->SetAttribute(nodeId, "kind", w_kind.c_str());
	_output->GetDataAssembly()->SetAttribute(nodeId, "title", w_title.c_str());

	if (type != TreeViewNodeType::Partial)
	{
		// metadatas attribute
		try {
			for (uint64_t i = 0; i < object->getExtraMetadataCount(); ++i) {
				_output->GetDataAssembly()->SetAttribute(nodeId, MakeValidNodeName(object->getExtraMetadataKeyAtIndex(i).c_str()).c_str(), MakeValidNodeName(object->getExtraMetadataStringValueAtIndex(i).c_str()).c_str());
			}
		}
		catch (const std::exception& e) {
			vtkOutputWindowDisplayWarningText(("Warning: could not retrieve extra metadata for object " + object->getUuid() + ": " + e.what() + "\n").c_str());
		}

		// date creation attribute


		// version attribute
		if (!object->getVersion().empty()) {
			_output->GetDataAssembly()->SetAttribute(nodeId, "version", object->getVersion().c_str());
		}

		// format attribute
		if (!object->getFormat().empty()) {
			_output->GetDataAssembly()->SetAttribute(nodeId, "format", object->getFormat().c_str());
		}

		// editor attribute
		if (!object->getEditor().empty()) {
			_output->GetDataAssembly()->SetAttribute(nodeId, "editor", object->getEditor().c_str());
		}

		// originator attribute
		if (!object->getOriginator().empty()) {
			_output->GetDataAssembly()->SetAttribute(nodeId, "originator", object->getOriginator().c_str());
		}

		// description attribute
		if (!object->getDescription().empty()) {
			_output->GetDataAssembly()->SetAttribute(nodeId, "description", object->getDescription().c_str());
		}
	}
}


std::string ResqmlDataRepositoryToVtkPartitionedDataSetCollection::searchRepresentations(resqml2::AbstractRepresentation const* p_representation, int p_nodeId)
{
	std::string w_result;

	// The leading underscore is forced by VTK which does not support a node name starting with a digit (probably because it is a QNAME).
	const std::string w_nodeName = "_" + p_representation->getUuid();
	const int w_existingNodeId = _output->GetDataAssembly()->FindFirstNodeWithName(w_nodeName.c_str());
	
	if (w_existingNodeId == -1)
	{
		// To shorten the xmlTag by removing �Representation� from the end.
		std::string w_representationVtkValidName = "";
		std::string w_typeRepresentation = SimplifyXmlTag(p_representation->getXmlTag());

		if (p_representation->isPartial())
		{
			p_nodeId = addNodeToDataAssembly(p_representation, TreeViewNodeType::Partial, p_nodeId);
			_output->GetDataAssembly()->SetAttribute(p_nodeId, "supporttype", w_typeRepresentation.c_str());
		}
		else
		{
			auto const* w_subrep = dynamic_cast<RESQML2_NS::SubRepresentation const*>(p_representation);

			TreeViewNodeType w_type;
			if (w_subrep == nullptr) {
				w_type = TreeViewNodeType::Representation;
			}
			else { // subRep
				auto elementType = w_subrep->getElementKindOfPatch(0, 0);

				auto* supportingIjkGrid = dynamic_cast<RESQML2_NS::AbstractIjkGridRepresentation*>(w_subrep->getSupportingRepresentation(0));
				auto* supportingUnstructuredGrid = dynamic_cast<RESQML2_NS::UnstructuredGridRepresentation*>(w_subrep->getSupportingRepresentation(0));

				if (supportingIjkGrid != nullptr) { // support = ijkGrid
					if (elementType != gsoap_eml2_3::eml23__IndexableElement::cells) {
						return w_result;
					}
				}
				else if (supportingUnstructuredGrid != nullptr) { // support = unstructuredGrid
					if (elementType != gsoap_eml2_3::eml23__IndexableElement::cells &&
						elementType != gsoap_eml2_3::eml23__IndexableElement::faces) {
						return w_result;
					}
				}
				else {
					return w_result;
				}
				w_type = TreeViewNodeType::SubRepresentation;
			}
			// A grid (IjkGrid / UnstructuredGrid) is wrapped in a GridContainer
			// folder holding a "Full Geometry" rep child; the grid's SubReps and
			// BlockedWellbores then hang as siblings of Full Geometry under the
			// folder. A checked folder renders nothing, so checking a sub-object
			// never drags the whole grid into the view. The Full Geometry node
			// KEEPS the grid uuid name (_<uuid>), so every uuid-based mapper lookup
			// (loadRepresentationMapper, SubRep / BlockedWellbore supporting grid)
			// resolves to it unchanged — the tree just gains a folder above it.
			if (w_type == TreeViewNodeType::Representation &&
				(dynamic_cast<RESQML2_NS::AbstractIjkGridRepresentation const*>(p_representation) != nullptr ||
				 dynamic_cast<RESQML2_NS::UnstructuredGridRepresentation const*>(p_representation) != nullptr))
			{
				const int w_gridFolderId = _output->GetDataAssembly()->AddNode(
					("_gridfolder_" + p_representation->getUuid()).c_str(), p_nodeId);
				addDefaultToDataAssemblyNode(p_representation, TreeViewNodeType::GridContainer, w_gridFolderId);
				p_nodeId = addNodeToDataAssembly(p_representation, TreeViewNodeType::Representation, w_gridFolderId);
				_output->GetDataAssembly()->SetAttribute(p_nodeId, "title", "Full Geometry");
			}
			else
			{
				p_nodeId = addNodeToDataAssembly(p_representation, w_type, p_nodeId);
			}
		}
	}
	else
	{
		p_nodeId = w_existingNodeId;
		if (!p_representation->isPartial()) {
			int w_type;
			_output->GetDataAssembly()->GetAttribute(p_nodeId, "type", w_type);
			if (w_type == static_cast<int>(TreeViewNodeType::Partial)) {

				auto const* w_subrep = dynamic_cast<RESQML2_NS::SubRepresentation const*>(p_representation);

				TreeViewNodeType w_type;
				if (w_subrep == nullptr) {
					w_type = TreeViewNodeType::Representation;
				}
				else { // subRep
					auto elementType = w_subrep->getElementKindOfPatch(0, 0);

					auto* supportingIjkGrid = dynamic_cast<RESQML2_NS::AbstractIjkGridRepresentation*>(w_subrep->getSupportingRepresentation(0));
					auto* supportingUnstructuredGrid = dynamic_cast<RESQML2_NS::UnstructuredGridRepresentation*>(w_subrep->getSupportingRepresentation(0));

					if (supportingIjkGrid != nullptr) { // support = ijkGrid
						if (elementType != gsoap_eml2_3::eml23__IndexableElement::cells) {
							return w_result;
						}
					} 
					else if (supportingUnstructuredGrid != nullptr) { // support = unstructuredGrid
							if (elementType != gsoap_eml2_3::eml23__IndexableElement::cells &&
								elementType != gsoap_eml2_3::eml23__IndexableElement::faces) {
								return w_result;
							}
					} 
					else {
						return w_result;
					}
					w_type = TreeViewNodeType::SubRepresentation;
				}
				addDefaultToDataAssemblyNode(p_representation, w_type, p_nodeId);
			}
		}
	}

	// add sub representation with properties (only for ijkGrid and unstructured grid)
	if (dynamic_cast<RESQML2_NS::AbstractIjkGridRepresentation const*>(p_representation) != nullptr ||
		dynamic_cast<RESQML2_NS::UnstructuredGridRepresentation const*>(p_representation) != nullptr)
	{
		w_result += searchSubRepresentation(p_representation, p_nodeId);
	}

	// add properties to representation
	w_result += searchProperties(p_representation, p_nodeId);

	return w_result;
}

std::string ResqmlDataRepositoryToVtkPartitionedDataSetCollection::searchSubRepresentation(RESQML2_NS::AbstractRepresentation const* p_representation, int p_nodeParent)
{
	std::string w_message = "";
	try
	{
		auto w_subRepresentationSet = p_representation->getSubRepresentationSet();
		std::sort(w_subRepresentationSet.begin(), w_subRepresentationSet.end(), lexicographicalComparison);

		w_message = std::accumulate(std::begin(w_subRepresentationSet), std::end(w_subRepresentationSet), std::string{},
			[&](std::string& message, RESQML2_NS::SubRepresentation* b)
			{
				return message += searchRepresentations(b, _output->GetDataAssembly()->GetParent(p_nodeParent));
			});
	}
	catch (const std::exception& e)
	{
		return "Exception in FESAPI when calling getSubRepresentationSet for uuid : " + p_representation->getUuid() + " : " + e.what() + ".\n";
	}

	return w_message;
}

int ResqmlDataRepositoryToVtkPartitionedDataSetCollection::searchPropertySet(resqml2_0_1::PropertySet const* p_propSet, int p_nodeId)
{
	if (_output->GetDataAssembly()->FindFirstNodeWithName(("_" + p_propSet->getUuid()).c_str()) == -1)
	{ // verify uuid exist in treeview
	  // To shorten the xmlTag by removing �Representation� from the end.
		resqml2_0_1::PropertySet* w_parent = p_propSet->getParent();
		if (w_parent != nullptr)
		{
			p_nodeId = searchPropertySet(w_parent, p_nodeId);
		}
		if (_output->GetDataAssembly()->FindFirstNodeWithName(("_" + p_propSet->getUuid()).c_str()) == -1)
		{
			p_nodeId = addNodeToDataAssembly(p_propSet, TreeViewNodeType::Collection, p_nodeId);
		}
	}
	else
	{
		return _output->GetDataAssembly()->FindFirstNodeWithName(("_" + p_propSet->getUuid()).c_str());
	}

	return p_nodeId;
}

std::string ResqmlDataRepositoryToVtkPartitionedDataSetCollection::searchProperties(RESQML2_NS::AbstractRepresentation const* p_representation, int p_nodeParent)
{
	std::vector<RESQML2_NS::AbstractValuesProperty*> w_valuesPropertySet;
	try
	{
		w_valuesPropertySet = p_representation->getValuesPropertySet();
		std::sort(w_valuesPropertySet.begin(), w_valuesPropertySet.end(), lexicographicalComparison);
	}
	catch (const std::exception& e)
	{
		return "Exception in FESAPI when calling getValuesPropertySet with representation uuid: " + p_representation->getUuid() + " : " + e.what() + ".\n";
	}

	std::string w_result;
	int w_propertySetNodeId = p_nodeParent;
	for (auto const* w_property : w_valuesPropertySet)
	{
		try
		{
			// Skip properties already consumed by a synthetic TimeSeries
			// / MultiRealization / MultiRealizationTimeSeries node on a
			// previous addFile() call. Re-adding them here as direct rep
			// children would cause searchTimeSeries() / searchRealization()
			// to re-consume them AND create a duplicate synth.
			if (_consumedPropUuids.count(w_property->getUuid()) > 0)
			{
				continue;
			}

			if (w_property->isPartial())
			{
				if (_output->GetDataAssembly()->FindFirstNodeWithName(("_" + w_property->getUuid()).c_str()) == -1)
				{
					addNodeToDataAssembly(w_property, TreeViewNodeType::Partial, w_propertySetNodeId);
				}
				continue;
			}

			try
			{
				for (resqml2_0_1::PropertySet const* w_propertySet : w_property->getPropertySets())
				{
					w_propertySetNodeId = searchPropertySet(w_propertySet, p_nodeParent);
				}
			}
			catch (const std::exception& e)
			{
				w_result += "Warning: could not get property sets for property " + w_property->getUuid() + " : " + e.what() + " — property will be added under its parent representation.\n";
				w_propertySetNodeId = p_nodeParent;
			}

			if (_output->GetDataAssembly()->FindFirstNodeWithName(("_" + w_property->getUuid()).c_str()) == -1)
			{ // verify uuid exist in treeview
				addNodeToDataAssembly(w_property, TreeViewNodeType::Properties, w_propertySetNodeId);
			}
		}
		catch (const std::exception& e)
		{
			w_result += "Exception in FESAPI when processing property uuid: " + w_property->getUuid() + " : " + e.what() + ".\n";
		}
	}

	return w_result;
}

int ResqmlDataRepositoryToVtkPartitionedDataSetCollection::searchRepresentationSetRepresentation(resqml2::RepresentationSetRepresentation const* p_rsr, int p_nodeId)
{
	if (_output->GetDataAssembly()->FindFirstNodeWithName(("_" + p_rsr->getUuid()).c_str()) == -1)
	{ // verify uuid exist in treeview
	  // To shorten the xmlTag by removing �Representation� from the end.

		for (resqml2::RepresentationSetRepresentation* w_rsr : p_rsr->getRepresentationSetRepresentationSet())
		{
			p_nodeId = searchRepresentationSetRepresentation(w_rsr, p_nodeId);
		}
		if (_output->GetDataAssembly()->FindFirstNodeWithName(("_" + p_rsr->getUuid()).c_str()) == -1)
		{
			p_nodeId = addNodeToDataAssembly(p_rsr, TreeViewNodeType::Collection, p_nodeId);

			// DefaultGraphicalInformation
			std::vector<EML2_3_NS::GraphicalInformationSet*> gisSet = _repository->getDataObjects<EML2_3_NS::GraphicalInformationSet>();
			for (unsigned int gisIndex = 0; gisIndex < gisSet.size(); ++gisIndex) {
				EML2_3_NS::GraphicalInformationSet* graphicalInformationSet = gisSet[gisIndex];
				for (unsigned int i = 0; i < graphicalInformationSet->getGraphicalInformationSetCount(); ++i)
				{
					for (unsigned int targetIndex = 0; targetIndex < graphicalInformationSet->getTargetObjectCount(i); ++targetIndex)
					{
						COMMON_NS::AbstractObject const* targetObject = graphicalInformationSet->getTargetObject(i, targetIndex);
						if (targetObject == nullptr)
						{
							continue;
						}
						if (targetObject->getUuid() == p_rsr->getUuid())
						{
							if (graphicalInformationSet->hasDefaultColor(targetObject)) {
								uint8_t R, G, B;
								graphicalInformationSet->getDefaultRgbColor(targetObject, R, G, B);
								double dR, dG, dB;
								graphicalInformationSet->getDefaultRgbColor(targetObject, dR, dG, dB);
								_blocksColors.push_back(_output->GetDataAssembly()->GetNodePath(p_nodeId).c_str());
								_blocksColors.push_back(std::to_string(dR).c_str());
								_blocksColors.push_back(std::to_string(dG).c_str());
								_blocksColors.push_back(std::to_string(dB).c_str());
								_blockColorsMap[_output->GetDataAssembly()->GetNodePath(p_nodeId).c_str()] =
								{
									dR,
									dG,
									dB
								};
								_output->GetDataAssembly()->SetAttribute(p_nodeId, "colorRGB", (std::to_string(R) + "," + std::to_string(G) + "," + std::to_string(B)).c_str());
							}
						}

					}
				}
			}
			//
		}
	}
	else
	{
		return _output->GetDataAssembly()->FindFirstNodeWithName(("_" + p_rsr->getUuid()).c_str());
	}
	return p_nodeId;
}

std::string ResqmlDataRepositoryToVtkPartitionedDataSetCollection::searchWellboreTrajectory(const std::string& p_fileName)
{
	std::string w_result;

	for (auto* w_wellboreTrajectory : _repository->getWellboreTrajectoryRepresentationSet())
	{
		// Most-exposed parse function on malformed data: getInterpretation(),
		// getInterpretedFeature() and getMdDatum() can all return null, and any
		// FESAPI getter here can throw. Guard the whole per-trajectory body so a
		// single bad wellbore is skipped + logged, never crashing the load.
		try
		{
			auto* w_interp = w_wellboreTrajectory->getInterpretation();
			if (w_interp == nullptr)
			{
				vtkOutputWindowDisplayWarningText(("Skipping wellbore trajectory uuid=" + w_wellboreTrajectory->getUuid() + ": no interpretation\n").c_str());
				continue;
			}
			const auto* w_wellboreFeature = dynamic_cast<RESQML2_NS::WellboreFeature*>(w_interp->getInterpretedFeature());
			if (w_wellboreFeature == nullptr)
			{
				vtkOutputWindowDisplayWarningText(("Skipping wellbore trajectory uuid=" + w_wellboreTrajectory->getUuid() + ": no wellbore feature\n").c_str());
				continue;
			}

			int w_nodeId = 0;
			int w_initNodeId = 0;
			if (_output->GetDataAssembly()->FindFirstNodeWithName(("_" + w_wellboreTrajectory->getUuid()).c_str()) == -1)
			{ // verify uuid exist in treeview
			  // To shorten the xmlTag by removing �Representation� from the end.

				for (resqml2::RepresentationSetRepresentation* w_rsr : w_wellboreTrajectory->getRepresentationSetRepresentationSet())
				{
					w_initNodeId = searchRepresentationSetRepresentation(w_rsr);
				}

				if (_output->GetDataAssembly()->FindFirstNodeWithName(("_" + w_wellboreFeature->getUuid()).c_str()) == -1)
				{
					w_initNodeId = addNodeToDataAssembly(w_wellboreFeature, TreeViewNodeType::Wellbore, w_initNodeId);
				}

				std::string w_vtkValidName = "";
				if (w_wellboreTrajectory->isPartial())
				{
					w_nodeId = addNodeToDataAssembly(w_wellboreTrajectory, TreeViewNodeType::Partial, w_initNodeId);
					_output->GetDataAssembly()->SetAttribute(w_nodeId, "supporttype", std::to_string(static_cast<int>(TreeViewNodeType::WellboreTrajectory)).c_str());
				}
				else
				{
					w_nodeId = addNodeToDataAssembly(w_wellboreTrajectory, TreeViewNodeType::WellboreTrajectory, w_initNodeId);
				}
			}
			else
			{
				if (!w_wellboreTrajectory->isPartial()) {
					int w_type;
					 _output->GetDataAssembly()->GetAttribute(_output->GetDataAssembly()->FindFirstNodeWithName(("_" + w_wellboreFeature->getUuid()).c_str()), "type", w_type);
					if (w_type == static_cast<int>(TreeViewNodeType::Partial)) {
						w_nodeId = _output->GetDataAssembly()->FindFirstNodeWithName(("_" + w_wellboreTrajectory->getUuid()).c_str());
						addDefaultToDataAssemblyNode(w_wellboreTrajectory, TreeViewNodeType::WellboreTrajectory, w_nodeId);
					}
				}
			}

			// add MdDatum position attribute — getMdDatum() may be null.
			auto* w_mdDatum = w_wellboreTrajectory->getMdDatum();
			if (w_mdDatum != nullptr)
			{
				const double x_MdDatum = w_mdDatum->getXInGlobalCrs();
				const double y_MdDatum = w_mdDatum->getYInGlobalCrs();
				const double z_MdDatum = w_mdDatum->getZInGlobalCrs();
				_output->GetDataAssembly()->SetAttribute(w_nodeId, "mdDatumPosition", (std::to_string(x_MdDatum) + "," + std::to_string(y_MdDatum) + "," + std::to_string(z_MdDatum)).c_str());
			}

			w_result += searchWellboreFrame(w_wellboreTrajectory, w_initNodeId);
			w_result += searchWellboreCompletion(w_wellboreFeature, w_initNodeId);
		}
		catch (const std::exception& e)
		{
			vtkOutputWindowDisplayErrorText(("Skipping wellbore trajectory uuid=" + (w_wellboreTrajectory != nullptr ? w_wellboreTrajectory->getUuid() : std::string("<null>")) + ": " + e.what() + "\n").c_str());
			continue;
		}
	}
	return w_result;
}

std::string ResqmlDataRepositoryToVtkPartitionedDataSetCollection::searchWellboreFrame(const resqml2::WellboreTrajectoryRepresentation* p_wellboreTrajectory, int p_nodeId)
{
	std::string w_result = "";
	for (auto* w_wellboreFrame : p_wellboreTrajectory->getWellboreFrameRepresentationSet())
	{
		if (_output->GetDataAssembly()->FindFirstNodeWithName(("_" + w_wellboreFrame->getUuid()).c_str()) == -1)
		{ // verify uuid exist in treeview
			// BlockedWellbore is a WellboreFrame subclass, but it belongs UNDER its
			// supporting grid (not the well) and acts as a cell filter. Detect it by
			// XML TAG (robust — a dynamic_cast across the FESAPI .so boundary can
			// silently fail) and route it away from the plain WellboreFrame branch.
			auto* w_blockedWellbore = dynamic_cast<RESQML2_NS::BlockedWellboreRepresentation const*>(w_wellboreFrame);
			const bool w_isBlocked = w_wellboreFrame->getXmlTag().find("BlockedWellbore") != std::string::npos;
			if (w_isBlocked || w_blockedWellbore != nullptr)
			{
				// Get the typed pointer robustly: dynamic_cast can fail across the
				// FESAPI .so boundary, so fall back to the repository's typed getter
				// (no cross-module RTTI), matched by uuid.
				RESQML2_NS::BlockedWellboreRepresentation const* w_bw = w_blockedWellbore;
				if (w_bw == nullptr)
				{
					for (auto* w_cand : _repository->getDataObjects<RESQML2_NS::BlockedWellboreRepresentation>())
					{
						if (w_cand != nullptr && w_cand->getUuid() == w_wellboreFrame->getUuid())
						{
							w_bw = w_cand;
							break;
						}
					}
				}
				if (w_bw != nullptr)
				{
					addBlockedWellboreUnderGrid(w_bw, p_wellboreTrajectory->getUuid());
				}
				else
				{
					vtkOutputWindowDisplayErrorText(("[BW DIAG] " + w_wellboreFrame->getUuid()
						+ " blocked by tag but no typed match in getDataObjects — cannot read cells\n").c_str());
				}
				continue; // never show a blocked well under the wellbore
			}
		  // common with wellboreMarkerFrame & WellboreFrame
			auto* w_wellboreMarkerFrame = dynamic_cast<RESQML2_NS::WellboreMarkerFrameRepresentation const*>(w_wellboreFrame);
			if (w_wellboreMarkerFrame == nullptr)
			{ // WellboreFrame
				int w_frameNodeId = addNodeToDataAssembly(w_wellboreFrame, TreeViewNodeType::WellboreFrame, p_nodeId);
				// chanel
				for (auto* w_property : w_wellboreFrame->getValuesPropertySet())
				{
					if (w_property == nullptr)
					{
						continue;
					}
					if (w_property->isPartial())
					{
						// PARTIAL wellbore-channel stub: only Title + UUID are
						// readable. ADD it as a Partial node (the Partial branch
						// in addDefaultToDataAssemblyNode reads only
						// treeViewNodeTypeName + getTitle, and the FESAPI-metadata
						// block is guarded out for Partial — so no "cannot get
						// anything but a Title and an UUID from a partial ..."
						// throw) so the UI shows it marked !!!PARTIAL!!! and
						// uncheckable, instead of skipping it (or aborting the
						// whole wellbore trajectory). getMapperType(Partial)=Folder,
						// so even if its id reaches a selection the load loop
						// no-ops — no data access, no crash.
						if (_output->GetDataAssembly()->FindFirstNodeWithName(("_" + w_property->getUuid()).c_str()) == -1)
						{
							int w_partialId = addNodeToDataAssembly(w_property, TreeViewNodeType::Partial, w_frameNodeId);
							_output->GetDataAssembly()->SetAttribute(w_partialId, "supporttype", "WellboreChannel");
						}
						continue;
					}
					int w_nodeId = addNodeToDataAssembly(w_property, TreeViewNodeType::WellboreChannel, w_frameNodeId);
				}
			}
			else
			{ // WellboreMarkerFrame
				int w_frameNodeId = addNodeToDataAssembly(w_wellboreFrame, TreeViewNodeType::WellboreMarkerFrame, p_nodeId);
				// marker
				for (auto* w_wellboreMarker : w_wellboreMarkerFrame->getWellboreMarkerSet())
				{
					int w_nodeId = addNodeToDataAssembly(w_wellboreMarker, TreeViewNodeType::WellboreMarker, w_frameNodeId);
				}
			}
		}
	}
	return w_result;
}

// BlockedWellbore: attach a node UNDER its supporting grid (not the well) and
// ride the intersected grid-cell indices + the trajectory uuid on the node as
// attributes, so the Python side can (a) filter the grid to those cells on
// selection and (b) force the referenced trajectory to display. The per-interval
// cellIndices / gridIndices arrays are sized to the frame's interval count
// (= node count - 1); a null entry means the interval crosses no cell / grid. We
// deliberately ignore intersected faces and intersection points for now.
void ResqmlDataRepositoryToVtkPartitionedDataSetCollection::addBlockedWellboreUnderGrid(
	RESQML2_NS::BlockedWellboreRepresentation const* p_blockedWellbore,
	const std::string& p_trajectoryUuid)
{
	try
	{
		const uint64_t w_mdCount = p_blockedWellbore->getMdValuesCount();
		const uint64_t w_cellCount = p_blockedWellbore->getCellCount(); // non-null cells
		if (w_mdCount == 0 || w_cellCount == 0 || p_blockedWellbore->getSupportingGridRepresentationCount() == 0)
		{
			return;
		}

		// FESAPI fills these per-interval arrays; their size is the frame's
		// interval count, which has NO getter and is NOT always nodeCount-1 (the
		// testingPackage blocked well uses intervalCount == nodeCount). Allocate
		// to getMdValuesCount() (always >= interval count) so the fill can never
		// overflow; the loop below stops after getCellCount() non-null cells —
		// which all sit inside the written prefix — so the over-allocated tail is
		// never read. A null entry means the interval crosses no cell / grid.
		std::vector<int64_t> w_cellIndices(w_mdCount);
		const int64_t w_cellNull = p_blockedWellbore->getCellIndices(w_cellIndices.data());
		std::vector<int8_t> w_gridIndices(w_mdCount);
		const int8_t w_gridNull = p_blockedWellbore->getGridIndices(w_gridIndices.data());

		// SCAFFOLD: handle the FIRST supporting grid (the single-grid common case;
		// multi-grid blocked wells are rare — revisit when a test case needs them).
		RESQML2_NS::AbstractGridRepresentation* w_grid = p_blockedWellbore->getSupportingGridRepresentation(0);
		if (w_grid == nullptr)
		{
			return;
		}
		const int w_gridNodeId = _output->GetDataAssembly()->FindFirstNodeWithName(("_" + w_grid->getUuid()).c_str());
		if (w_gridNodeId == -1)
		{
			vtkOutputWindowDisplayWarningText(("Blocked wellbore uuid=" + p_blockedWellbore->getUuid()
				+ ": supporting grid " + w_grid->getUuid() + " not in the tree, skipping\n").c_str());
			return;
		}

		// CSV of the blocked cell indices belonging to that grid (grid index 0;
		// a null grid index is treated as grid 0). This is the Python filter input.
		// Stop once we have visited all getCellCount() non-null cells — they all
		// live in the written interval-count prefix, so we never touch the
		// over-allocated tail.
		std::string w_cellCsv;
		uint64_t w_seen = 0;  // non-null cells visited (across grids) -> stop condition
		uint64_t w_added = 0; // cells written to the CSV (this grid)
		for (uint64_t i = 0; i < w_mdCount && w_seen < w_cellCount; ++i)
		{
			if (w_cellIndices[i] == w_cellNull)
			{
				continue;
			}
			++w_seen;
			const int w_g = (w_gridIndices[i] == w_gridNull) ? 0 : static_cast<int>(w_gridIndices[i]);
			if (w_g != 0)
			{
				continue; // a cell of another supporting grid (multi-grid) — not this scaffold's grid
			}
			if (w_added > 0)
			{
				w_cellCsv += ",";
			}
			w_cellCsv += std::to_string(w_cellIndices[i]);
			++w_added;
		}

		// Attach the blocked well as a SIBLING of the grid's Full Geometry rep —
		// i.e. under the grid's container folder (the parent of the _<uuid> node),
		// not under the rep itself.
		const int w_gridFolderId = _output->GetDataAssembly()->GetParent(w_gridNodeId);
		const int w_bwNodeId = addNodeToDataAssembly(p_blockedWellbore, TreeViewNodeType::BlockedWellbore, w_gridFolderId);
		_output->GetDataAssembly()->SetAttribute(w_bwNodeId, "cellIndices", w_cellCsv.c_str());
		_output->GetDataAssembly()->SetAttribute(w_bwNodeId, "cellCount", std::to_string(w_added).c_str());
		_output->GetDataAssembly()->SetAttribute(w_bwNodeId, "supportingGridUuid", w_grid->getUuid().c_str());
		if (!p_trajectoryUuid.empty())
		{
			_output->GetDataAssembly()->SetAttribute(w_bwNodeId, "trajectoryUuid", p_trajectoryUuid.c_str());
		}
	}
	catch (const std::exception& e)
	{
		vtkOutputWindowDisplayErrorText(("Skipping blocked wellbore uuid="
			+ (p_blockedWellbore != nullptr ? p_blockedWellbore->getUuid() : std::string("<null>"))
			+ ": " + e.what() + "\n").c_str());
	}
}

std::string ResqmlDataRepositoryToVtkPartitionedDataSetCollection::searchWellboreCompletion(const RESQML2_NS::WellboreFeature* p_wellboreFeature, int p_nodeId)
{
	std::string w_result = "";
	// witsml2::Wellbore *witsmlWellbore = nullptr;
//	if (!p_wellboreFeature->isPartial())
//	{
	if (const witsml2::Wellbore* w_witsmlWellbore = dynamic_cast<witsml2::Wellbore*>(p_wellboreFeature->getWitsmlWellbore()))
	{
		for (const auto* w_wellboreCompletion : w_witsmlWellbore->getWellboreCompletionSet())
		{
			int w_completionNodeId = addNodeToDataAssembly(w_wellboreCompletion, TreeViewNodeType::WellboreCompletion, p_nodeId);
			// Iterate over the perforations.
			for (uint64_t w_perforationIndex = 0; w_perforationIndex < w_wellboreCompletion->getConnectionCount(WITSML2_1_NS::WellboreCompletion::WellReservoirConnectionType::PERFORATION); ++w_perforationIndex)
			{
				// init
				std::string w_perforationName = "Perfo";
				std::string w_perforationSkin = "";
				std::string w_perforationDiameter = "";

				// Test with Petrel rules
				auto w_petrelName = w_wellboreCompletion->getConnectionExtraMetadata(WITSML2_1_NS::WellboreCompletion::WellReservoirConnectionType::PERFORATION, w_perforationIndex, "Petrel:Name0");
				// arbitrarily select the first event name as perforation name
				if (w_petrelName.size() > 0)
				{
					w_perforationName += "_" + w_petrelName[0];
					// skin
					auto w_petrelSkin = w_wellboreCompletion->getConnectionExtraMetadata(WITSML2_1_NS::WellboreCompletion::WellReservoirConnectionType::PERFORATION, w_perforationIndex, "Petrel:Skin0");
					if (w_petrelSkin.size() > 0)
					{
						w_perforationSkin = w_petrelSkin[0];
					}
					// diameter
					auto w_petrelDiam = w_wellboreCompletion->getConnectionExtraMetadata(WITSML2_1_NS::WellboreCompletion::WellReservoirConnectionType::PERFORATION, w_perforationIndex, "Petrel:BoreholePerforatedSection0");
					if (w_petrelDiam.size() > 0)
					{
						w_perforationDiameter = w_petrelDiam[0];
					}
				}
				else
				{
					// Test with Sismage rules
					auto w_sismageName = w_wellboreCompletion->getConnectionExtraMetadata(WITSML2_1_NS::WellboreCompletion::WellReservoirConnectionType::PERFORATION, w_perforationIndex, "Sismage-CIG:Name");
					if (w_sismageName.size() > 0)
					{
						w_perforationName += "_" + w_sismageName[0];
						// skin
						auto w_sismageSkin = w_wellboreCompletion->getConnectionExtraMetadata(WITSML2_1_NS::WellboreCompletion::WellReservoirConnectionType::PERFORATION, w_perforationIndex, "Sismage-CIG:Skin0");
						if (w_sismageSkin.size() > 0)
						{
							w_perforationSkin = w_sismageSkin[0];
						}
						// diameter
						auto w_sismageDiam = w_wellboreCompletion->getConnectionExtraMetadata(WITSML2_1_NS::WellboreCompletion::WellReservoirConnectionType::PERFORATION, w_perforationIndex, "Sismage-CIG:CompletionDiameter");
						if (w_sismageDiam.size() > 0)
						{
							w_perforationDiameter = w_sismageDiam[0];
						}
					}
					else
					{
						// default
						w_perforationName += "_" + w_wellboreCompletion->getConnectionUid(WITSML2_1_NS::WellboreCompletion::WellReservoirConnectionType::PERFORATION, w_perforationIndex);
					}
				}
				w_perforationName += "__Skin_" + w_perforationSkin + "__Diam_" + w_perforationDiameter;

				int w_nodeId = _output->GetDataAssembly()->AddNode(this->MakeValidNodeName(("_" + w_wellboreCompletion->getUuid() + "_" + w_wellboreCompletion->getConnectionUid(WITSML2_1_NS::WellboreCompletion::WellReservoirConnectionType::PERFORATION, w_perforationIndex)).c_str()).c_str(), w_completionNodeId);
				_output->GetDataAssembly()->SetAttribute(w_nodeId, "label", MakeValidNodeName((w_perforationName).c_str()).c_str());
				_output->GetDataAssembly()->SetAttribute(w_nodeId, "type", std::to_string(static_cast<int>(TreeViewNodeType::Perforation)).c_str());
				_output->GetDataAssembly()->SetAttribute(w_nodeId, "kind", treeViewNodeTypeName(TreeViewNodeType::Perforation));
				_output->GetDataAssembly()->SetAttribute(w_nodeId, "title", w_perforationName.c_str());
				_output->GetDataAssembly()->SetAttribute(w_nodeId, "connection", w_wellboreCompletion->getConnectionUid(WITSML2_1_NS::WellboreCompletion::WellReservoirConnectionType::PERFORATION, w_perforationIndex).c_str());
				_output->GetDataAssembly()->SetAttribute(w_nodeId, "skin", w_perforationSkin.c_str());
				_output->GetDataAssembly()->SetAttribute(w_nodeId, "diameter", w_perforationDiameter.c_str());
			}
		}
	}
	//	}
	return w_result;
}
std::string ResqmlDataRepositoryToVtkPartitionedDataSetCollection::searchTimeSeries(const std::string& p_fileName)
{
	//_timesStep.clear();

	std::string w_message = "";
	std::vector<EML2_NS::TimeSeries*> w_timeSeriesSet;
	try
	{
		w_timeSeriesSet = _repository->getTimeSeriesSet();
	}
	catch (const std::exception& e)
	{
		w_message = w_message + "Exception in FESAPI when calling getTimeSeriesSet with file: " + p_fileName + " : " + e.what();
	}

	/****
	 *  change property parent to times serie parent
	 ****/
	for (auto const* w_timeSeries : w_timeSeriesSet)
	{
		// get properties link to Times series
		try
		{
			std::map<std::string, std::vector<int>> w_propertyNameToNodeIdSet;
			std::map<std::string, double> w_propertyNameToMinPropValue;
			std::map<std::string, double> w_propertyNameToMaxPropValue;
			std::map<std::string, std::string> w_propertyNameToKind;
			for (auto* w_prop : w_timeSeries->getPropertySet())
			{
				if (w_prop->getXmlTag() == RESQML2_NS::ContinuousProperty::XML_TAG ||
					w_prop->getXmlTag() == RESQML2_NS::DiscreteProperty::XML_TAG)
				{
					w_propertyNameToKind[w_prop->getTitle()] = propKindName(w_prop);
					auto w_nodeId = (_output->GetDataAssembly()->FindFirstNodeWithName(("_" + w_prop->getUuid()).c_str()));
					if (w_nodeId == -1)
					{
						w_message = w_message + "The property " + w_prop->getUuid() + " is not supported and consequently cannot be associated to its time series.\n";
						continue;
					}
					else
					{
						// same node parent else not supported
						const int w_parentNodeId = _output->GetDataAssembly()->GetParent(w_nodeId);
						if (w_parentNodeId != -1)
						{
							// If this property also has realization indices, it belongs to the
							// Realization+TimeSeries category. Store it there and skip the normal
							// TimeSeries grouping; searchRealization() will build the tree nodes.
							if (w_prop->hasRealizationIndices() && !w_prop->getRealizationIndices().empty()
								&& w_prop->getSingleTimestamp() != -1)
							{
								const uint32_t realIdx = w_prop->getRealizationIndices()[0];
								const size_t timeIdx = w_timeSeries->getTimestampIndex(w_prop->getSingleTimestamp());
								_timesStepIndexToISODate[timeIdx] = w_timeSeries->getTimestampAsIsoString(timeIdx);
								_timesStepIndex.push_back(timeIdx);
								_realAndTimeSeriesToIndexAndPropertiesUuid[w_prop->getTitle()][realIdx][timeIdx] = w_prop->getUuid();
								_realAndTimeSeriesTsUuid[w_prop->getTitle()] = w_timeSeries->getUuid();
								_consumedPropUuids.insert(w_prop->getUuid());
								// Leave the individual node in the tree for searchRealization() to handle
							}
							else
							{
								w_propertyNameToNodeIdSet[w_prop->getTitle()].push_back(w_nodeId);
								_consumedPropUuids.insert(w_prop->getUuid());
								if (w_prop->getSingleTimestamp() != -1)
								{
									const size_t w_timeIndexInTimeSeries = w_timeSeries->getTimestampIndex(w_prop->getSingleTimestamp());
									_timesStepIndexToISODate[w_timeIndexInTimeSeries] = w_timeSeries->getTimestampAsIsoString(w_timeIndexInTimeSeries);
									_timesStepIndex.push_back(w_timeIndexInTimeSeries);
									_timeSeriesUuidAndTitleToIndexAndPropertiesUuid[w_timeSeries->getUuid()][MakeValidNodeName((w_timeSeries->getXmlTag() + '_' + w_prop->getTitle()).c_str())][w_timeIndexInTimeSeries] = w_prop->getUuid();
								}
							}
						}
						else
						{
							w_message = w_message + "The properties of time series " + w_timeSeries->getUuid() + " aren't parent and is not supported.\n";
							continue;
						}
					}
				}
				auto* w_prop_cont = dynamic_cast<RESQML2_NS::ContinuousProperty*>(w_prop);
				if (w_prop_cont != nullptr)
				{
					auto min = w_prop_cont->getMinimumValue();
					if (!std::isnan(min))
					{
						if (w_propertyNameToMinPropValue.find(w_prop->getTitle()) == w_propertyNameToMinPropValue.end())
						{
							w_propertyNameToMinPropValue[w_prop->getTitle()] = min;
						}
						else
						{
							w_propertyNameToMinPropValue[w_prop->getTitle()] = w_propertyNameToMinPropValue[w_prop->getTitle()] > min ? min : w_propertyNameToMinPropValue[w_prop->getTitle()];
						}
					}
					auto max = w_prop_cont->getMaximumValue();
					if (!std::isnan(max))
					{
						if (w_propertyNameToMaxPropValue.find(w_prop->getTitle()) == w_propertyNameToMaxPropValue.end())
						{
							w_propertyNameToMaxPropValue[w_prop->getTitle()] = max;
						}
						else
						{
							w_propertyNameToMaxPropValue[w_prop->getTitle()] = w_propertyNameToMaxPropValue[w_prop->getTitle()] < max ? max : w_propertyNameToMaxPropValue[w_prop->getTitle()];
						}
					}
				}
				auto* w_prop_disc = dynamic_cast<RESQML2_NS::DiscreteProperty*>(w_prop);
				if (w_prop_disc != nullptr)
				{
					if (w_prop_disc->hasMinimumValue())
					{
						auto min = w_prop_disc->getMinimumValue();
						if (w_propertyNameToMinPropValue.find(w_prop->getTitle()) == w_propertyNameToMinPropValue.end())
						{
							w_propertyNameToMinPropValue[w_prop->getTitle()] = min;
						}
						else
						{
							w_propertyNameToMinPropValue[w_prop->getTitle()] = w_propertyNameToMinPropValue[w_prop->getTitle()] > min ? min : w_propertyNameToMinPropValue[w_prop->getTitle()];
						}
					}
					if (w_prop_disc->hasMaximumValue())
					{
						auto max = w_prop_disc->getMaximumValue();
						if (w_propertyNameToMaxPropValue.find(w_prop->getTitle()) == w_propertyNameToMaxPropValue.end())
						{
							w_propertyNameToMaxPropValue[w_prop->getTitle()] = max;
						}
						else
						{
							w_propertyNameToMaxPropValue[w_prop->getTitle()] = w_propertyNameToMaxPropValue[w_prop->getTitle()] < max ? max : w_propertyNameToMaxPropValue[w_prop->getTitle()];
						}
					}
				}
			}
			// erase duplicate Index
			sort(_timesStepIndex.begin(), _timesStepIndex.end());
			_timesStepIndex.erase(unique(_timesStepIndex.begin(), _timesStepIndex.end()), _timesStepIndex.end());
			for (const auto timeIndex : _timesStepIndex)
			{
				_output->GetDataAssembly()->SetAttribute(0, ("time" + std::to_string(timeIndex)).c_str(), _timesStepIndexToISODate[timeIndex].c_str());
			}

			for (const auto& w_myPair : w_propertyNameToNodeIdSet)
			{
				std::vector<int> w_propertyNodeSet = w_myPair.second;

				int w_parentNodeId = -1;
				// erase property add to treeview for group by TimeSerie
				for (auto node : w_propertyNodeSet)
				{
					w_parentNodeId = _output->GetDataAssembly()->GetParent(node);
					_output->GetDataAssembly()->RemoveNode(node);
				}
				std::string w_vtkValidName = MakeValidNodeName((w_timeSeries->getXmlTag() + '_' + w_myPair.first).c_str());
				auto w_nodeId = _output->GetDataAssembly()->AddNode(("_" + w_timeSeries->getUuid() + w_vtkValidName).c_str(), w_parentNodeId);
				_output->GetDataAssembly()->SetAttribute(w_nodeId, "label", w_vtkValidName.c_str());
				_output->GetDataAssembly()->SetAttribute(w_nodeId, "type", std::to_string(static_cast<int>(TreeViewNodeType::TimeSeries)).c_str());
				_output->GetDataAssembly()->SetAttribute(w_nodeId, "kind", treeViewNodeTypeName(TreeViewNodeType::TimeSeries));
				_output->GetDataAssembly()->SetAttribute(w_nodeId, "title", w_myPair.first.c_str());
				if (auto kindIt = w_propertyNameToKind.find(w_myPair.first); kindIt != w_propertyNameToKind.end())
				{
					_output->GetDataAssembly()->SetAttribute(w_nodeId, "propKind", kindIt->second.c_str());
				}
				if (auto it = w_propertyNameToMinPropValue.find(w_myPair.first); it != w_propertyNameToMinPropValue.end())
				{
					_output->GetDataAssembly()->SetAttribute(w_nodeId, "minvalue", std::to_string(static_cast<double>(it->second)).c_str());
				}
				if (auto it = w_propertyNameToMaxPropValue.find(w_myPair.first); it != w_propertyNameToMaxPropValue.end())
				{
					_output->GetDataAssembly()->SetAttribute(w_nodeId, "maxvalue", std::to_string(static_cast<double>(it->second)).c_str());
				}
			}
		}
		catch (const std::exception& e)
		{
			w_message = w_message + "Exception in FESAPI when calling getPropertySet with file: " + p_fileName + " : " + e.what();
		}
	}

	return w_message;
}

std::string ResqmlDataRepositoryToVtkPartitionedDataSetCollection::searchRealization()
{
	// Iterate through all properties to detect those with multiple realizations
	// Unlike TimeSeries, there is NO global "Realization" object
	// Each individual property can have hasRealizationIndices() == true

	std::string w_message = "";
	std::map<std::string, std::vector<int>> propertyNameToNodeIdSet;
	std::map<std::string, int> propertyNameToParentNode;
	std::map<std::string, double> propertyNameToGlobalMin;
	std::map<std::string, double> propertyNameToGlobalMax;
	std::map<std::string, std::string> propertyNameToKind;

	// Iterate through all representations in the repository
	std::vector<RESQML2_NS::AbstractRepresentation const*> w_allReps;

	try
	{
		// Collecter toutes les représentations par type
		sortAndAdd(_repository->getHorizonGrid2dRepresentationSet(), w_allReps);
		sortAndAdd(_repository->getIjkGridRepresentationSet(), w_allReps);
		sortAndAdd(_repository->getPointSetRepresentationSet(), w_allReps);
		sortAndAdd(_repository->getAllPolylineSetRepresentationSet(), w_allReps);
		sortAndAdd(_repository->getAllPolylineRepresentationSet(), w_allReps);
		sortAndAdd(_repository->getAllTriangulatedSetRepresentationSet(), w_allReps);
		sortAndAdd(_repository->getUnstructuredGridRepresentationSet(), w_allReps);
	}
	catch (const std::exception& e)
	{
		w_message += "Exception in FESAPI when collecting representations: " + std::string(e.what()) + "\n";
		return w_message;
	}

	// Iterate through all collected representations
	for (auto* rep : w_allReps)
	{
		if (rep->isPartial())
		{
			continue;
		}

		std::vector<RESQML2_NS::AbstractValuesProperty*> valuesPropertySet;
		try
		{
			valuesPropertySet = rep->getValuesPropertySet();
		}
		catch (const std::exception& e)
		{
			w_message += "Exception when getting properties for representation " + rep->getUuid() + " : " + e.what() + "\n";
			continue;
		}

		for (auto* prop : valuesPropertySet)
		{
			try
			{
				if (prop->isPartial())
				{
					continue;
				}

				// Skip properties already consumed by a synthetic MR / MR+TS
				// node on a previous addFile(). The synth's children stay
				// in the tree under the synth — re-iterating them here would
				// collect those children into propertyNameToNodeIdSet, then
				// `RemoveNode + AddNode` would tear the MR apart and rebuild
				// it under itself as a child (parent lookup returns the
				// existing synth, not the rep).
				if (_consumedPropUuids.count(prop->getUuid()) > 0)
				{
					continue;
				}

				// DETECTION: Check if THIS property has realization indices
				if (prop->hasRealizationIndices())
				{
					std::string propTitle = prop->getTitle();

					// Skip: already handled as Realization+TimeSeries in section 3
					if (_realAndTimeSeriesToIndexAndPropertiesUuid.count(propTitle) > 0)
					{
						continue;
					}

					// Get the realization indices
					auto realizationIndices = prop->getRealizationIndices();
					if (realizationIndices.empty())
					{
						continue;
					}
					uint32_t realizationIndex = realizationIndices[0];

					// Find the node of this property in the TreeView
					auto w_nodeId = _output->GetDataAssembly()->FindFirstNodeWithName(("_" + prop->getUuid()).c_str());
					if (w_nodeId == -1)
					{
						const std::string repUuid = rep->getUuid();
						const int repNodeId = _output->GetDataAssembly()->FindFirstNodeWithName(("_" + repUuid).c_str());
						vtkOutputWindowDisplayWarningText(("Warning: property '" + propTitle + "' (UUID: " + prop->getUuid() + ", realization: " + std::to_string(realizationIndex) + ") not found in TreeView. Parent rep UUID: " + repUuid + " (node: " + std::to_string(repNodeId) + ").\n").c_str());
						continue;
					}

					// Store in the mapping structure (per-property)
					_realizationTitleToIndexAndPropertiesUuid[propTitle][realizationIndex] = prop->getUuid();
					_consumedPropUuids.insert(prop->getUuid());

					// Collect all nodes for this property (different realizations)
					propertyNameToNodeIdSet[propTitle].push_back(w_nodeId);

					// Record the underlying property kind for the synthetic node.
					if (propertyNameToKind.find(propTitle) == propertyNameToKind.end())
					{
						propertyNameToKind[propTitle] = propKindName(prop);
					}

					// Remember the parent (same for all realizations of a property)
					if (propertyNameToParentNode.find(propTitle) == propertyNameToParentNode.end())
					{
						int parentNodeId = _output->GetDataAssembly()->GetParent(w_nodeId);
						propertyNameToParentNode[propTitle] = parentNodeId;
					}

					// Collect global min/max across all realizations (same pattern as TimeSeries)
					auto* w_prop_cont = dynamic_cast<RESQML2_NS::ContinuousProperty*>(prop);
					if (w_prop_cont)
					{
						auto minVal = w_prop_cont->getMinimumValue();
						auto maxVal = w_prop_cont->getMaximumValue();
						// Fallback: compute from actual data if metadata min/max are absent
						if (std::isnan(minVal) || std::isnan(maxVal))
						{
							try
							{
								const uint64_t count = prop->getValuesCountOfPatch(0);
								if (count > 0)
								{
									std::vector<float> vals(count);
									w_prop_cont->getFloatValuesOfPatch(0, vals.data());
									float localMin = std::numeric_limits<float>::max();
									float localMax = std::numeric_limits<float>::lowest();
									for (uint64_t vi = 0; vi < count; ++vi)
									{
										if (!std::isnan(vals[vi]))
										{
											if (vals[vi] < localMin) localMin = vals[vi];
											if (vals[vi] > localMax) localMax = vals[vi];
										}
									}
									if (localMin <= localMax)
									{
										if (std::isnan(minVal)) minVal = static_cast<double>(localMin);
										if (std::isnan(maxVal)) maxVal = static_cast<double>(localMax);
									}
								}
							}
							catch (...) {}
						}
						if (!std::isnan(minVal))
						{
							if (propertyNameToGlobalMin.find(propTitle) == propertyNameToGlobalMin.end())
								propertyNameToGlobalMin[propTitle] = minVal;
							else
								propertyNameToGlobalMin[propTitle] = propertyNameToGlobalMin[propTitle] > minVal ? minVal : propertyNameToGlobalMin[propTitle];
						}
						if (!std::isnan(maxVal))
						{
							if (propertyNameToGlobalMax.find(propTitle) == propertyNameToGlobalMax.end())
								propertyNameToGlobalMax[propTitle] = maxVal;
							else
								propertyNameToGlobalMax[propTitle] = propertyNameToGlobalMax[propTitle] < maxVal ? maxVal : propertyNameToGlobalMax[propTitle];
						}
					}
					auto* w_prop_disc = dynamic_cast<RESQML2_NS::DiscreteProperty*>(prop);
					if (w_prop_disc)
					{
						if (w_prop_disc->hasMinimumValue())
						{
							auto minVal = static_cast<double>(w_prop_disc->getMinimumValue());
							if (propertyNameToGlobalMin.find(propTitle) == propertyNameToGlobalMin.end())
								propertyNameToGlobalMin[propTitle] = minVal;
							else
								propertyNameToGlobalMin[propTitle] = propertyNameToGlobalMin[propTitle] > minVal ? minVal : propertyNameToGlobalMin[propTitle];
						}
						if (w_prop_disc->hasMaximumValue())
						{
							auto maxVal = static_cast<double>(w_prop_disc->getMaximumValue());
							if (propertyNameToGlobalMax.find(propTitle) == propertyNameToGlobalMax.end())
								propertyNameToGlobalMax[propTitle] = maxVal;
							else
								propertyNameToGlobalMax[propTitle] = propertyNameToGlobalMax[propTitle] < maxVal ? maxVal : propertyNameToGlobalMax[propTitle];
						}
					}
				}
			}
			catch (const std::exception& e)
			{
				w_message += "Exception when processing property " + prop->getUuid() + " : " + e.what() + "\n";
			}
		}
	}

	// 2. Create the hierarchy in the TreeView
	// MultiRealization parent (pure grouping) + one Properties child per
	// realization. Each child's "realization_index" attribute drives the
	// "_real_<idx>" suffix on the VTK array name so multiple realizations
	// of the same property co-exist on the partition. Selecting the parent
	// propagates to all children (see isGroupingType in enum.h); the user
	// can also check individual indices for a partial load.
	for (const auto& [propName, w_propertyNodeSet] : propertyNameToNodeIdSet)
	{
		if (w_propertyNodeSet.size() < 2)
		{
			vtkOutputWindowDisplayWarningText(("Warning: property '" + propName + "' has realization indices but only " + std::to_string(w_propertyNodeSet.size()) + " node(s) found in the TreeView — no realization group created.\n").c_str());
			continue;
		}

		int w_parentNodeId = -1;
		for (auto node : w_propertyNodeSet)
		{
			w_parentNodeId = _output->GetDataAssembly()->GetParent(node);
			_output->GetDataAssembly()->RemoveNode(node);
		}

		const std::string w_vtkValidName = MakeValidNodeName(propName.c_str());
		const int w_nodeId = _output->GetDataAssembly()->AddNode(
			("_multireal_" + w_vtkValidName).c_str(),
			w_parentNodeId
		);
		const std::string w_label = "MultiRealization_" + w_vtkValidName;
		_output->GetDataAssembly()->SetAttribute(w_nodeId, "label", w_label.c_str());
		_output->GetDataAssembly()->SetAttribute(
			w_nodeId,
			"type",
			std::to_string(static_cast<int>(TreeViewNodeType::MultiRealization)).c_str()
		);
		_output->GetDataAssembly()->SetAttribute(w_nodeId, "kind", treeViewNodeTypeName(TreeViewNodeType::MultiRealization));
		_output->GetDataAssembly()->SetAttribute(w_nodeId, "title", w_vtkValidName.c_str());
		_output->GetDataAssembly()->SetAttribute(w_nodeId, "propTitle", propName.c_str());
		if (auto kindIt = propertyNameToKind.find(propName); kindIt != propertyNameToKind.end())
		{
			_output->GetDataAssembly()->SetAttribute(w_nodeId, "propKind", kindIt->second.c_str());
		}
		_output->GetDataAssembly()->SetAttribute(w_nodeId, "realization_count",
			std::to_string(_realizationTitleToIndexAndPropertiesUuid[propName].size()).c_str());
		// CSV of actual realization indices (e.g. "23,24") — fespp_on_trame
		// uses this to label the slider with real values instead of 0..N-1.
		{
			std::string indicesCsv;
			for (const auto& [idx, _uuid] : _realizationTitleToIndexAndPropertiesUuid[propName])
			{
				if (!indicesCsv.empty()) indicesCsv += ",";
				indicesCsv += std::to_string(idx);
			}
			_output->GetDataAssembly()->SetAttribute(w_nodeId, "realization_indices", indicesCsv.c_str());
		}
		if (auto it = propertyNameToGlobalMin.find(propName); it != propertyNameToGlobalMin.end())
			_output->GetDataAssembly()->SetAttribute(w_nodeId, "minvalue", std::to_string(it->second).c_str());
		if (auto it = propertyNameToGlobalMax.find(propName); it != propertyNameToGlobalMax.end())
			_output->GetDataAssembly()->SetAttribute(w_nodeId, "maxvalue", std::to_string(it->second).c_str());

		// Children: one Properties node per realization. Standard
		// `_<propUuid>` naming reuses the existing Properties branch of
		// addDataToParent — the only difference is the `realization_index`
		// attribute which the branch reads to apply the array-name suffix.
		for (const auto& [idx, propUuid] : _realizationTitleToIndexAndPropertiesUuid[propName])
		{
			const int childId = _output->GetDataAssembly()->AddNode(
				("_" + propUuid).c_str(),
				w_nodeId
			);
			const std::string childLabel = "Realization " + std::to_string(idx);
			_output->GetDataAssembly()->SetAttribute(childId, "label", childLabel.c_str());
			_output->GetDataAssembly()->SetAttribute(childId, "type",
				std::to_string(static_cast<int>(TreeViewNodeType::Properties)).c_str());
			_output->GetDataAssembly()->SetAttribute(childId, "kind",
				treeViewNodeTypeName(TreeViewNodeType::Properties));
			_output->GetDataAssembly()->SetAttribute(childId, "realization_index",
				std::to_string(idx).c_str());
			if (auto kindIt = propertyNameToKind.find(propName); kindIt != propertyNameToKind.end())
			{
				_output->GetDataAssembly()->SetAttribute(childId, "propKind",
					kindIt->second.c_str());
			}
		}
	}

	// 3. MultiRealizationTimeSeries: parent (pure grouping) + one TimeSeries
	// child per realization. Each child reuses the standard TimeSeries
	// branch of addDataToParent — we pre-populate
	// _timeSeriesUuidAndTitleToIndexAndPropertiesUuid[tsUuid][childKey] with
	// the realization's time→uuid slice, and stamp the child's
	// "realization_index" attribute so the branch applies the
	// "_real_<idx>" suffix on time advance.
	for (const auto& [propName, realizationMap] : _realAndTimeSeriesToIndexAndPropertiesUuid)
	{
		const std::string w_vtkValidName = MakeValidNodeName(propName.c_str());
		int w_parentNodeId = -1;
		std::vector<int> nodesToRemove;
		double globalMin = std::numeric_limits<double>::max();
		double globalMax = std::numeric_limits<double>::lowest();
		bool hasMinMax = false;
		std::string mrtsKind;

		for (const auto& [realIdx, timeMap] : realizationMap)
		{
			for (const auto& [timeIdx, propUuid] : timeMap)
			{
				const int nodeId = _output->GetDataAssembly()->FindFirstNodeWithName(("_" + propUuid).c_str());
				if (nodeId == -1)
					continue;
				if (w_parentNodeId == -1)
					w_parentNodeId = _output->GetDataAssembly()->GetParent(nodeId);
				nodesToRemove.push_back(nodeId);

				auto* prop = _repository->getDataObjectByUuid<RESQML2_NS::AbstractValuesProperty>(propUuid);
				if (mrtsKind.empty() && prop)
				{
					mrtsKind = propKindName(prop);
				}
				if (auto* contProp = dynamic_cast<RESQML2_NS::ContinuousProperty*>(prop))
				{
					const auto minV = static_cast<double>(contProp->getMinimumValue());
					const auto maxV = static_cast<double>(contProp->getMaximumValue());
					if (!std::isnan(minV)) { if (minV < globalMin) globalMin = minV; hasMinMax = true; }
					if (!std::isnan(maxV)) { if (maxV > globalMax) globalMax = maxV; hasMinMax = true; }
				}
			}
		}

		if (w_parentNodeId == -1 || nodesToRemove.empty())
			continue;

		for (const int nodeId : nodesToRemove)
			_output->GetDataAssembly()->RemoveNode(nodeId);

		const std::string tsUuid = _realAndTimeSeriesTsUuid.count(propName) > 0
			? _realAndTimeSeriesTsUuid[propName] : std::string(36, '0');
		// Parent grouping node — "_<tsUuid>multirealts_<vtkValidName>" kept
		// for tree uniqueness across reps. The parent is never dispatched
		// (grouping); the dispatcher walks past it to the rep mapper.
		const std::string nodeName = "_" + tsUuid + "multirealts_" + w_vtkValidName;
		const int w_nodeId = _output->GetDataAssembly()->AddNode(nodeName.c_str(), w_parentNodeId);
		const std::string w_label = "MultiRealizationTimeSeries_" + w_vtkValidName;
		_output->GetDataAssembly()->SetAttribute(w_nodeId, "label", w_label.c_str());
		_output->GetDataAssembly()->SetAttribute(w_nodeId, "type",
			std::to_string(static_cast<int>(TreeViewNodeType::MultiRealizationTimeSeries)).c_str());
		_output->GetDataAssembly()->SetAttribute(w_nodeId, "kind", treeViewNodeTypeName(TreeViewNodeType::MultiRealizationTimeSeries));
		_output->GetDataAssembly()->SetAttribute(w_nodeId, "title", w_vtkValidName.c_str());
		_output->GetDataAssembly()->SetAttribute(w_nodeId, "propTitle", propName.c_str());
		if (!mrtsKind.empty())
		{
			_output->GetDataAssembly()->SetAttribute(w_nodeId, "propKind", mrtsKind.c_str());
		}
		_output->GetDataAssembly()->SetAttribute(w_nodeId, "realization_count",
			std::to_string(realizationMap.size()).c_str());
		// CSV of actual realization indices (e.g. "23,24") — same purpose as above.
		{
			std::string indicesCsv;
			for (const auto& [idx, _tsMap] : realizationMap)
			{
				if (!indicesCsv.empty()) indicesCsv += ",";
				indicesCsv += std::to_string(idx);
			}
			_output->GetDataAssembly()->SetAttribute(w_nodeId, "realization_indices", indicesCsv.c_str());
		}
		if (hasMinMax)
		{
			_output->GetDataAssembly()->SetAttribute(w_nodeId, "minvalue", std::to_string(globalMin).c_str());
			_output->GetDataAssembly()->SetAttribute(w_nodeId, "maxvalue", std::to_string(globalMax).c_str());
		}

		// Children: one TimeSeries node per realization. The standard
		// TimeSeries dispatch parses substr(0,36)=tsUuid, substr(36)=key.
		// We use "real_<idx>_<vtkValidName>" as the key and inject the
		// realization's time→uuid map into the existing lookup table.
		for (const auto& [realIdx, timeMap] : realizationMap)
		{
			const std::string childKey = "real_" + std::to_string(realIdx) + "_" + w_vtkValidName;
			const std::string childName = "_" + tsUuid + childKey;
			const int childId = _output->GetDataAssembly()->AddNode(childName.c_str(), w_nodeId);
			const std::string childLabel = "Realization " + std::to_string(realIdx);
			_output->GetDataAssembly()->SetAttribute(childId, "label", childLabel.c_str());
			_output->GetDataAssembly()->SetAttribute(childId, "type",
				std::to_string(static_cast<int>(TreeViewNodeType::TimeSeries)).c_str());
			_output->GetDataAssembly()->SetAttribute(childId, "kind",
				treeViewNodeTypeName(TreeViewNodeType::TimeSeries));
			_output->GetDataAssembly()->SetAttribute(childId, "realization_index",
				std::to_string(realIdx).c_str());
			if (!mrtsKind.empty())
			{
				_output->GetDataAssembly()->SetAttribute(childId, "propKind", mrtsKind.c_str());
			}

			// Populate the per-child time→uuid slice. The TimeSeries
			// branch cursor lookup is `find(t)` with t = double; for MR+TS
			// timeIdx values are pushed into _timesStepIndex as doubles so
			// matching by static_cast<double>(timeIdx) is correct.
			for (const auto& [timeIdx, propUuid] : timeMap)
			{
				_timeSeriesUuidAndTitleToIndexAndPropertiesUuid[tsUuid][childKey]
					[static_cast<double>(timeIdx)] = propUuid;
			}
		}
	}

	return w_message;
}

std::string ResqmlDataRepositoryToVtkPartitionedDataSetCollection::selectNodeId(int p_node)
{
	// _currentSelection accumulates within a batch (one batch = ParaView
	// pushing the full Selectors list, which starts with ClearSelectors →
	// clearSelection() → _currentSelection.clear()). Earlier this function
	// cleared _currentSelection itself, but that only worked because each
	// AddSelector call ran a full pipeline Update before the next clear.
	// With Update() removed for performance (one batched execution at the
	// end), clearing here would leave only the LAST AddSelector's nodes
	// visible to RequestData, dropping every prior selector in the batch.
	if (p_node != 0)
	{
		// Walk up adding ancestors to the selection. Grids are now CONTAINER
		// folders (MapperType::Folder) that render nothing, so a child rep
		// (Full Geometry / SubRep / BlockedWellbore) checking its grid folder is
		// harmless — no need to special-case the BlockedWellbore here.
		selectNodeIdParent(p_node);

		_currentSelection.insert(p_node);
		_oldSelection.erase(p_node);
	}

	// Children-walk strategy:
	//   - Legacy mode (_explicitSelection=false) : always propagate to
	//     descendants. Required for ParaView GUI's data_assembly_editor
	//     widget which collapses fully-selected subtrees to a single parent
	//     path — without this, checking a parent in the GUI would only load
	//     the parent, dropping all the children the user expected.
	//   - Explicit mode (_explicitSelection=true) : only propagate when the
	//     node is a pure grouping (Collection, Wellbore, Partial). For real
	//     objects (Representation, Property, Trajectory, ...) the selector
	//     is taken literally and children are NOT auto-included. fespp_on_trame
	//     uses this mode + UI-side expansion (`update_selected` handler)
	//     to give users per-node independent checkboxes.
	bool propagateToChildren = !_explicitSelection;
	if (_explicitSelection && p_node != 0)
	{
		uint32_t typeVal = 0;
		if (_output->GetDataAssembly()->GetAttribute(p_node, "type", typeVal))
		{
			if (isGroupingType(static_cast<TreeViewNodeType>(typeVal)))
			{
				propagateToChildren = true;
			}
		}
	}
	if (propagateToChildren)
	{
		selectNodeIdChildren(p_node);
	}

	_selection.insert(_currentSelection.begin(), _currentSelection.end());
	return "";
}

void ResqmlDataRepositoryToVtkPartitionedDataSetCollection::selectNodeIdParent(int p_node)
{
	if (_output->GetDataAssembly()->GetParent(p_node) > 0)
	{
		_currentSelection.insert(_output->GetDataAssembly()->GetParent(p_node));
		_oldSelection.erase(_output->GetDataAssembly()->GetParent(p_node));
		selectNodeIdParent(_output->GetDataAssembly()->GetParent(p_node));
	}
}
void ResqmlDataRepositoryToVtkPartitionedDataSetCollection::selectNodeIdChildren(int p_node)
{
	for (int w_nodeChild : _output->GetDataAssembly()->GetChildNodes(p_node))
	{
		_currentSelection.insert(w_nodeChild);
		_oldSelection.erase(w_nodeChild);
		selectNodeIdChildren(w_nodeChild);
	}
}

void ResqmlDataRepositoryToVtkPartitionedDataSetCollection::clearSelection()
{
	_oldSelection = _selection;
	_selection.clear();
	_currentSelection.clear();
	_selectionCleared = true;
}

void ResqmlDataRepositoryToVtkPartitionedDataSetCollection::initMapperSet(const TreeViewNodeType p_type, const int p_nodeId, const uint32_t p_nbProcess, const uint32_t p_processId)
{
	CommonAbstractObjectToVtkPartitionedDataSet* w_caotvpds = nullptr;
	const std::string w_uuid = std::string(_output->GetDataAssembly()->GetNodeName(p_nodeId)).substr(1);

	COMMON_NS::AbstractObject* const w_abstractObject = _repository->getDataObjectByUuid(w_uuid);

	if (p_type == TreeViewNodeType::WellboreCompletion)
	{
		try
		{
			if (auto* w_obj = dynamic_cast<witsml2_1::WellboreCompletion*>(w_abstractObject); w_obj != nullptr)
			{
				_nodeIdToMapperSet[p_nodeId] = new WitsmlWellboreCompletionToVtkPartitionedDataSet(w_obj, p_processId, p_nbProcess);
			}
			else
			{
				vtkOutputWindowDisplayErrorText(("Error object type in vtkDataAssembly for uuid: " + w_uuid + "\n").c_str());
			}
		}
		catch (const std::exception& e)
		{
			vtkOutputWindowDisplayErrorText(("Error when initialize uuid: " + w_uuid + "\n" + e.what()).c_str());
		}
	}
	else if (p_type == TreeViewNodeType::WellboreMarkerFrame)
	{
		try
		{
			if (auto* w_obj = dynamic_cast<RESQML2_NS::WellboreMarkerFrameRepresentation*>(w_abstractObject); w_obj != nullptr)
			{
				_nodeIdToMapperSet[p_nodeId] = new ResqmlWellboreMarkerFrameToVtkPartitionedDataSet(w_obj, p_processId, p_nbProcess);
			}
			else
			{
				vtkOutputWindowDisplayErrorText(("Error object type in vtkDataAssembly for uuid: " + w_uuid + "\n").c_str());
			}
		}
		catch (const std::exception& e)
		{
			vtkOutputWindowDisplayErrorText(("Error when initialize uuid: " + w_uuid + "\n" + e.what()).c_str());
		}
	}
	else if (p_type == TreeViewNodeType::WellboreFrame)
	{
		try
		{
			if (auto* w_obj = dynamic_cast<RESQML2_NS::WellboreFrameRepresentation*>(w_abstractObject); w_obj != nullptr)
			{
				_nodeIdToMapperSet[p_nodeId] = new ResqmlWellboreFrameToVtkPartitionedDataSet(w_obj, p_processId, p_nbProcess);
			}
			else
			{
				vtkOutputWindowDisplayErrorText(("Error object type in vtkDataAssembly for uuid: " + w_uuid + "\n").c_str());
			}
		}
		catch (const std::exception& e)
		{
			vtkOutputWindowDisplayErrorText(("Error when initialize uuid: " + w_uuid + "\n" + e.what()).c_str());
		}
	}

}

void ResqmlDataRepositoryToVtkPartitionedDataSetCollection::loadMapper(const TreeViewNodeType p_type, const int p_nodeId, const uint32_t p_nbProcess, const uint32_t p_processId)
{
	if (TreeViewNodeType::Representation == p_type)
	{
		loadRepresentationMapper(p_nodeId, p_nbProcess, p_processId);
	}
	else if (TreeViewNodeType::WellboreTrajectory == p_type)
	{
		loadWellboreTrajectoryMapper(p_nodeId);
	}
	else if (TreeViewNodeType::SubRepresentation == p_type)
	{
		loadRepresentationMapper(p_nodeId, p_nbProcess, p_processId);
	}
	else if (TreeViewNodeType::BlockedWellbore == p_type)
	{
		loadBlockedWellboreMapper(p_nodeId, p_nbProcess, p_processId);
	}
}

void ResqmlDataRepositoryToVtkPartitionedDataSetCollection::loadRepresentationMapper(const int p_nodeId, const uint32_t p_nbProcess, const uint32_t p_processId)
{
	const std::string w_uuid = std::string(_output->GetDataAssembly()->GetNodeName(p_nodeId)).substr(1);
	COMMON_NS::AbstractObject* const w_abstractObject = _repository->getDataObjectByUuid(w_uuid);

	CommonAbstractObjectToVtkPartitionedDataSet* w_caotvpds = nullptr;

	// Per-type mapper ctors call FESAPI geometry getters that can throw or
	// deref null (e.g. getSupportingRepresentation(0) on a dangling subrep).
	// Guard the whole ctor chain so a single bad rep is skipped (w_caotvpds
	// stays null -> early return below) instead of crashing.
	try
	{
	if (auto* w_ijkGrid = dynamic_cast<RESQML2_NS::AbstractIjkGridRepresentation*>(w_abstractObject); w_ijkGrid != nullptr)
	{
		w_caotvpds = new ResqmlIjkGridToVtkExplicitStructuredGrid(w_ijkGrid, p_processId, p_nbProcess);
	}
	else if (auto* w_grid2d = dynamic_cast<RESQML2_NS::Grid2dRepresentation*>(w_abstractObject); w_grid2d != nullptr)
	{
		w_caotvpds = new ResqmlGrid2dToVtkStructuredGrid(w_grid2d);
	}
	else if (auto* w_triangulatedSet = dynamic_cast<RESQML2_NS::TriangulatedSetRepresentation*>(w_abstractObject); w_triangulatedSet != nullptr)
	{
		w_caotvpds = new ResqmlTriangulatedSetToVtkPartitionedDataSet(w_triangulatedSet);
	}
	else if (auto* w_pointSet = dynamic_cast<RESQML2_NS::PointSetRepresentation*>(w_abstractObject); w_pointSet != nullptr)
	{
		w_caotvpds = new ResqmlPointSetToVtkPolyVertex(w_pointSet);
	}
	else if (auto* w_polylineSet = dynamic_cast<RESQML2_NS::PolylineSetRepresentation*>(w_abstractObject); w_polylineSet != nullptr)
	{
		w_caotvpds = new ResqmlPolylineSetToVtkPolyData(w_polylineSet);
	}
	else if (auto* w_polyline = dynamic_cast<RESQML2_NS::PolylineRepresentation*>(w_abstractObject); w_polyline != nullptr)
	{
		w_caotvpds = new ResqmlPolylineToVtkPolyData(w_polyline);
	}
	else if (auto* w_unstructuredGrid = dynamic_cast<RESQML2_NS::UnstructuredGridRepresentation*>(w_abstractObject); w_unstructuredGrid != nullptr)
	{
		if (w_unstructuredGrid->hasGeometry())
		{
			w_caotvpds = new ResqmlUnstructuredGridToVtkUnstructuredGrid(w_unstructuredGrid);
		}
		else
		{
			vtkOutputWindowDisplayErrorText(("Error: UnstructuredGrid (uuid: " + w_uuid + ") has no geometry, cannot render.\n").c_str());
		}
	}
	else if (auto* w_subRep = dynamic_cast<RESQML2_NS::SubRepresentation*>(w_abstractObject); w_subRep != nullptr)
	{
		if (auto* w_ijkGrid = dynamic_cast<RESQML2_NS::AbstractIjkGridRepresentation*>(w_subRep->getSupportingRepresentation(0)); w_ijkGrid != nullptr)
		{
			const int w_supportingNodeId = _output->GetDataAssembly()->FindFirstNodeWithName(("_" + w_ijkGrid->getUuid()).c_str());
			if (_nodeIdToMapper.find(w_supportingNodeId) == _nodeIdToMapper.end())
			{
				_nodeIdToMapper[w_supportingNodeId] = new ResqmlIjkGridToVtkExplicitStructuredGrid(w_ijkGrid);
			}
			w_caotvpds = new ResqmlIjkGridSubRepToVtkExplicitStructuredGrid(w_subRep, dynamic_cast<ResqmlIjkGridToVtkExplicitStructuredGrid*>(_nodeIdToMapper[w_supportingNodeId]));
		}
		else if (auto* w_unstructuredGrid = dynamic_cast<RESQML2_NS::UnstructuredGridRepresentation*>(w_subRep->getSupportingRepresentation(0)); w_unstructuredGrid != nullptr)
		{
			const int w_supportingNodeId = _output->GetDataAssembly()->FindFirstNodeWithName(("_" + w_unstructuredGrid->getUuid()).c_str());
			if (_nodeIdToMapper.find(w_supportingNodeId) == _nodeIdToMapper.end())
			{
				_nodeIdToMapper[w_supportingNodeId] = new ResqmlUnstructuredGridToVtkUnstructuredGrid(w_unstructuredGrid);
			}
			w_caotvpds = new ResqmlUnstructuredGridSubRepToVtkUnstructuredGrid(w_subRep, dynamic_cast<ResqmlUnstructuredGridToVtkUnstructuredGrid*>(_nodeIdToMapper[w_supportingNodeId]));
		}
		else
		{
			vtkOutputWindowDisplayWarningText(("FESPP only supports IJK Grid or UnstructuredGrid as supporting representation of subrepresentation  (for uuid: " + w_uuid + ")\n").c_str());
		}
	}
	}
	catch (const std::exception& e)
	{
		vtkOutputWindowDisplayErrorText(("Error building mapper uuid: " + w_uuid + "\n" + e.what()).c_str());
		w_caotvpds = nullptr;
	}

	if (w_caotvpds == nullptr)
	{
		return;
	}

	_nodeIdToMapper[p_nodeId] = w_caotvpds;
	try
	{ // load representation
		_nodeIdToMapper[p_nodeId]->loadVtkObject();
	}
	catch (const std::exception& e)
	{
		vtkOutputWindowDisplayErrorText(("Error when rendering uuid: " + w_uuid + "\n" + e.what()).c_str());
	}
}

void ResqmlDataRepositoryToVtkPartitionedDataSetCollection::loadBlockedWellboreMapper(const int p_nodeId, const uint32_t p_nbProcess, const uint32_t p_processId)
{
	const std::string w_uuid = std::string(_output->GetDataAssembly()->GetNodeName(p_nodeId)).substr(1);
	COMMON_NS::AbstractObject* const w_abstractObject = _repository->getDataObjectByUuid(w_uuid);

	// Robust typed pointer (dynamic_cast can fail across the FESAPI .so boundary;
	// fall back to the repository's typed getter matched by uuid).
	RESQML2_NS::BlockedWellboreRepresentation* w_blockedWellbore = dynamic_cast<RESQML2_NS::BlockedWellboreRepresentation*>(w_abstractObject);
	if (w_blockedWellbore == nullptr)
	{
		for (auto* w_cand : _repository->getDataObjects<RESQML2_NS::BlockedWellboreRepresentation>())
		{
			if (w_cand != nullptr && w_cand->getUuid() == w_uuid)
			{
				w_blockedWellbore = w_cand;
				break;
			}
		}
	}
	if (w_blockedWellbore == nullptr)
	{
		vtkOutputWindowDisplayErrorText(("Error object type in vtkDataAssembly for blocked wellbore uuid: " + w_uuid + "\n").c_str());
		return;
	}

	CommonAbstractObjectToVtkPartitionedDataSet* w_caotvpds = nullptr;
	try
	{
		// The supporting grid may be an IjkGrid or an UnstructuredGrid — reuse its
		// already-built mapper (create it if absent), then pass it as the common
		// base to the single BlockedWellbore mapper which branches on the kind.
		RESQML2_NS::AbstractGridRepresentation* w_grid = w_blockedWellbore->getSupportingGridRepresentation(0);
		if (auto* w_ijkGrid = dynamic_cast<RESQML2_NS::AbstractIjkGridRepresentation*>(w_grid); w_ijkGrid != nullptr)
		{
			const int w_supportingNodeId = _output->GetDataAssembly()->FindFirstNodeWithName(("_" + w_ijkGrid->getUuid()).c_str());
			if (_nodeIdToMapper.find(w_supportingNodeId) == _nodeIdToMapper.end())
			{
				_nodeIdToMapper[w_supportingNodeId] = new ResqmlIjkGridToVtkExplicitStructuredGrid(w_ijkGrid);
			}
			w_caotvpds = new ResqmlBlockedWellboreToVtkUnstructuredGrid(w_blockedWellbore, dynamic_cast<ResqmlAbstractRepresentationToVtkPartitionedDataSet*>(_nodeIdToMapper[w_supportingNodeId]), p_processId, p_nbProcess);
		}
		else if (auto* w_unstructuredGrid = dynamic_cast<RESQML2_NS::UnstructuredGridRepresentation*>(w_grid); w_unstructuredGrid != nullptr)
		{
			const int w_supportingNodeId = _output->GetDataAssembly()->FindFirstNodeWithName(("_" + w_unstructuredGrid->getUuid()).c_str());
			if (_nodeIdToMapper.find(w_supportingNodeId) == _nodeIdToMapper.end())
			{
				_nodeIdToMapper[w_supportingNodeId] = new ResqmlUnstructuredGridToVtkUnstructuredGrid(w_unstructuredGrid);
			}
			w_caotvpds = new ResqmlBlockedWellboreToVtkUnstructuredGrid(w_blockedWellbore, dynamic_cast<ResqmlAbstractRepresentationToVtkPartitionedDataSet*>(_nodeIdToMapper[w_supportingNodeId]), p_processId, p_nbProcess);
		}
		else
		{
			vtkOutputWindowDisplayWarningText(("FESPP only supports IjkGrid or UnstructuredGrid as the supporting grid of a blocked wellbore (uuid: " + w_uuid + ")\n").c_str());
		}
	}
	catch (const std::exception& e)
	{
		vtkOutputWindowDisplayErrorText(("Error building blocked wellbore mapper uuid: " + w_uuid + "\n" + e.what()).c_str());
		w_caotvpds = nullptr;
	}

	if (w_caotvpds == nullptr)
	{
		return;
	}
	_nodeIdToMapper[p_nodeId] = w_caotvpds;
	try
	{
		_nodeIdToMapper[p_nodeId]->loadVtkObject();
	}
	catch (const std::exception& e)
	{
		vtkOutputWindowDisplayErrorText(("Error when rendering blocked wellbore uuid: " + w_uuid + "\n" + e.what()).c_str());
	}
}

void ResqmlDataRepositoryToVtkPartitionedDataSetCollection::loadWellboreTrajectoryMapper(const int p_nodeId)
{
	const std::string w_uuid = std::string(_output->GetDataAssembly()->GetNodeName(p_nodeId)).substr(1);
	COMMON_NS::AbstractObject* const w_abstractObject = _repository->getDataObjectByUuid(w_uuid);


	if (dynamic_cast<RESQML2_NS::WellboreTrajectoryRepresentation*>(w_abstractObject) != nullptr)
	{
		_nodeIdToMapper[p_nodeId] = new ResqmlWellboreTrajectoryToVtkPolyData(static_cast<RESQML2_NS::WellboreTrajectoryRepresentation*>(w_abstractObject));
	}
	else {
		vtkOutputWindowDisplayErrorText(("Error object type in vtkDataAssembly for uuid: " + w_uuid + "\n").c_str());
		return;
	}
	try
	{ // load representation
		_nodeIdToMapper[p_nodeId]->loadVtkObject();
		return;
	}
	catch (const std::exception& e)
	{
		vtkOutputWindowDisplayErrorText(("Error when rendering uuid: " + w_uuid + "\n" + e.what()).c_str());
		return;
	}
}

void ResqmlDataRepositoryToVtkPartitionedDataSetCollection::addDataToParent(const TreeViewNodeType p_type, const int p_nodeId, const uint32_t p_nbProcess, const uint32_t p_processId)
{
	const std::string w_uuid = std::string(_output->GetDataAssembly()->GetNodeName(p_nodeId)).substr(1);

	// search representation NodeId — skip past pure grouping nodes
	// (Collection, MultiRealization, MultiRealizationTimeSeries) so the
	// rep mapper lookup hits the actual Representation node.
	int w_nodeParent = _output->GetDataAssembly()->GetParent(p_nodeId);
	uint32_t value_type;
	_output->GetDataAssembly()->GetAttribute(w_nodeParent, "type", value_type);
	TreeViewNodeType w_typeParent = static_cast<TreeViewNodeType>(value_type);
	while (w_typeParent == TreeViewNodeType::Collection
		|| w_typeParent == TreeViewNodeType::MultiRealization
		|| w_typeParent == TreeViewNodeType::MultiRealizationTimeSeries)
	{
		w_nodeParent = _output->GetDataAssembly()->GetParent(w_nodeParent);
		_output->GetDataAssembly()->GetAttribute(w_nodeParent, "type", value_type);
		w_typeParent = static_cast<TreeViewNodeType>(value_type);
	}

	if (TreeViewNodeType::Perforation == p_type)
	{
		try
		{
			if (static_cast<WitsmlWellboreCompletionToVtkPartitionedDataSet*>(_nodeIdToMapperSet[w_nodeParent]))
			{
				const char* w_connection;
				_output->GetDataAssembly()->GetAttribute(p_nodeId, "connection", w_connection);
				const char* w_name;
				_output->GetDataAssembly()->GetAttribute(p_nodeId, "label", w_name);
				const char* w_skin_s;
				_output->GetDataAssembly()->GetAttribute(p_nodeId, "skin", w_skin_s);
				const double w_skin = std::strtod(w_skin_s, nullptr);
				if (!_nodeIdToMapperSet[w_nodeParent]->existUuid(w_connection))
				{
					(static_cast<WitsmlWellboreCompletionToVtkPartitionedDataSet*>(_nodeIdToMapperSet[w_nodeParent]))->addPerforation(w_connection, w_name, w_skin);
				} // else perforation already exist
			}
		}
		catch (const std::exception& e)
		{
			vtkOutputWindowDisplayErrorText(("Error when initialize uuid: " + w_uuid + "\n" + e.what()).c_str());
		}
		return;
	}
	else if (TreeViewNodeType::WellboreChannel == p_type)
	{
		try
		{
			COMMON_NS::AbstractObject* const w_result = _repository->getDataObjectByUuid(w_uuid);
			if (static_cast<ResqmlWellboreFrameToVtkPartitionedDataSet*>(_nodeIdToMapperSet[w_nodeParent]))
			{
				if (!_nodeIdToMapperSet[w_nodeParent]->existUuid(w_uuid))
				{
					if (dynamic_cast<RESQML2_NS::AbstractValuesProperty*>(w_result) != nullptr)
					{
						(static_cast<ResqmlWellboreFrameToVtkPartitionedDataSet*>(_nodeIdToMapperSet[w_nodeParent]))->addChannel(w_uuid, static_cast<resqml2::AbstractValuesProperty*>(w_result));
					}
				}
			}
			else
			{
				vtkOutputWindowDisplayErrorText(("Error object type in vtkDataAssembly for uuid: " + w_uuid + "\n").c_str());
			}
		}
		catch (const std::exception& e)
		{
			vtkOutputWindowDisplayErrorText(("Error when initialize uuid: " + w_uuid + "\n" + e.what()).c_str());
		}
		return;
	}
	else if (TreeViewNodeType::WellboreMarker == p_type)
	{
		try
		{
			if (static_cast<ResqmlWellboreMarkerFrameToVtkPartitionedDataSet*>(_nodeIdToMapperSet[w_nodeParent]))
			{
				const std::string w_uuidParent = std::string(_output->GetDataAssembly()->GetNodeName(w_nodeParent)).substr(1);
				resqml2::WellboreMarkerFrameRepresentation* w_markerFrame = _repository->getDataObjectByUuid<resqml2::WellboreMarkerFrameRepresentation>(w_uuidParent);
				if (!_nodeIdToMapperSet[w_nodeParent]->existUuid(w_uuid))
				{
					resqml2::WellboreMarker* const w_marker = _repository->getDataObjectByUuid<resqml2::WellboreMarker>(w_uuid);
					(static_cast<ResqmlWellboreMarkerFrameToVtkPartitionedDataSet*>(_nodeIdToMapperSet[w_nodeParent]))->addMarker(w_markerFrame, w_uuid, _markerOrientation, _markerSize);
				}
				else
				{
					(static_cast<ResqmlWellboreMarkerFrameToVtkPartitionedDataSet*>(_nodeIdToMapperSet[w_nodeParent]))->changeOrientationAndSize(w_uuid, _markerOrientation, _markerSize);
				}
			}
			else
			{
				vtkOutputWindowDisplayErrorText(("Error object type in vtkDataAssembly for uuid: " + w_uuid + "\n").c_str());
			}
		}
		catch (const std::exception& e)
		{
			vtkOutputWindowDisplayErrorText(("Error when initialize uuid: " + w_uuid + "\n" + e.what()).c_str());
		}
		return;
	}
	else if (TreeViewNodeType::Properties == p_type)
	{
		try
		{
			if (auto it = _nodeIdToMapper.find(w_nodeParent); it != _nodeIdToMapper.end() && it->second)
			{
				auto* abstractRepresentation = static_cast<ResqmlAbstractRepresentationToVtkPartitionedDataSet*>(it->second);
				if (abstractRepresentation->getOutput()->GetNumberOfPartitions() == 0) {
					abstractRepresentation->loadVtkObject();
				}
				// Multi-realization child: the realization_index attribute
				// drives an array-name suffix so concurrent realizations of
				// the same property don't collide on SetName.
				const char* realIdxAttr = nullptr;
				_output->GetDataAssembly()->GetAttribute(p_nodeId, "realization_index", realIdxAttr);
				const std::string suffix = realIdxAttr
					? std::string("_real_") + realIdxAttr
					: std::string{};
				abstractRepresentation->addDataArray(w_uuid, 0, true, suffix);
			}
			else
			{
				vtkOutputWindowDisplayErrorText(("Error representation for property uuid: " + w_uuid + " not exist\n").c_str());
			}
		}
		catch (const std::exception& e)
		{
			vtkOutputWindowDisplayErrorText(("Error when initialize uuid: " + w_uuid + "\n" + e.what()).c_str());
		}
	}
	else if (TreeViewNodeType::MultiRealization == p_type
		|| TreeViewNodeType::MultiRealizationTimeSeries == p_type)
	{
		// Pure grouping — children (Properties for MR, TimeSeries for
		// MR+TS) carry the actual load. Selecting the parent in the
		// tree propagates to all children via selectNodeIdChildren()
		// (see isGroupingType in enum.h).
		return;
	}
	else if (TreeViewNodeType::TimeSeries == p_type)
	{
		try
		{
			const std::string w_tsUuid   = w_uuid.substr(0, 36);
			const std::string w_nodeName = w_uuid.substr(36);

			if (auto it = _nodeIdToMapper.find(w_nodeParent); it != _nodeIdToMapper.end() && it->second)
			{
				auto* abstractRepresentation = static_cast<ResqmlAbstractRepresentationToVtkPartitionedDataSet*>(it->second);
				if (abstractRepresentation->getOutput()->GetNumberOfPartitions() == 0)
					abstractRepresentation->loadVtkObject();

				// Use .find() to avoid creating empty entries in the map and
				// to safely skip when a step has no entry for this property
				// (in that case keep the previous data unchanged).
				auto lookupTs = [&](double t) -> std::string {
					auto tsIt = _timeSeriesUuidAndTitleToIndexAndPropertiesUuid.find(w_tsUuid);
					if (tsIt == _timeSeriesUuidAndTitleToIndexAndPropertiesUuid.end()) return {};
					auto nIt = tsIt->second.find(w_nodeName);
					if (nIt == tsIt->second.end()) return {};
					auto sIt = nIt->second.find(t);
					return sIt == nIt->second.end() ? std::string{} : sIt->second;
				};

				const bool isStepSwap = _timeStepCursor.changed();
				const std::string newUuid = lookupTs(_timeStepCursor.current());
				// Multi-realization+TS child carries a realization_index
				// attribute — the suffix keeps concurrent realizations from
				// colliding on SetName when they share an underlying title.
				// Suffix is stable across time so the array name doesn't
				// change on step advance (ColorBy bindings survive).
				const char* realIdxAttr = nullptr;
				_output->GetDataAssembly()->GetAttribute(p_nodeId, "realization_index", realIdxAttr);
				const std::string suffix = realIdxAttr
					? std::string("_real_") + realIdxAttr
					: std::string{};
				// Canonical shared name for this TS property's real arrays AND
				// its NaN placeholder (mode-aware: raw title vs MakeValidNodeName,
				// resolved inside the rep so this dispatch never duplicates the
				// _isHyperslabed branch). For a plain-TS leaf the "title" attr is
				// the raw property title (set at this file:1298); for an MR+TS
				// child the attr is ABSENT (node created without a "title"), so
				// w_nanName stays empty and the NaN operations below self-skip
				// (MR+TS NaN-on-scrub is out of scope, same as prior behaviour).
				const char* w_title = nullptr;
				_output->GetDataAssembly()->GetAttribute(p_nodeId, "title", w_title);
				const std::string w_titleStr = (w_title != nullptr) ? std::string(w_title) : std::string();
				const std::string w_nanName = abstractRepresentation->resolveDataArrayName(w_titleStr, suffix);

				if (newUuid.empty())
				{
					// No property data at the current step. NaN-fill EVERY cell so
					// the step renders transparent (app NaN opacity == 0), on BOTH
					// fresh activation and a scrub-onto-empty step swap. (Previously
					// a swap-onto-empty returned without doing anything, leaving the
					// previous step's data resident — the Change-A bug.)
					if (isStepSwap)
					{
						// Remove the PREVIOUS step's real array (UUID-tracked) first;
						// otherwise addNaNFillDataArray's idempotency guard short-
						// circuits and the stale data persists.
						const std::string oldUuid = lookupTs(_timeStepCursor.old());
						if (!oldUuid.empty())
							abstractRepresentation->deleteDataArray(oldUuid);
					}
					// Clear any stale NaN placeholder under the shared name (e.g.
					// from a prior empty step) then (re)add a fresh one. With the
					// real array just removed, the idempotency guard now passes and
					// the NaN fill actually paints; ColorBy was already bound to the
					// shared name, so the grid goes transparent on the scrub.
					if (!w_nanName.empty())
						abstractRepresentation->removeDataArrayByName(w_nanName);
					if (!w_titleStr.empty())
						abstractRepresentation->addNaNFillDataArray(
							w_titleStr, suffix, /*autoActivate*/ !isStepSwap);
					return;
				}

				if (isStepSwap)
				{
					const std::string oldUuid = lookupTs(_timeStepCursor.old());
					if (!oldUuid.empty() && oldUuid != newUuid)
						abstractRepresentation->deleteDataArray(oldUuid);
				}
				// Evict a lingering NaN placeholder under the shared name before
				// the real array reclaims it (covers the empty->data and
				// data->empty->data cycles; also robust when the old step was
				// empty so there was no real oldUuid to delete above). No-op if
				// no NaN array is present.
				if (!w_nanName.empty())
					abstractRepresentation->removeDataArrayByName(w_nanName);
				// Initial add (first time the TS property is selected) keeps the
				// auto-activate so coloring follows the user's pick. Step swaps
				// must not steal the active scalar coloring.
				abstractRepresentation->addDataArray(newUuid, 0, !isStepSwap, suffix);
			}
		}
		catch (const std::exception& e)
		{
			vtkOutputWindowDisplayErrorText(("Error when load Time Series property uuid: " + w_uuid + "\n" + e.what()).c_str());
		}
		return;
	}
}

/**
 * delete oldSelection mapper
 */
void ResqmlDataRepositoryToVtkPartitionedDataSetCollection::deleteMapper()
{
	// Reuse the existing assembly pointer instead of DeepCopy: the previous
	// _output still owns the assembly while we hold a smart pointer to it,
	// so it survives the _output reassignment. Skipping DeepCopy saves
	// O(assembly_size) work — the assembly grows with N grids/properties
	// and DeepCopy was a measurable per-add cost.
	vtkSmartPointer<vtkDataAssembly> w_Assembly = _output->GetDataAssembly();
	_output = vtkSmartPointer<vtkPartitionedDataSetCollection>::New();
	_output->SetDataAssembly(w_Assembly);

	// delete unchecked object
	for (const int w_nodeId : _oldSelection)
	{
		// retrieval of object type for nodeid
		uint32_t w_valueType;
		w_Assembly->GetAttribute(w_nodeId, "type", w_valueType);
		TreeViewNodeType valueType = static_cast<TreeViewNodeType>(w_valueType);

		// retrieval of object UUID for nodeId
		const std::string uuid_unselect = std::string(w_Assembly->GetNodeName(w_nodeId)).substr(1);

		if (valueType == TreeViewNodeType::MultiRealization
			|| valueType == TreeViewNodeType::MultiRealizationTimeSeries)
		{
			// Pure grouping — children clean up their own arrays via
			// the Properties / TimeSeries branches below.
			continue;
		}
		else if (valueType == TreeViewNodeType::TimeSeries)
		{ // Plain TimeSerie properties deselection (and MR+TS child).
			const std::string w_tsUuid   = uuid_unselect.substr(0, 36);
			const std::string w_nodeName = uuid_unselect.substr(36);

			// Walk past pure grouping nodes (Collection, MR+TS group) to
			// reach the rep mapper.
			int w_nodeParent = w_Assembly->GetParent(w_Assembly->FindFirstNodeWithName(("_" + uuid_unselect).c_str()));
			uint32_t w_typeValue;
			w_Assembly->GetAttribute(w_nodeParent, "type", w_typeValue);
			TreeViewNodeType w_typeParent = static_cast<TreeViewNodeType>(w_typeValue);
			while (w_typeParent == TreeViewNodeType::Collection
				|| w_typeParent == TreeViewNodeType::MultiRealization
				|| w_typeParent == TreeViewNodeType::MultiRealizationTimeSeries)
			{
				w_nodeParent = w_Assembly->GetParent(w_nodeParent);
				w_Assembly->GetAttribute(w_nodeParent, "type", w_typeValue);
				w_typeParent = static_cast<TreeViewNodeType>(w_typeValue);
			}
			if (_nodeIdToMapper.find(w_nodeParent) != _nodeIdToMapper.end())
			{
				auto* w_rep = static_cast<ResqmlAbstractRepresentationToVtkPartitionedDataSet*>(_nodeIdToMapper[w_nodeParent]);
				// Avoid operator[]: a .find() lookup so an empty step does not
				// default-insert a spurious [current()]="" entry into the map.
				if (auto tsIt = _timeSeriesUuidAndTitleToIndexAndPropertiesUuid.find(w_tsUuid);
					tsIt != _timeSeriesUuidAndTitleToIndexAndPropertiesUuid.end())
				{
					if (auto nIt = tsIt->second.find(w_nodeName); nIt != tsIt->second.end())
					{
						if (auto sIt = nIt->second.find(_timeStepCursor.current()); sIt != nIt->second.end())
							w_rep->deleteDataArray(sIt->second);
					}
				}
				// The current step may be empty (NaN-filled placeholder, untracked
				// by UUID); deleteDataArray cannot reach it, so remove it by name.
				// Plain-TS deselect uses an empty suffix (the placeholder was
				// added with empty suffix); MR+TS children have no "title" attr so
				// resolveDataArrayName returns empty and this self-skips.
				const char* w_dTitle = nullptr;
				w_Assembly->GetAttribute(
					w_Assembly->FindFirstNodeWithName(("_" + uuid_unselect).c_str()), "title", w_dTitle);
				if (w_dTitle != nullptr && w_dTitle[0] != '\0')
					w_rep->removeDataArrayByName(
						w_rep->resolveDataArrayName(std::string(w_dTitle), std::string()));
			}
		}
		else if (valueType == TreeViewNodeType::Properties)
		{
			int w_nodeParent = _output->GetDataAssembly()->GetParent(w_nodeId);
			uint32_t w_typeValue;
			_output->GetDataAssembly()->GetAttribute(w_nodeParent, "type", w_typeValue);
			TreeViewNodeType w_typeParent = static_cast<TreeViewNodeType>(w_typeValue);
			while (w_typeParent == TreeViewNodeType::Collection
				|| w_typeParent == TreeViewNodeType::MultiRealization
				|| w_typeParent == TreeViewNodeType::MultiRealizationTimeSeries)
			{
				w_nodeParent = _output->GetDataAssembly()->GetParent(w_nodeParent);
				_output->GetDataAssembly()->GetAttribute(w_nodeParent, "type", w_typeValue);
				w_typeParent = static_cast<TreeViewNodeType>(w_typeValue);
			}

			try
			{
				if (_nodeIdToMapper.find(w_nodeParent) != _nodeIdToMapper.end())
				{
					auto* abstractRepresentation = static_cast<ResqmlAbstractRepresentationToVtkPartitionedDataSet*>(_nodeIdToMapper[w_nodeParent]);
					abstractRepresentation->deleteDataArray(uuid_unselect);
				}
			}
			catch (const std::exception& e)
			{
				vtkOutputWindowDisplayErrorText(("Error in property unload for uuid: " + uuid_unselect + "\n" + e.what()).c_str());
			}
		}
		else if (valueType == TreeViewNodeType::WellboreMarker ||
			valueType == TreeViewNodeType::WellboreChannel)
		{
			const int w_nodeParent = w_Assembly->GetParent(w_nodeId);
			try
			{
				if (_nodeIdToMapperSet.find(w_nodeParent) != _nodeIdToMapperSet.end())
				{
					_nodeIdToMapperSet[w_nodeParent]->removeCommonAbstractObjectToVtkPartitionedDataSet(std::string(w_Assembly->GetNodeName(w_nodeId)).substr(1));
					//GetAssembly()->RemoveAllDataSetIndices(w_nodeId);
				}
			}
			catch (const std::exception& e)
			{
				vtkOutputWindowDisplayErrorText(("Error in property unload for uuid: " + uuid_unselect + "\n" + e.what()).c_str());
			}
		}
		else if (valueType == TreeViewNodeType::SubRepresentation)
		{
			try
			{
				if (auto it = _nodeIdToMapper.find(w_nodeId); it != _nodeIdToMapper.end())
				{
					std::string uuid_supporting_grid;
					auto* w_mapper = it->second;
					if (auto* subrep = dynamic_cast<ResqmlUnstructuredGridSubRepToVtkUnstructuredGrid*>(w_mapper); subrep != nullptr)
					{
						uuid_supporting_grid = subrep->unregisterToMapperSupportingGrid();
					}
					else if (auto* subrep = dynamic_cast<ResqmlIjkGridSubRepToVtkExplicitStructuredGrid*>(w_mapper); subrep != nullptr)
					{
						uuid_supporting_grid = subrep->unregisterToMapperSupportingGrid();
					}
					delete w_mapper;
					_nodeIdToMapper.erase(it);
					//GetAssembly()->RemoveAllDataSetIndices(w_nodeId);
				}
				else
				{
					vtkOutputWindowDisplayErrorText(("Error in deselection for uuid: " + uuid_unselect + "\n").c_str());
				}
			}
			catch (const std::exception& e)
			{
				vtkOutputWindowDisplayErrorText(("Error in subrepresentation unload for uuid: " + uuid_unselect + "\n" + e.what()).c_str());
			}
		}
		else if (valueType == TreeViewNodeType::BlockedWellbore)
		{
			try
			{
				if (auto it = _nodeIdToMapper.find(w_nodeId); it != _nodeIdToMapper.end())
				{
					// release the refcount taken on the supporting grid mapper
					if (auto* w_bw = dynamic_cast<ResqmlBlockedWellboreToVtkUnstructuredGrid*>(it->second); w_bw != nullptr)
					{
						w_bw->unregisterToMapperSupportingGrid();
					}
					delete it->second;
					_nodeIdToMapper.erase(it);
				}
			}
			catch (const std::exception& e)
			{
				vtkOutputWindowDisplayErrorText(("Error in blocked wellbore unload for uuid: " + uuid_unselect + "\n" + e.what()).c_str());
			}
		}
		else if (valueType == TreeViewNodeType::Representation ||
			valueType == TreeViewNodeType::WellboreTrajectory)
		{
			try
			{
				if (_nodeIdToMapper.find(w_nodeId) != _nodeIdToMapper.end())
				{
					delete _nodeIdToMapper[w_nodeId];
					_nodeIdToMapper.erase(w_nodeId);
					//GetAssembly()->RemoveAllDataSetIndices(w_nodeId);
				}
			}
			catch (const std::exception& e)
			{
				vtkOutputWindowDisplayErrorText(("Error for unload uuid: " + uuid_unselect + "\n" + e.what()).c_str());
			}
		}
		else if (valueType == TreeViewNodeType::Perforation)
			// delete child of CommonAbstractObjectSetToVtkPartitionedDataSetSet
		{
			const int w_nodeParent = w_Assembly->GetParent(w_nodeId);
			try
			{
				if (_nodeIdToMapperSet.find(w_nodeParent) != _nodeIdToMapperSet.end())
				{
					const char* w_connection;
					_output->GetDataAssembly()->GetAttribute(w_nodeId, "connection", w_connection);
					_nodeIdToMapperSet[w_nodeParent]->removeCommonAbstractObjectToVtkPartitionedDataSet(w_connection);
					//GetAssembly()->RemoveAllDataSetIndices(w_nodeId);
				}
			}
			catch (const std::exception& e)
			{
				vtkOutputWindowDisplayErrorText(("Error in WellboreCompletion unload for uuid: " + uuid_unselect + "\n" + e.what()).c_str());
			}
		}
		else if (valueType == TreeViewNodeType::WellboreCompletion ||
			valueType == TreeViewNodeType::WellboreMarkerFrame ||
			valueType == TreeViewNodeType::WellboreFrame)
		{
			try
			{
				if (_nodeIdToMapperSet.find(w_nodeId) != _nodeIdToMapperSet.end())
				{
					delete _nodeIdToMapperSet[w_nodeId];
					_nodeIdToMapperSet.erase(w_nodeId);
					//GetAssembly()->RemoveAllDataSetIndices(w_nodeId);
				}
			}
			catch (const std::exception& e)
			{
				vtkOutputWindowDisplayErrorText(("Error in WellboreCompletion unload for uuid: " + uuid_unselect + "\n" + e.what()).c_str());
			}
		}
	}
	this->RemoveAllDataSetIndicesRecursive(GetAssembly()->FindFirstNodeWithName("data"));
}

vtkPartitionedDataSetCollection* ResqmlDataRepositoryToVtkPartitionedDataSetCollection::getVtkPartitionedDatasSetCollection(const double p_time, const uint32_t p_nbProcess, const uint32_t p_processId)
{
	// Detect a TimeControl change. On such a change, ParaView triggers
	// RequestData without a fresh selectNodeId batch, so _currentSelection
	// is stale (= last node added). We must iterate _selection (the
	// cumulative checked set) to refresh every loaded property's data.
	const bool timeChanged = _timeStepCursor.set(p_time);

	if (timeChanged)
		_selectionCleared = true;

	ResetResqmlColor();
	addResqmlColor();

	if (_selectionCleared) {
		deleteMapper();
	}

	const std::set<int>& nodesToProcess = timeChanged ? _selection : _currentSelection;

	// vtkParitionedDataSetCollection - hierarchy - build.
	// On a TimeControl change we run addDataToParent for EVERY selected
	// Data node so every loaded TimeSeries array is refreshed for the new
	// step. The autoActivate flag in addDataToParent is set to false on
	// step swaps so the active scalar coloring isn't stolen by a
	// background swap.
	auto w_it = nodesToProcess.begin();
	while (w_it != nodesToProcess.end())
	{
		uint32_t w_typeValue;
		_output->GetDataAssembly()->GetAttribute(*w_it, "type", w_typeValue);
		TreeViewNodeType w_type = static_cast<TreeViewNodeType>(w_typeValue);

		// init MapperSet && save nodeId for attach to vtkPartitionedDataSetcollection
		if (getMapperType(w_type) == MapperType::MapperSet)
		{
			// initialize mapperSet with nodeId
			if (_nodeIdToMapperSet.find(*w_it) == _nodeIdToMapperSet.end())
			{
				initMapperSet(w_type, *w_it, p_nbProcess, p_processId);
			}
			++w_it;
		}
		else if (getMapperType(w_type) == MapperType::Mapper)
		{
			// load mapper with nodeId
			if (_nodeIdToMapper.find(*w_it) == _nodeIdToMapper.end())
			{
				loadMapper(w_type, *w_it, p_nbProcess, p_processId);
			}
			++w_it;
		}
		else if (getMapperType(w_type) == MapperType::Folder)
		{
			++w_it;
		}
		else if (getMapperType(w_type) == MapperType::Data)
		{
			addDataToParent(w_type, *w_it, p_nbProcess, p_processId);
			++w_it;
		}
	}

	unsigned int w_PartitionIndex = _output->GetNumberOfPartitionedDataSets();
	// foreach selection node load object — same source as the init loop above.
	for (const int w_nodeSelection : nodesToProcess)
	{
		uint32_t w_typeValue;
		_output->GetDataAssembly()->GetAttribute(w_nodeSelection, "type", w_typeValue);
		TreeViewNodeType w_type = static_cast<TreeViewNodeType>(w_typeValue);
		const std::string nodeUuid = std::string(_output->GetDataAssembly()->GetNodeName(w_nodeSelection)).substr(1);

		if (getMapperType(w_type) == MapperType::MapperSet)
		{
			// load mapper representation
			if (_nodeIdToMapperSet.find(w_nodeSelection) != _nodeIdToMapperSet.end())
			{
				try
				{
					_nodeIdToMapperSet[w_nodeSelection]->loadVtkObject();

					for (auto partition : _nodeIdToMapperSet[w_nodeSelection]->getMapperSet())
					{
						_output->SetPartitionedDataSet(w_PartitionIndex, partition->getOutput());
						_output->GetMetaData(w_PartitionIndex)->Set(vtkCompositeDataSet::NAME(), partition->getTitle() + '(' + partition->getUuid() + ')');
						if (w_type == TreeViewNodeType::WellboreCompletion)
						{
							std::string idName = std::string(GetAssembly()->GetNodeName(w_nodeSelection)) + "_" + partition->getUuid();
							int id = GetAssembly()->FindFirstNodeWithName(idName.c_str());
							GetAssembly()->AddDataSetIndex(id, w_PartitionIndex); // attach hierarchy to assembly
						}
						else if (w_type == TreeViewNodeType::WellboreFrame || w_type == TreeViewNodeType::WellboreMarkerFrame)
						{
							std::string idName = "_" + partition->getUuid();
							int id = GetAssembly()->FindFirstNodeWithName(idName.c_str());
							GetAssembly()->AddDataSetIndex(id, w_PartitionIndex); // attach hierarchy to assembly
						}
						w_PartitionIndex++;
					}
				}
				catch (const std::exception& e)
				{
					vtkOutputWindowDisplayErrorText(("FESAPI Error for uuid " + nodeUuid + " : " + e.what() + "\n").c_str());
					// The MapperSet load threw partway (e.g. a partial property
					// with no values on a wellbore frame). The partition-setting
					// loop above was skipped, leaving this selected assembly node
					// WITHOUT a partition. A dangling node makes the downstream
					// render / color path deref a null partition and hard-crash
					// the process. Attach an EMPTY partition so the node stays
					// valid (renders as nothing) instead of dangling.
					try
					{
						vtkNew<vtkPartitionedDataSet> w_emptyPds;
						_output->SetPartitionedDataSet(w_PartitionIndex, w_emptyPds);
						_output->GetMetaData(w_PartitionIndex)->Set(vtkCompositeDataSet::NAME(), nodeUuid.c_str());
						GetAssembly()->AddDataSetIndex(w_nodeSelection, w_PartitionIndex);
						w_PartitionIndex++;
					}
					catch (...)
					{
						// nothing more we can safely do; never rethrow
					}
				}
			}
		}
		else if (getMapperType(w_type) == MapperType::Mapper)
		{
			// load mapper representation
			if (_nodeIdToMapper.find(w_nodeSelection) != _nodeIdToMapper.end())
			{
				try
				{
					auto* w_mapper = _nodeIdToMapper[w_nodeSelection];
					vtkSmartPointer<vtkPartitionedDataSet> w_out =
						(w_mapper != nullptr) ? w_mapper->getOutput() : nullptr;
					if (w_out != nullptr)
					{
						_output->SetPartitionedDataSet(w_PartitionIndex, w_out);
						_output->GetMetaData(w_PartitionIndex)->Set(vtkCompositeDataSet::NAME(), w_mapper->getTitle() + '(' + w_mapper->getUuid() + ')');
						GetAssembly()->AddDataSetIndex(w_nodeSelection, w_PartitionIndex); // attach hierarchy to assembly
						w_PartitionIndex++;
					}
					else
					{
						// Null output (failed/partial geometry): attach an empty
						// partition so the node is not left dangling -> no
						// downstream null-deref crash.
						vtkNew<vtkPartitionedDataSet> w_emptyPds;
						_output->SetPartitionedDataSet(w_PartitionIndex, w_emptyPds);
						_output->GetMetaData(w_PartitionIndex)->Set(vtkCompositeDataSet::NAME(), nodeUuid.c_str());
						GetAssembly()->AddDataSetIndex(w_nodeSelection, w_PartitionIndex);
						w_PartitionIndex++;
					}
				}
				catch (const std::exception& e)
				{
					vtkOutputWindowDisplayErrorText(("FESAPI Error for uuid " + nodeUuid + " : " + e.what() + "\n").c_str());
				}
			}
		}
	}

	_selectionCleared = false;
	_output->Modified();
	// Commit the cursor so subsequent calls with no further changes see
	// changed()==false (no swap). Next external set() will bump old again.
	_timeStepCursor.commit();

	return _output;
}

void ResqmlDataRepositoryToVtkPartitionedDataSetCollection::setMarkerOrientation(bool orientation)
{
	_markerOrientation = orientation;
}

void ResqmlDataRepositoryToVtkPartitionedDataSetCollection::setMarkerSize(uint32_t size)
{
	_markerSize = size;
}

void ResqmlDataRepositoryToVtkPartitionedDataSetCollection::ResetResqmlColor()
{
	vtkSMPVRepresentationProxy* representation = getRepresentation();

	if (representation)
	{
		vtkSMColorMapEditorHelper::RemoveBlockColor(representation, "/");
		representation->Modified();
		representation->UpdatePipeline();
		representation->UpdatePipelineInformation();
	}
}

void ResqmlDataRepositoryToVtkPartitionedDataSetCollection::addResqmlColor()
{
	vtkSMPVRepresentationProxy* representation = getRepresentation();

	if (representation)
	{
		for (const auto& blockColors : _blockColorsMap)
		{
			vtkSMColorMapEditorHelper::SetBlockColor(representation, blockColors.first, blockColors.second);
		}
	}
}

vtkSMPVRepresentationProxy* ResqmlDataRepositoryToVtkPartitionedDataSetCollection::getRepresentation()
{
	vtkSMPVRepresentationProxy* representation = nullptr;

	vtkSMSessionProxyManager* activeSessionProxyManager = vtkSMProxyManager::GetProxyManager()->GetActiveSessionProxyManager();
	if (!activeSessionProxyManager)
	{
		vtkOutputWindowDisplayErrorText("vtkSMSessionProxyManager not found.\n");
	}
	else
	{
		vtkSMProxySelectionModel* selectionModel = activeSessionProxyManager->GetSelectionModel("ActiveView");
		if (!selectionModel)
		{
			vtkOutputWindowDisplayErrorText("Failed to get ActiveView selection model.");
		}
		else
		{
			vtkSMViewProxy* activeView = vtkSMViewProxy::SafeDownCast(selectionModel->GetCurrentProxy());
			if (!activeView)
			{
				vtkOutputWindowDisplayErrorText("No active view found.\n");
			}
			else
			{
				// search representation
				vtkNew<vtkCollection> representations;
				activeSessionProxyManager->GetProxies("representations", representations);

				for (int i = 0; i < representations->GetNumberOfItems(); i++)
				{
					vtkSMPVRepresentationProxy* rep =
						vtkSMPVRepresentationProxy::SafeDownCast(representations->GetItemAsObject(i));
					if (rep && rep->GetProperty("Input"))
					{
						vtkSMPropertyHelper helper(rep->GetProperty("Input"));
						if (helper.GetNumberOfElements() > 0)
						{
							return rep;
						}
					}
				}
			}
		}

	}
	return representation;
}

void ResqmlDataRepositoryToVtkPartitionedDataSetCollection::RemoveAllDataSetIndicesRecursive(int nodeId)
{
	GetAssembly()->RemoveAllDataSetIndices(nodeId);

	for (int child_id : GetAssembly()->GetChildNodes(nodeId))
	{
		RemoveAllDataSetIndicesRecursive(child_id);
	}
}