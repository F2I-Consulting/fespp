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
#ifndef __ResqmlDataRepositoryToVtkPartitionedDataSetCollection_h
#define __ResqmlDataRepositoryToVtkPartitionedDataSetCollection_h

// include system
#include <string>
#include <map>
#include <set>
#include <vector>

#include <vtkSmartPointer.h>
#include <vtkPartitionedDataSetCollection.h>
#include <vtkMultiProcessController.h>

#ifdef WITH_ETP_SSL
#include <fetpapi/etp/ClientSessionLaunchers.h>
#endif

#include "../Tools/enum.h"
#include "../Tools/Cursor.h"

namespace common
{
	class DataObjectRepository;
	class AbstractObject;
}

namespace resqml2
{
	class AbstractRepresentation;
	class RepresentationSetRepresentation;
	class PropertySet;
	class WellboreTrajectoryRepresentation;
	class WellboreFeature;
	class BlockedWellboreRepresentation;
}

namespace resqml2_0_1
{
	class PropertySet;
}

class ResqmlAbstractRepresentationToVtkPartitionedDataSet;
class CommonAbstractObjectSetToVtkPartitionedDataSetSet;
class CommonAbstractObjectToVtkPartitionedDataSet;
class vtkSMPVRepresentationProxy;

/**
 * @brief	transform a fesapi data repository to VtkPartitionedDataSetCollection
 */
class ResqmlDataRepositoryToVtkPartitionedDataSetCollection
{
public:
	ResqmlDataRepositoryToVtkPartitionedDataSetCollection();
	~ResqmlDataRepositoryToVtkPartitionedDataSetCollection();
	// --------------- PART: TreeView ---------------------
	// The assembly is a single live vtkDataAssembly, EXTENDED INCREMENTALLY
	// via AddNode/SetAttribute on every addFile/addDataspace traversal — it
	// is never rebuilt through XML serialize/parse round-trips. The only
	// from-scratch rebuild is rebuildAssembly() (explicit tree-mode change).
	//
	// NODE-NAME CONTRACT (fespp_on_trame V2 relies on it): every node name is
	// deterministic and derived from RESQML/WITSML identity — reloading the
	// same EPC set yields the same node PATHS regardless of load order:
	//   objects                  "_<uuid>"
	//   grid container folders   "_gridfolder_<uuid>" / "_propsfolder_<uuid>"
	//                            / "_subrepfolder_<uuid>"
	//   feature/interp grouping  "_feature_<uuid>" / "_interp_<uuid>"
	//   perforations             "_<uuid>_<connectionUid>" (sanitized)
	//   synthetic TimeSeries     "_<tsUuid><sanitized kind_title>"
	//   synthetic realizations   "_multireal_<sanitized title>" (+ "_<propUuid>"
	//                            children); MR+TS "_<tsUuid>multirealts_<...>"
	// No insertion-order counter is ever part of a node name. Node IDs (ints)
	// are NOT stable across rebuilds — always address nodes by path.
	vtkDataAssembly* GetAssembly() { return _output->GetDataAssembly(); };

	//---------------------------------

	// for EPC reader
	std::string addFile(const char *p_file);
	// for EPC reader
	void closeHdfProxies();

	// for ETP source
	std::string addDataspace(const char *p_dataspace);

	// for ETP connection
	std::vector<std::string> connect(const std::string &p_etpUrl, const std::string &p_dataPartition, const std::string &p_authConnection, const std::string& p_proxyUrl, const std::string& p_proxyConnection);
	void disconnect();

	// Wellbore Options
	void setMarkerOrientation(bool p_orientation);
	void setMarkerSize(uint32_t p_size);

	// Selection mode. When false (default), selectNodeId propagates to
	// ALL descendants of the matched node (legacy compat with the
	// ParaView GUI's data_assembly_editor widget). When true,
	// propagation only happens for pure grouping types — see
	// isGroupingType in enum.h. Used by fespp_on_trame for its
	// independent-selection treeview.
	void setExplicitSelection(bool value) { _explicitSelection = value; }

	// Layout of the assembly tree. Changing the value flags the
	// assembly for a rebuild — call rebuildAssembly() afterwards to
	// re-traverse the already-loaded fesapi repository with the new
	// layout. See TreeHierarchyMode in enum.h.
	void setTreeHierarchyMode(TreeHierarchyMode mode) { _treeHierarchyMode = mode; }

	// Drop the current vtkDataAssembly and per-node mapper/selection
	// caches, then re-traverse the in-memory fesapi repository to
	// rebuild the assembly from scratch with the current
	// TreeHierarchyMode. Lets a mode change apply without re-reading
	// EPC files from disk. Returns concatenated warnings/messages
	// from the traversal (same convention as
	// buildDataAssemblyFromDataObjectRepo).
	std::string rebuildAssembly();

