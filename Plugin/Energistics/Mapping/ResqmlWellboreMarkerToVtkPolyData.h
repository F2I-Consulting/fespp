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
#ifndef _ResqmlWellboreMarkerToVtkPolyData_H_
#define _ResqmlWellboreMarkerToVtkPolyData_H_

#include "Mapping/ResqmlAbstractRepresentationToVtkPartitionedDataSet.h"

#include <set>

namespace RESQML2_NS
{
	class WellboreMarkerFrameRepresentation;
}

class ResqmlWellboreMarkerToVtkPolyData : public ResqmlAbstractRepresentationToVtkPartitionedDataSet
{
public:
	/**
	 * Constructor
	 */
	ResqmlWellboreMarkerToVtkPolyData(const RESQML2_NS::WellboreMarkerFrameRepresentation *marker, std::string uuid, bool orientation, int size, int p_procNumber = 0, int p_maxProc = 1);

	/**
	 * load vtkDataSet with resqml data
	 */
	void loadVtkObject() override;

	bool getMarkerOrientation() { return orientation; }
	int getMarkerSize() { return size; }

	void setMarkerOrientation(bool p_orientation) { orientation = p_orientation; }
	void setMarkerSize(int p_size) { size = p_size; }

protected:
	const resqml2::WellboreMarkerFrameRepresentation *getResqmlData() const;

private:
	void createDisk(unsigned int markerIndex);
	void createSphere(unsigned int markerIndex);

	bool orientation;
	int size;
};
#endif
