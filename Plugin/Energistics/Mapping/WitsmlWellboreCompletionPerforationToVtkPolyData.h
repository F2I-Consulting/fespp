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
#ifndef __WitsmlWellboreCompletionPerforationToVtkPolyData_H_
#define __WitsmlWellboreCompletionPerforationToVtkPolyData__H_

#include "Mapping/CommonAbstractObjectToVtkPartitionedDataSet.h"

namespace WITSML2_1_NS
{
	class WellboreCompletion;
}

namespace resqml
{
	class WellboreTrajectoryRepresentation;
}

class WitsmlWellboreCompletionPerforationToVtkPolyData : public CommonAbstractObjectToVtkPartitionedDataSet
{
public:
	/**
	 * Constructor
	 */
	WitsmlWellboreCompletionPerforationToVtkPolyData(const WITSML2_1_NS::WellboreCompletion *completion, const resqml2::WellboreTrajectoryRepresentation *trajectory, const std::string &connection_uid, int proc_number = 0, int max_proc = 1);

	/**
	 * load vtkDataSet with resqml data
	 */
	void loadVtkObject() override;


protected:
	const WITSML2_1_NS::WellboreCompletion * WellboreCompletion;

	const resqml2::WellboreTrajectoryRepresentation * wellboreTrajectory;

	const std::string &connectionUid;

};
#endif
