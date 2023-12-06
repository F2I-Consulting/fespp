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
#ifndef __CommonAbstractObjectSetToVtkPartitionedDataSetSet__h__
#define __CommonAbstractObjectSetToVtkPartitionedDataSetSet__h__

// include system
#include <string>

// include VTK library
#include <vtkSmartPointer.h>
#include <vtkPartitionedDataSet.h>

// include F2i-consulting Energistics Standards API
#include <fesapi/common/AbstractObject.h>

// include F2i-consulting Energistics ParaView Plugin
#include "Mapping/CommonAbstractObjectToVtkPartitionedDataSet.h"

/** @brief	transform a RESQML abstract object to vtkPartitionedDataSet
 */
class CommonAbstractObjectSetToVtkPartitionedDataSetSet
{
public:
	// Constructor
	CommonAbstractObjectSetToVtkPartitionedDataSetSet(const COMMON_NS::AbstractObject * p_abstractObject, int p_procNumber = 0, int p_maxProc = 1);

	// destructor
	~CommonAbstractObjectSetToVtkPartitionedDataSetSet();

	/**
	*
	*/
	std::string getUuid() const { return _uuid; };
	std::string getTitle() const { return _title; };

	void loadVtkObject();
	void removeCommonAbstractObjectToVtkPartitionedDataSet(const std::string& p_id);
	std::vector<CommonAbstractObjectToVtkPartitionedDataSet*> getMapperSet() { return _mapperSet; }

	bool existUuid(const std::string& p_id);

protected:
	const COMMON_NS::AbstractObject * _resqmlData;
	
	// for Multithreading
	int _procNumber;
	int _maxProc;

	std::string _uuid;
	std::string _title;

	std::vector<CommonAbstractObjectToVtkPartitionedDataSet*> _mapperSet;

};
#endif
