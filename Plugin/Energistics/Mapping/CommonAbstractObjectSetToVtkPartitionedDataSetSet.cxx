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
#include "Mapping/CommonAbstractObjectSetToVtkPartitionedDataSetSet.h"

#include <algorithm>

//----------------------------------------------------------------------------
CommonAbstractObjectSetToVtkPartitionedDataSetSet::CommonAbstractObjectSetToVtkPartitionedDataSetSet(const COMMON_NS::AbstractObject *p_abstractObject, uint32_t p_procNumber, uint32_t p_maxProc)
	: _procNumber(p_procNumber),
	  _maxProc(p_maxProc),
	  _resqmlData(p_abstractObject),
	  _mapperSet()
{
	_uuid = p_abstractObject->getUuid();
	_title = p_abstractObject->getTitle();
}

CommonAbstractObjectSetToVtkPartitionedDataSetSet::~CommonAbstractObjectSetToVtkPartitionedDataSetSet()
{
	for (auto &w_item : _mapperSet)
	{
		delete w_item;
	}
	_mapperSet.clear();
}

void CommonAbstractObjectSetToVtkPartitionedDataSetSet::loadVtkObject()
{
	for (uint32_t w_i = _procNumber; w_i < _mapperSet.size(); w_i += _maxProc)
	{
		_mapperSet[w_i]->loadVtkObject();
	}
}

//----------------------------------------------------------------------------
void CommonAbstractObjectSetToVtkPartitionedDataSetSet::removeCommonAbstractObjectToVtkPartitionedDataSet(const std::string &p_id)
{
	auto it = std::find_if(_mapperSet.begin(), _mapperSet.end(),
		[&p_id](const CommonAbstractObjectToVtkPartitionedDataSet* m) { return m->getUuid() == p_id; });
	if (it != _mapperSet.end())
	{
		delete *it;
		_mapperSet.erase(it);
	}
}

//----------------------------------------------------------------------------
bool CommonAbstractObjectSetToVtkPartitionedDataSetSet::existUuid(const std::string &p_id)
{
	return std::any_of(_mapperSet.begin(), _mapperSet.end(),
		[&p_id](const CommonAbstractObjectToVtkPartitionedDataSet* m) { return m->getUuid() == p_id; });
}
