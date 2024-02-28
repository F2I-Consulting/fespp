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
	ResqmlWellboreMarkerToVtkPolyData(const RESQML2_NS::WellboreMarkerFrameRepresentation *p_marker, std::string p_uuid, bool p_orientation, uint32_t p_size, uint32_t p_procNumber = 0, uint32_t p_maxProc = 1);

	/**
	 * load vtkDataSet with resqml data
	 */
	void loadVtkObject() override;

	bool getMarkerOrientation() { return _orientation; }
	uint32_t getMarkerSize() { return _size; }

	void setMarkerOrientation(bool p_orientation) { _orientation = p_orientation; }
	void setMarkerSize(uint32_t p_size) { _size = p_size; }

protected:
	const resqml2::WellboreMarkerFrameRepresentation *getResqmlData() const;

private:
	void createDisk(uint32_t markerIndex);
	void createSphere(uint32_t markerIndex);

	bool _orientation;
	uint32_t _size;
};
#endif
