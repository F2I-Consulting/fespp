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
#ifndef __ResqmlAbstractRepresentationTovtkPartitionedDataSet__h__
#define __ResqmlAbstractRepresentationTovtkPartitionedDataSet__h__

// include system
#include <string>

// include VTK library
#include <vtkSmartPointer.h>
#include <vtkPartitionedDataSet.h>

// include F2i-consulting Energistics Standards API
#include <fesapi/resqml2/AbstractRepresentation.h>

// include F2i-consulting Energistics Standards Paraview Plugin
#include "CommonAbstractObjectToVtkPartitionedDataSet.h"

/** @brief	transform a RESQML abstract representation to vtkPartitionedDataSet
 */
class ResqmlAbstractRepresentationToVtkPartitionedDataSet : public CommonAbstractObjectToVtkPartitionedDataSet
{
public:
	/**
	 * Constructor
	 */
	ResqmlAbstractRepresentationToVtkPartitionedDataSet(const RESQML2_NS::AbstractRepresentation *p_abstractRepresentation, uint32_t p_procNumber = 0, uint32_t p_maxProc = 1);

	/**
	 * Destructor
	 */
	virtual ~ResqmlAbstractRepresentationToVtkPartitionedDataSet() = default;

	/**
	 * load VtkPartitionedDataSet with resqml data
	 */
	virtual void loadVtkObject() = 0;

	/**
	 * add a resqml property to VtkPartitionedDataSet.
	 * When p_autoActivate is false, the array is added without forcing it as
	 * the active scalar coloring on the representation. Set this to false
	 * for swap operations (multi-realization or time-step changes) where the
	 * user has already chosen which array drives the coloring.
	 *
	 * `p_arrayNameSuffix` (default empty) is appended to the VTK array name
	 * derived from the property's title. The per-property multi-realization
	 * load uses this to materialize concurrent realizations of the same
	 * property under distinct names (e.g. "K_real_3", "K_real_7"). When
	 * empty, the array name matches the property's sanitized title exactly,
	 * preserving legacy single-realization behaviour.
	 *
	 * The map keyed by UUID is unchanged — each realization of a property
	 * has its own RESQML UUID, so concurrent realizations naturally take
	 * separate map slots. The suffix only affects the VTK array name shown
	 * downstream in ColorBy / scalar selectors.
	 */
	char * addDataArray(const std::string &p_uuid, uint32_t p_patchIndex = 0, bool p_autoActivate = true,
		const std::string& p_arrayNameSuffix = std::string());

	/**
	 * remove a resqml property to VtkPartitionedDataSet
	 */
	void deleteDataArray(const std::string &p_uuid);

	/**
	 * True iff the given RESQML property UUID is currently materialised
	 * on this representation (i.e. its data array has been added via
	 * addDataArray and not yet removed). Used by the per-property
	 * multi-realization load to compute the diff between "what the
	 * user wants loaded" and "what's actually in the partition".
	 */
	bool hasDataArray(const std::string& p_uuid) const
	{
		return _uuidToVtkDataArray.count(p_uuid) > 0;
	}

	/**
	 * Return the VTK array name currently bound to `p_uuid`, or empty
	 * string when the uuid isn't loaded. Lets callers detect a
	 * suffix-mismatch case (the same uuid loaded under a different name,
	 * typically after a legacy ↔ per-property mode transition for
	 * multi-realization properties).
	 */
	std::string getDataArrayName(const std::string& p_uuid) const;

	/**
	 *
	 */
	void registerSubRep();
	void unregisterSubRep();
	uint32_t subRepLinkedCount();

	const char* getActivePropertyName() { return activeArrayName; }
	int getActivePropertyType() { return activeType; }

	int ActiveProperty(const char* arrayName, vtkDataObject::AttributeTypes type);

private:

protected:
	const RESQML2_NS::AbstractRepresentation *getResqmlData() const { return _resqmlData; }

	uint32_t _subrepPointerOnPointsCount;

	uint64_t _pointCount = 0;
	uint32_t _iCellCount = 0; // = cellcount if not ijkGrid
	uint32_t _jCellCount = 1;
	uint32_t _kCellCount = 1;
	uint32_t _initKIndex = 0;
	uint32_t _maxKIndex = 0;

	bool _isHyperslabed = false;

	const RESQML2_NS::AbstractRepresentation *_resqmlData;

	std::unordered_map<std::string, class ResqmlPropertyToVtkDataArray *> _uuidToVtkDataArray;

	char * activeArrayName = nullptr;
	int activeType =0;
};
#endif
