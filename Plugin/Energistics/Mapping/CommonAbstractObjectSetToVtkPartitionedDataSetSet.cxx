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

//----------------------------------------------------------------------------
CommonAbstractObjectSetToVtkPartitionedDataSetSet::CommonAbstractObjectSetToVtkPartitionedDataSetSet(const COMMON_NS::AbstractObject *p_abstractObject, int p_procNumber, int p_maxProc)
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
	for (int w_i = _procNumber; w_i < _mapperSet.size(); w_i += _maxProc)
	{
		_mapperSet[w_i]->loadVtkObject();
	}
}

//----------------------------------------------------------------------------
void CommonAbstractObjectSetToVtkPartitionedDataSetSet::removeCommonAbstractObjectToVtkPartitionedDataSet(const std::string &p_id)
{
	for (auto w_it = _mapperSet.begin(); w_it != _mapperSet.end();)
	{
		CommonAbstractObjectToVtkPartitionedDataSet *w_mapper = *w_it;
		if (w_mapper->getUuid() == p_id)
		{
			delete w_mapper;
			w_it = _mapperSet.erase(w_it);

			return;
		}
		else
		{
			++w_it;
		}
	}
}

//----------------------------------------------------------------------------
bool CommonAbstractObjectSetToVtkPartitionedDataSetSet::existUuid(const std::string &p_id)
{
	for (auto w_it = _mapperSet.begin(); w_it != _mapperSet.end();)
	{
		CommonAbstractObjectToVtkPartitionedDataSet *w_mapper = *w_it;
		if (w_mapper->getUuid() == p_id)
		{
			return true;
		}
		else
		{
			++w_it;
		}
	}
	return false;
}
