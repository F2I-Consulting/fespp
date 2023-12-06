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
#include "Mapping/CommonAbstractObjectToVtkPartitionedDataSet.h"

//----------------------------------------------------------------------------
CommonAbstractObjectToVtkPartitionedDataSet::CommonAbstractObjectToVtkPartitionedDataSet(const COMMON_NS::AbstractObject *abstract_object, int proc_number, int max_proc):
	procNumber(proc_number),
	maxProc(max_proc),
	resqmlData(abstract_object),
	vtkData(nullptr)
{
	this->abs_uuid = this->getResqmlData()->getUuid();
	this->abs_title = this->getResqmlData()->getTitle();
}
