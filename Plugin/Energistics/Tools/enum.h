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
	// Synthetic types created by searchRealization() to fold per-realization
	// (and per-realization+per-timestep) property nodes into a single tree node.
	// Appended on purpose: existing serialized 'type' int values are unchanged.
	// Python uses the string name (via the 'kind' attribute), not the int — so
	// adding values here is also Python-safe.
	MultiRealization,
	MultiRealizationTimeSeries,
	// Grouping nodes used by the alternate tree hierarchy modes
	// (ByInterpretation, ByFeatureAndInterpretation). They have no VTK
	// object behind them — selecting one propagates to all descendants.
	Feature,
	Interpretation
};

// Three layouts for the tree built from the data repository:
// - Flat: Representation directly under root (or under Wellbore for wells),
//   Properties under Rep. Default; matches legacy behavior.
// - ByInterpretation: Reps are grouped under their Interpretation parent.
// - ByFeatureAndInterpretation: Reps are grouped under Feature → Interpretation.
enum class TreeHierarchyMode
{
	Flat = 0,
	ByInterpretation = 1,
	ByFeatureAndInterpretation = 2
};

// True when the node is a "pure grouping" — i.e. has no VTK object behind it,
// just organizes children. Used by the explicit-selection mode to decide
// whether `selectNodeIdChildren` should propagate downward: groupings DO
// propagate (selecting a Wellbore loads everything in it), real objects DO
// NOT (selecting a grid loads only its geometry, not its properties).
inline bool isGroupingType(TreeViewNodeType p_type)
{
	return p_type == TreeViewNodeType::Collection
		|| p_type == TreeViewNodeType::Wellbore
		|| p_type == TreeViewNodeType::Partial
		|| p_type == TreeViewNodeType::Feature
		|| p_type == TreeViewNodeType::Interpretation;
}

// String name of a TreeViewNodeType. Used for the 'kind' DataAssembly attribute
// shared with Python. Keep this list in sync with the enum above.
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
	}
	return "Unknown";
}

#endif // ENUM_H
