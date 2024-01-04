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
#ifndef __ResqmlWellboreFrameToVtkPartitionedDataSet_H_
#define __ResqmlWellboreFrameToVtkPartitionedDataSet_H_

#include "Mapping/CommonAbstractObjectSetToVtkPartitionedDataSetSet.h"

namespace RESQML2_NS
{
	class WellboreFrameRepresentation;
	class AbstractValuesProperty;
}

class ResqmlWellboreFrameToVtkPartitionedDataSet : public CommonAbstractObjectSetToVtkPartitionedDataSetSet
{
public:
	/**
	 * Constructor
	 */
	ResqmlWellboreFrameToVtkPartitionedDataSet(const RESQML2_NS::WellboreFrameRepresentation *frame, int p_procNumber = 0, int p_maxProc = 1);

	void addChannel(const std::string& uuid, RESQML2_NS::AbstractValuesProperty* property);
};
#endif
