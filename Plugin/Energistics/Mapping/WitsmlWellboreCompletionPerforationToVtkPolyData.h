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
#define __WitsmlWellboreCompletionPerforationToVtkPolyData_H_

#include <fesapi/resqml2/WellboreTrajectoryRepresentation.h>

#include "Mapping/CommonAbstractObjectToVtkPartitionedDataSet.h"
#include "../Tools/enum.h"

#include <vtkPolyData.h>

class WitsmlWellboreCompletionPerforationToVtkPolyData : public CommonAbstractObjectToVtkPartitionedDataSet
{
public:
	/**
	 * Constructor
	 */
	WitsmlWellboreCompletionPerforationToVtkPolyData(const resqml2::WellboreTrajectoryRepresentation *wellboreTrajectory, const WITSML2_1_NS::WellboreCompletion *WellboreCompletion, const std::string &connectionuid, const std::string &title, const double skin, const int p_procNumber = 0, int p_maxProc = 1);

	/**
	 * load vtkDataSet with resqml data
	 */
	void loadVtkObject();

	std::string getTitle() const { return title; }
	std::string getConnectionuid() const { return connectionuid; }

private:
	void addSkin();

protected:
	const WITSML2_1_NS::WellboreCompletion *wellboreCompletion;
	const resqml2::WellboreTrajectoryRepresentation *wellboreTrajectory;
	std::string title;
	std::string connectionuid;
	double skin;
	uint64_t index;
};
#endif
