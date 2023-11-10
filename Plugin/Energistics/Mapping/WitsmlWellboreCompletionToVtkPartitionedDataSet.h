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

#include "Mapping/CommonAbstractObjectToVtkPartitionedDataSet.h"
#include "Mapping/WitsmlWellboreCompletionPerforationToVtkPolyData.h"

namespace WITSML2_1_NS
{
	class WellboreCompletion;
}

namespace resqml
{
	class WellboreTrajectoryRepresentation;
}

class WitsmlWellboreCompletionToVtkPartitionedDataSet : public CommonAbstractObjectToVtkPartitionedDataSet
{
public:
	/**
	 * Constructor
	 */
	WitsmlWellboreCompletionToVtkPartitionedDataSet(const WITSML2_1_NS::WellboreCompletion *completion, int proc_number = 0, int max_proc = 1);

	/**
	 * load vtkDataSet with resqml data
	 */
	void loadVtkObject() override;

	void addPerforation(const std::string& connectionuid, const std::string& name);
	std::vector<WitsmlWellboreCompletionPerforationToVtkPolyData*> getPerforations();

protected:
	const WITSML2_1_NS::WellboreCompletion const* getResqmlData() const;
	const resqml2::WellboreTrajectoryRepresentation const* getWellboreTrajectory() const;

	const resqml2::WellboreTrajectoryRepresentation * wellboreTrajectory;

private:
	std::vector<WitsmlWellboreCompletionPerforationToVtkPolyData*> perforations;

};
#endif
