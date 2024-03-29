﻿/*-----------------------------------------------------------------------
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
#ifndef __CommonAbstractObjectTovtkPartitionedDataSet__h__
#define __CommonAbstractObjectTovtkPartitionedDataSet__h__

// include system
#include <string>

// include VTK library
#include <vtkSmartPointer.h>
#include <vtkPartitionedDataSet.h>

// include F2i-consulting Energistics Standards API
#include <fesapi/common/AbstractObject.h>

/** @brief	transform a RESQML abstract object to vtkPartitionedDataSet
 */
class CommonAbstractObjectToVtkPartitionedDataSet
{
public:
	/**
	 * Constructor
	 */

	CommonAbstractObjectToVtkPartitionedDataSet(const COMMON_NS::AbstractObject* p_abstractObject, uint32_t p_procNumber = 0, uint32_t p_maxProc = 1);

	/**
	 * Destructor
	 */
	virtual ~CommonAbstractObjectToVtkPartitionedDataSet() = default;

	/**
	 * load VtkPartitionedDataSet with RESQML data
	 */
	virtual void loadVtkObject() = 0;

	/**
	 * @return the vtkPartitionedDataSet of RESQML object
	 */
	vtkSmartPointer<vtkPartitionedDataSet> getOutput() const { return  _vtkData; }

	const std::string& getUuid() const { return  _absUuid; };
	const std::string& getTitle() const { return _absTitle; };
	void setUuid(const std::string& p_newUuid) {  _absUuid = p_newUuid; }
	void setTitle(const std::string& p_newTitle) { _absTitle = p_newTitle; }

protected:
	const COMMON_NS::AbstractObject* getResqmlData() const { return _resqmlData; }

	uint32_t _procNumber;
	uint32_t _maxProc;

	vtkSmartPointer<vtkPartitionedDataSet>  _vtkData;

	std::string _absUuid;
	std::string _absTitle;

private:
	const COMMON_NS::AbstractObject* _resqmlData;
};
#endif
