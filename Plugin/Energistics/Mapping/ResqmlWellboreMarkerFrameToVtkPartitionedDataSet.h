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
#ifndef _ResqmlWellboreMarkerFrameToVtkPartitionedDataSet_H_
#define _ResqmlWellboreMarkerFrameToVtkPartitionedDataSet_H_

#include "Mapping/CommonAbstractObjectSetToVtkPartitionedDataSetSet.h"

#include <vector>

namespace RESQML2_NS
{
	class WellboreMarkerFrameRepresentation;
	class AbstractValuesProperty;
}

class ResqmlWellboreMarkerToVtkPolyData;

class ResqmlWellboreMarkerFrameToVtkPartitionedDataSet : public CommonAbstractObjectSetToVtkPartitionedDataSetSet
{
public:
	/**
	 * Constructor
	 */
	explicit ResqmlWellboreMarkerFrameToVtkPartitionedDataSet(const RESQML2_NS::WellboreMarkerFrameRepresentation *marker, int p_procNumber = 0, int p_maxProc = 1);

	void addMarker(const RESQML2_NS::WellboreMarkerFrameRepresentation* marker, const std::string & p_uuid, bool orientation, int size);

	void changeOrientationAndSize(const std::string& p_uuid, bool orientation, int size);

private:
	bool orientation;
	int size;
};
#endif
