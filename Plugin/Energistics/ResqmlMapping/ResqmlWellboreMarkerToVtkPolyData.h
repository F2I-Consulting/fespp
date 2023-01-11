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

#include "ResqmlMapping/ResqmlAbstractRepresentationToVtkPartitionedDataSet.h"

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
	ResqmlWellboreMarkerToVtkPolyData(RESQML2_NS::WellboreMarkerFrameRepresentation *marker, std::string uuid, bool orientation, int size, int proc_number = 0, int max_proc = 1);

	/**
	 * load vtkDataSet with resqml data
	 */
	void loadVtkObject() override;

	/**
	 * @brief modify orientation & size + representation reload
	 *
	 * @param orientation
	 * @param size
	 */
	void displayOption(bool orientation, int size);

	/**
	 * get uuid for verify
	 */
	std::string getUuid() { return this->uuid; }
	std::string getTitle() { return this->title; }
	bool getMarkerOrientation() { return this->orientation; }
	int getMarkerSize() { return this->size; }

protected:
	resqml2::WellboreMarkerFrameRepresentation const* getResqmlData() const;

private:
	void createDisk(unsigned int markerIndex);
	void createSphere(unsigned int markerIndex);

	bool orientation;
	int size;

	std::string uuid;
	std::string title;
};
#endif