	vtkPartitionedDataSetCollection *getVtkPartitionedDatasSetCollection(const double p_time, const uint32_t p_nbProcess = 1, const uint32_t p_processId = 0);
	vtkPartitionedDataSetCollection* getVtkPartitionedDatasSetCollection() { return _output; };

	std::vector<double> getTimes() { return _timesStepIndex; };

	/**
	 * @return selection parent
	 */
	std::string selectNodeId(int p_nodeId);
	void clearSelection();

	void addResqmlColor();

private:
	std::string buildDataAssemblyFromDataObjectRepo(const char *p_fileName);
	int addNodeToDataAssembly(common::AbstractObject const* object,const TreeViewNodeType type, int nodeId_parent); // return new nodeId
	void addDefaultToDataAssemblyNode(common::AbstractObject const* object, const TreeViewNodeType type, int nodeId);
	// Find-or-create a grid container grouping sub-folder (properties/SubRep/BW).
	// Idempotent via synthetic name "<prefix><gridUuid>".
	int findOrCreateGridSubFolder(const std::string& p_namePrefix, const std::string& p_gridUuid,
		TreeViewNodeType p_type, const char* p_title, int p_containerId);
	// SIBLING resolver: from a grid Property/TS/MR node whose chain ends at
	// PropertiesFolder->GridContainer, return the geometry rep node id ("_<uuid>"
	// Representation child of the GridContainer). Returns -1 for non-grid nodes.
	int resolveGridGeometryRepId(int p_nodeId);

	// A blocked wellbore has no RESQML property of its own: it borrows its
	// supporting grid's CELL arrays, restricted to the cells it crosses. Push
	// them from the grid geometry rep p_gridGeomNodeId (with its mapper, which
	// the caller has just resolved and thus knows to be alive) to every CHECKED
	// blocked wellbore of that grid. No-op for a non-grid rep, and never creates
	// a mapper — an unchecked wellbore is skipped. Also the EVICTION path: after
	// a deleteDataArray on the grid, the same push drops the mirrored copy.
	void fanOutCellDataToBlockedWellbores(int p_gridGeomNodeId,
		ResqmlAbstractRepresentationToVtkPartitionedDataSet* p_gridMapper);

	// Helper for the alternate tree hierarchy modes
	// (ByInterpretation, ByFeatureAndInterpretation). Given a
	// top-level representation and a logical parent (typically
	// 0 = root), returns the effective parent node id — inserting
	// Feature and/or Interpretation grouping nodes between root and
	// the rep based on _treeHierarchyMode. Idempotent: existing
	// grouping nodes are reused (looked up by uuid). Returns
	// p_parent unchanged for Flat mode or when the rep has no
	// interpretation.
	int resolveGroupingParent(resqml2::AbstractRepresentation const* p_representation, int p_parent);

	std::string searchWellboreTrajectory(const std::string& p_fileName);												  // traj
	std::string searchWellboreFrame(const resqml2::WellboreTrajectoryRepresentation* w_wellboreTrajectory, int p_nodeId); // frame/markerFrame + chanel + marker
	// BlockedWellbore: attaches a node UNDER its supporting grid (not the well) +
	// rides the intersected cell indices / trajectory uuid as node attributes.
	void addBlockedWellboreUnderGrid(const resqml2::BlockedWellboreRepresentation* p_blockedWellbore, const std::string& p_trajectoryUuid);
	std::string searchWellboreCompletion(const resqml2::WellboreFeature* w_wellboreTrajectory, int p_nodeId);			  // completion + perforation
	std::string searchRepresentations(resqml2::AbstractRepresentation const *p_representation, int p_nodeId = 0 /* 0 is root's id*/);
	int searchRepresentationSetRepresentation(resqml2::RepresentationSetRepresentation const *p_rsr, int p_nodeId = 0 /* 0 is root's id*/);
	std::string searchSubRepresentation(resqml2::AbstractRepresentation const *p_representation, int p_nodeParent);
	std::string searchTimeSeries(const std::string &p_fileName);
	std::string searchRealization();
	int searchPropertySet(resqml2_0_1::PropertySet const *p_propSet, int p_nodeId);
	std::string searchProperties(resqml2::AbstractRepresentation const *p_representation, int p_nodeParent);

	void ResetResqmlColor();

	void selectNodeIdParent(int p_nodeId);
	void selectNodeIdChildren(int p_nodeId);

	void RemoveAllDataSetIndicesRecursive(int nodeId);

	vtkSMPVRepresentationProxy* getRepresentation();

