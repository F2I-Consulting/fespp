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

#include "ResqmlMapping/ResqmlAbstractRepresentationToVtkDataset.h"
#include "ResqmlMapping/ResqmlWellboreMarkerToVtkPolyData.h"

#include <vector>

namespace RESQML2_NS
{
	class WellboreMarkerFrameRepresentation;
	class AbstractValuesProperty;
}

class ResqmlWellboreMarkerToVtkPolyData;

class ResqmlWellboreMarkerFrameToVtkPartitionedDataSet : public ResqmlAbstractRepresentationToVtkDataset
{
public:
	/**
	 * Constructor
	 */
	ResqmlWellboreMarkerFrameToVtkPartitionedDataSet(RESQML2_NS::WellboreMarkerFrameRepresentation *marker, bool orientation, int size, int proc_number = 1, int max_proc = 1);

	/**
	 * load vtkDataSet with resqml data
	 */
	void loadVtkObject();

	void toggleMarkerOrientation(bool orientation);
	void setMarkerSize(int newSize);

	void addMarker(std::string marker_uuid);
	void removeMarker(std::string marker_uuid);

protected:
	resqml2::WellboreMarkerFrameRepresentation *resqmlData;

private:
	bool orientation;
	int size;

	std::vector<ResqmlWellboreMarkerToVtkPolyData *> list_marker;
};
#endif
