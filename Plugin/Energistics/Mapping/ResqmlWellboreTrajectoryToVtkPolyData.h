﻿/*-----------------------------------------------------------------------
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
#ifndef __ResqmlWellboreTrajectoryToVtkPolyData_h
#define __ResqmlWellboreTrajectoryToVtkPolyData_h

#include "ResqmlAbstractRepresentationToVtkPartitionedDataSet.h"

namespace resqml2
{
	class WellboreTrajectoryRepresentation;
}

class ResqmlWellboreTrajectoryToVtkPolyData : public ResqmlAbstractRepresentationToVtkPartitionedDataSet
{
public:
	/**
	 * Constructor
	 */
	explicit ResqmlWellboreTrajectoryToVtkPolyData(const resqml2::WellboreTrajectoryRepresentation *wellbore, uint32_t p_procNumber = 0, uint32_t p_maxProc = 1);

	/**
	 * load vtkDataSet with resqml data
	 */
	void loadVtkObject() override;

protected:
	const resqml2::WellboreTrajectoryRepresentation *getResqmlData() const;

private:
};
#endif
