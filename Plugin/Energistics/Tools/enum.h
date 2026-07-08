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

#ifndef ENUM_H
#define ENUM_H

#include <string>

enum class MapperType
{
	Folder,
	Data,
	Mapper,
	MapperSet
};

// Tree node kinds set as the "type" attribute on each vtkDataAssembly
// node. Python (the trame app) reads them as strings via the parallel
// "kind" attribute — see treeViewNodeTypeName below.
enum class TreeViewNodeType
{
	Unknown,
	Collection,
	Representation,
	SubRepresentation,
	Properties,
	Wellbore,
	WellboreTrajectory,
	WellboreFrame,
	WellboreChannel,
	WellboreMarkerFrame,
	WellboreMarker,
	WellboreCompletion,
	TimeSeries,
	Realization,
	Perforation,
	Partial,
	// Synthetic types created by searchRealization() to fold per-
	// realization (and per-realization+per-timestep) property nodes
	// into a single tree leaf. Appended on purpose: existing
	// serialized 'type' int values are unchanged.
	MultiRealization,
	MultiRealizationTimeSeries,
	// Grouping nodes inserted by the alternate tree hierarchy modes
	// (ByInterpretation, ByFeatureAndInterpretation). They have no
	// VTK object behind them — selecting one propagates to all
	// descendants.
	Feature,
	Interpretation,
	// Appended on purpose (keeps existing serialized int values stable).
	// A BlockedWellbore is a WellboreFrame subclass, but FESPP attaches it
	// UNDER its supporting grid (not the well) — selecting it acts as a cell
	// filter on that grid (the cell indices ride on the node as attributes).
	BlockedWellbore,
	// Appended on purpose (keeps existing serialized int values stable).
	// A grid (IjkGrid / UnstructuredGrid) is wrapped in a GridContainer folder
	// that holds a "Full Geometry" rep child plus the grid's SubReps and
	// BlockedWellbores as sibling reps. The folder has no VTK object — a checked
	// folder renders nothing — so checking a sub-object never drags the full
	// grid geometry into the view.
	GridContainer,
	// Appended on purpose (keeps existing serialized int values stable).
	// A per-grid folder that groups ALL of a grid's BlockedWellbores under ONE
	// grouping node, so the UI can select them with a single path (the folder
	// auto-expands to its children via selectNodeIdChildren). No VTK object.
	BlockedWellboreFolder,
	// Per-grid folder holding the grid GEOMETRY leaf ("_<uuid>" Representation,
	// displayed "SolidColor") as a SIBLING of the real properties. NOT a grouping
	// type (see isGroupingType) so checking it does not cascade. No VTK object.
	PropertiesFolder,
	// Per-grid folder grouping ALL of a grid's SubRepresentations. No VTK object.
	SubRepresentationFolder
};

// Three layouts for the tree built from the data repository:
// - Flat: representations directly under root (legacy, default).
// - ByInterpretation: representations grouped under their Interpretation.
// - ByFeatureAndInterpretation: representations grouped under
//   Feature → Interpretation.
enum class TreeHierarchyMode
{
	Flat = 0,
	ByInterpretation = 1,
	ByFeatureAndInterpretation = 2
};

// True when the node is a "pure grouping" — has no VTK object behind
// it, just organises children. Used by the explicit-selection mode
// (vtkEPCCollector::ExplicitSelection) to decide whether
// selectNodeIdChildren should propagate downward: groupings DO
// propagate (selecting a Wellbore or Feature loads everything in it),
// real objects DO NOT (selecting a grid loads only its geometry, not
// its properties).
inline bool isGroupingType(TreeViewNodeType p_type)
{
	return p_type == TreeViewNodeType::Collection
		|| p_type == TreeViewNodeType::Wellbore
		|| p_type == TreeViewNodeType::Partial
		|| p_type == TreeViewNodeType::Feature
		|| p_type == TreeViewNodeType::Interpretation
		|| p_type == TreeViewNodeType::MultiRealization
		|| p_type == TreeViewNodeType::MultiRealizationTimeSeries
		// A grid container folder groups the grid's reps; checking it propagates
		// the selection to its Full Geometry / SubRep / BlockedWellbore children.
		|| p_type == TreeViewNodeType::GridContainer
		// A BlockedWellbore folder groups a grid's blocked wellbores; checking it
		// propagates the selection to every blocked-wellbore child.
		|| p_type == TreeViewNodeType::BlockedWellboreFolder
		// PropertiesFolder is DELIBERATELY NOT grouping — checking it must NOT
		// cascade to geometry + all props. Still MapperType::Folder (renders nothing).
		|| p_type == TreeViewNodeType::SubRepresentationFolder;
}

// String name of a TreeViewNodeType. Used for the "kind" attribute
// shared with Python — keep in sync with the enum above.
inline const char* treeViewNodeTypeName(TreeViewNodeType p_type)
{
	switch (p_type)
	{
	case TreeViewNodeType::Unknown:                   return "Unknown";
	case TreeViewNodeType::Collection:                return "Collection";
	case TreeViewNodeType::Representation:            return "Representation";
	case TreeViewNodeType::SubRepresentation:         return "SubRepresentation";
	case TreeViewNodeType::Properties:                return "Properties";
	case TreeViewNodeType::Wellbore:                  return "Wellbore";
	case TreeViewNodeType::WellboreTrajectory:        return "WellboreTrajectory";
	case TreeViewNodeType::WellboreFrame:             return "WellboreFrame";
	case TreeViewNodeType::WellboreChannel:           return "WellboreChannel";
	case TreeViewNodeType::WellboreMarkerFrame:       return "WellboreMarkerFrame";
	case TreeViewNodeType::WellboreMarker:            return "WellboreMarker";
	case TreeViewNodeType::WellboreCompletion:        return "WellboreCompletion";
	case TreeViewNodeType::TimeSeries:                return "TimeSeries";
	case TreeViewNodeType::Realization:               return "Realization";
	case TreeViewNodeType::Perforation:               return "Perforation";
	case TreeViewNodeType::Partial:                   return "Partial";
	case TreeViewNodeType::MultiRealization:          return "MultiRealization";
	case TreeViewNodeType::MultiRealizationTimeSeries:return "MultiRealizationTimeSeries";
	case TreeViewNodeType::Feature:                   return "Feature";
	case TreeViewNodeType::Interpretation:            return "Interpretation";
	case TreeViewNodeType::BlockedWellbore:           return "BlockedWellbore";
	case TreeViewNodeType::GridContainer:             return "GridContainer";
		case TreeViewNodeType::BlockedWellboreFolder:     return "BlockedWellboreFolder";
		case TreeViewNodeType::PropertiesFolder:          return "PropertiesFolder";
		case TreeViewNodeType::SubRepresentationFolder:   return "SubRepresentationFolder";
	}
	return "Unknown";
}

#endif // ENUM_H
