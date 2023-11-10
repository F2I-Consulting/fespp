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
	CommonAbstractObjectToVtkPartitionedDataSet(const COMMON_NS::AbstractObject *abstract_object, int proc_number = 0, int max_proc = 1);

	/**
	 * Destructor
	 */
	virtual ~CommonAbstractObjectToVtkPartitionedDataSet() = default;

	/**
	 * load VtkPartitionedDataSet with resqml data
	 */
	virtual void loadVtkObject() = 0;
	/**
	 * unload VtkPartitionedDataSet
	 */
	void unloadVtkObject();

	/**
	 * return the vtkPartitionedDataSet of resqml object
	 */
	vtkSmartPointer<vtkPartitionedDataSet> getOutput() const { return vtkData; }

	/**
	*
	*/
	std::string getUuid() const { return abs_uuid; };
	std::string getTitle() const { return abs_title; };
	void setUuid(std::string new_uuid) { abs_uuid = new_uuid; }
	void setTitle(std::string new_title) { abs_title = new_title; }
	

protected:

	const COMMON_NS::AbstractObject * getResqmlData() const { return resqmlData; }

	int procNumber;
	int maxProc;

	const COMMON_NS::AbstractObject * resqmlData;

	vtkSmartPointer<vtkPartitionedDataSet> vtkData;

	std::string abs_uuid;
	std::string abs_title;

};
#endif
