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
	ResqmlAbstractRepresentationToVtkPartitionedDataSet(const RESQML2_NS::AbstractRepresentation *p_abstractRepresentation, int p_procNumber = 0, int p_maxProc = 1);

	/**
	 * Destructor
	 */
	virtual ~ResqmlAbstractRepresentationToVtkPartitionedDataSet() = default;

	/**
	 * load VtkPartitionedDataSet with resqml data
	 */
	virtual void loadVtkObject() = 0;

	/**
	 * add a resqml property to VtkPartitionedDataSet
	 */
	void addDataArray(const std::string &p_uuid, int p_patchIndex = 0);

	/**
	 * remove a resqml property to VtkPartitionedDataSet
	 */
	void deleteDataArray(const std::string &p_uuid);

	/**
	 *
	 */
	void registerSubRep();
	void unregisterSubRep();
	unsigned int subRepLinkedCount();

protected:
	const RESQML2_NS::AbstractRepresentation *getResqmlData() const { return _resqmlData; }

	unsigned int _subrepPointerOnPointsCount;

	uint64_t _pointCount = 0;
	uint32_t _iCellCount = 0; // = cellcount if not ijkGrid
	uint32_t _jCellCount = 1;
	uint32_t _kCellCount = 1;
	uint32_t _initKIndex = 0;
	uint32_t _maxKIndex = 0;

	bool _isHyperslabed = false;

	const RESQML2_NS::AbstractRepresentation *_resqmlData;

	std::unordered_map<std::string, class ResqmlPropertyToVtkDataArray *> _uuidToVtkDataArray;
};
#endif
