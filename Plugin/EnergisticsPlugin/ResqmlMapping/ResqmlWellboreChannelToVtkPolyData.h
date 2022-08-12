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
#ifndef __ResqmlWellboreChannelToVtkPolyData_H_
#define __ResqmlWellboreChannelToVtkPolyData__H_

#include "ResqmlMapping/ResqmlAbstractRepresentationToVtkDataset.h"

namespace RESQML2_NS
{
	class WellboreFrameRepresentation;
	class AbstractValuesProperty;
}

class ResqmlWellboreChannelToVtkPolyData : public ResqmlAbstractRepresentationToVtkDataset
{
public:
	/**
	 * Constructor
	 */
	ResqmlWellboreChannelToVtkPolyData(RESQML2_NS::WellboreFrameRepresentation *frame, RESQML2_NS::AbstractValuesProperty *property, const std::string &uuid, int proc_number = 1, int max_proc = 1);

	/**
	 * load vtkDataSet with resqml data
	 */
	void loadVtkObject();

	std::string getUuid() const { return this->uuid; }
	std::string getTitle() const { return this->title; }

protected:
	RESQML2_NS::WellboreFrameRepresentation *resqmlData;

private:
	RESQML2_NS::AbstractValuesProperty *abstractProperty;

	std::string uuid;
	std::string title;
};
#endif
