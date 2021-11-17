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
#ifndef __ResqmlAbstractRepresentationToVtkDataset__h__
#define __ResqmlAbstractRepresentationToVtkDataset__h__

/** @brief	transform a resqml abstract representation to vtkDataSet
 */

// include system
#include <string>

// include VTK library
#include <vtkSmartPointer.h>
#include <vtkDataSet.h>

// include F2i-consulting Energistics Standards API
#include <fesapi/resqml2/AbstractRepresentation.h>

class ResqmlAbstractRepresentationToVtkDataset
{
public:
	/**
	 * Constructor
	 */
	ResqmlAbstractRepresentationToVtkDataset(RESQML2_NS::AbstractRepresentation *abstract_representation, int proc_number = 1, int max_proc = 1);

	/**
	 * Destructor
	 */

	/**
	 * load vtkDataSet with resqml data
	 */
	void loadVtkObject();

	/**
	 * unload vtkDataSet with resqml data
	 */
	void unloadVtkObject() { this->vtkData = nullptr; }

	/**
	 * add a resqml property to vtkDataSet
	 */
	void addDataArray(const std::string &uuid);

	/**
	 * remove a resqml property to vtkDataSet
	 */
	void deleteDataArray(const std::string &uuid);

	/**
	 * return the vtkDataSet of resqml representation
	 */
	vtkSmartPointer<vtkDataSet> getOutput() const;

protected:
	virtual ~ResqmlAbstractRepresentationToVtkDataset() = default;

	RESQML2_NS::AbstractRepresentation *resqmlData;
	ULONG64 pointCount;
	uint32_t iCellCount;
	uint32_t jCellCount;
	uint32_t kCellCount;
	uint32_t initKIndex;
	uint32_t maxKIndex;

	bool isHyperslabed;

	int procNumber;
	int maxProc;

	vtkSmartPointer<vtkDataSet> vtkData;
	std::unordered_map<std::string, class ResqmlPropertyToVtkDataArray *> uuidToVtkDataArray;

};
#endif
