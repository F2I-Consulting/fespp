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
#ifndef __WitsmlWellboreCompletionToVtkPartitionedDataSet_H_
#define __WitsmlWellboreCompletionToVtkPartitionedDataSet_H_

#include <vector>

#include "Mapping/CommonAbstractObjectSetToVtkPartitionedDataSetSet.h"

namespace WITSML2_1_NS
{
	class WellboreCompletion;
}

namespace resqml
{
	class WellboreTrajectoryRepresentation;
}

class WitsmlWellboreCompletionToVtkPartitionedDataSet : public CommonAbstractObjectSetToVtkPartitionedDataSetSet
{
public:
	/**
	 * Constructor
	 */
	WitsmlWellboreCompletionToVtkPartitionedDataSet(const WITSML2_1_NS::WellboreCompletion *p_completion, int p_procNumber = 0, int p_maxProc = 1);

	void addPerforation(const std::string &p_connectionuid, const std::string &p_name, const double p_skin);

protected:
	const resqml2::WellboreTrajectoryRepresentation *_wellboreTrajectory;

};
#endif