	/**
	 * delete _oldSelection mapper
	 */
	void deleteMapper();
	/**
	 * initialize _nodeIdToMapperSet
	 */
	void initMapperSet(const TreeViewNodeType p_type, const int p_nodeId, const uint32_t p_nbProcess, const uint32_t p_processId);
	/**
	 * initialize and load _nodeIdToMapper
	 */
	void loadMapper(const TreeViewNodeType p_type, const int p_nodeId,const uint32_t p_nbProcess, const uint32_t p_processId);
	void loadRepresentationMapper(const int p_nodeId, const uint32_t p_nbProcess, const uint32_t p_processId);
	void loadWellboreTrajectoryMapper(const int p_nodeId);
	// BlockedWellbore: builds the subset of its supporting grid's cells the
	// wellbore is blocked in (IjkGrid or UnstructuredGrid support).
	void loadBlockedWellboreMapper(const int p_nodeId, const uint32_t p_nbProcess, const uint32_t p_processId);
	/**
 * add data to parent nodeId
 */
	void addDataToParent(const TreeViewNodeType p_type, const int p_nodeId, const uint32_t p_nbProcess, const uint32_t p_processId);

	// This function replaces the VTK function vtkDataAssembly::MakeValidNodeName(),
	// which has a bug in the sorted_valid_chars array. The '.' character is placed
	// before the '-' character, which is incorrect. This function uses a valid_chars
	// array that correctly sorts the characters. The function checks if each character
	// in the input string is valid, and adds it to the _output string if it is valid.
	// If the first character of the _output string is not valid, an underscore is added
	// to the beginning of the _output string. This function is designed to create a valid
	// node name from a given string.
	std::string MakeValidNodeName(const char *p_name);

	bool _markerOrientation;
	uint32_t _markerSize;

	// See setExplicitSelection. Default false → legacy "propagate to
	// all descendants on selection" behaviour preserved for ParaView
	// GUI users.
	bool _explicitSelection = false;

	// See setTreeHierarchyMode. Default Flat → legacy assembly
	// layout. Modes ByInterpretation / ByFeatureAndInterpretation
	// insert Feature / Interpretation grouping nodes above the
	// representations.
	TreeHierarchyMode _treeHierarchyMode = TreeHierarchyMode::Flat;

	common::DataObjectRepository *_repository;

	vtkSmartPointer<vtkPartitionedDataSetCollection> _output;

	std::map<int, CommonAbstractObjectToVtkPartitionedDataSet *> _nodeIdToMapper;		   // index of VtkDataAssembly to CommonAbstractObjectToVtkPartitionedDataSet
	std::map<int, CommonAbstractObjectSetToVtkPartitionedDataSetSet *> _nodeIdToMapperSet; // index of VtkDataAssembly to CommonAbstractObjectSetToVtkPartitionedDataSetSet

	//\/          uuid             title            index        prop_uuid
	std::map<std::string, std::map<std::string, std::map<double, std::string>>> _timeSeriesUuidAndTitleToIndexAndPropertiesUuid;

	std::set<int> _selection;
	std::set<int> _currentSelection;
	std::set<int> _oldSelection;
	bool _selectionCleared;

	std::set<std::string> _files;

	// Property UUIDs already consumed by a synthetic TimeSeries /
	// MultiRealization / MultiRealizationTimeSeries node. searchProperties()
	// skips them so re-running addFile() after the first synth pass doesn't
	// re-introduce them as direct rep children (which would then be
	// re-consumed AND a fresh synth created, duplicating the synthetic node).
	// Populated lazily by searchTimeSeries() and searchRealization() the
	// first time they group these properties under a synth.
	std::set<std::string> _consumedPropUuids;

	// time step values
	std::map<double, std::string> _timesStepIndexToISODate;
	std::vector<double> _timesStepIndex;
	// (current, old) pair. Updated in getVtkPartitionedDatasSetCollection
	// via set(p_time); committed at the end so the next call sees no change.
	Cursor<double> _timeStepCursor{ 0.0 };

	// realization values
	// Note: Unlike TimeSeries, no global UUID because realizations are per-property
	//      prop_title       realization_index   prop_uuid
	std::map<std::string, std::map<uint32_t, std::string>> _realizationTitleToIndexAndPropertiesUuid;

	// Properties with BOTH multi-realization AND TimeSeries (Realization parent + TimeSeries children)
	// prop_title → realization_index → time_step_index → prop_uuid
	std::map<std::string, std::map<uint32_t, std::map<size_t, std::string>>> _realAndTimeSeriesToIndexAndPropertiesUuid;
	// prop_title → TimeSeries UUID (for node naming: "_<tsUuid>realts_<N>_<propVtkName>")
	std::map<std::string, std::string> _realAndTimeSeriesTsUuid;

	std::vector<const char*> _blocksColors;
	std::map<std::string, std::array<double, 3>> _blockColorsMap;

#ifdef WITH_ETP_SSL
	std::shared_ptr<ETP_NS::ClientSession> _session;
#endif
};
#endif
