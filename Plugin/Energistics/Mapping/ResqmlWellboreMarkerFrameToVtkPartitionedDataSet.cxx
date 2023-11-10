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
#include "Mapping/ResqmlWellboreMarkerFrameToVtkPartitionedDataSet.h"

#include <iostream>
#include <string>

// include VTK library
#include <vtkInformation.h>

#include <fesapi/resqml2/WellboreMarker.h>
#include <fesapi/resqml2/WellboreMarkerFrameRepresentation.h>
#include <fesapi/resqml2/AbstractLocal3dCrs.h>

#include "Mapping/ResqmlWellboreMarkerToVtkPolyData.h"

//----------------------------------------------------------------------------
ResqmlWellboreMarkerFrameToVtkPartitionedDataSet::ResqmlWellboreMarkerFrameToVtkPartitionedDataSet(const RESQML2_NS::WellboreMarkerFrameRepresentation *marker_frame, int proc_number, int max_proc)
	: ResqmlAbstractRepresentationToVtkPartitionedDataSet(marker_frame,
											   proc_number,
											   max_proc)
{
	this->vtkData = vtkSmartPointer<vtkPartitionedDataSet>::New();
	this->vtkData->Modified();
}

//----------------------------------------------------------------------------
const RESQML2_NS::WellboreMarkerFrameRepresentation * ResqmlWellboreMarkerFrameToVtkPartitionedDataSet::getResqmlData() const
{
	return static_cast<const RESQML2_NS::WellboreMarkerFrameRepresentation *>(resqmlData);
}

//----------------------------------------------------------------------------
void ResqmlWellboreMarkerFrameToVtkPartitionedDataSet::loadVtkObject()
{
	for (int idx = 0; idx < this->list_marker.size(); ++idx)
	{
		this->vtkData->SetPartition(idx, this->list_marker[idx]->getOutput()->GetPartitionAsDataObject(0));
		this->vtkData->GetMetaData(idx)->Set(vtkCompositeDataSet::NAME(), this->list_marker[idx]->getTitle().c_str());
	}
}

//----------------------------------------------------------------------------
void ResqmlWellboreMarkerFrameToVtkPartitionedDataSet::addMarker(std::string marker_uuid, bool orientation, int size)
{
	bool exist = false;
	for (int idx = 0; idx < this->list_marker.size(); ++idx)
	{
		if (this->list_marker[idx]->getUuid() == marker_uuid)
		{
			auto old_size = this->list_marker[idx]->getMarkerSize();
			auto old_orientation = this->list_marker[idx]->getMarkerOrientation();
			if (this->list_marker[idx]->getMarkerOrientation() != orientation || this->list_marker[idx]->getMarkerSize() != size)
			{
				this->list_marker[idx]->displayOption(orientation, size);
				this->loadVtkObject();
			}
			exist = true;
		}
	}
	if (!exist)
	{
		this->list_marker.push_back(new ResqmlWellboreMarkerToVtkPolyData(getResqmlData(), marker_uuid, orientation, size));
		this->loadVtkObject();
	}
}

//----------------------------------------------------------------------------
void ResqmlWellboreMarkerFrameToVtkPartitionedDataSet::removeMarker(std::string marker_uuid)
{
	this->vtkData = vtkSmartPointer<vtkPartitionedDataSet>::New();
	for (auto it = this->list_marker.begin(); it != this->list_marker.end();)
	{
		ResqmlWellboreMarkerToVtkPolyData *marker = *it;
		if (marker->getUuid() == marker_uuid)
		{
			this->vtkData = vtkSmartPointer<vtkPartitionedDataSet>::New();
			it = this->list_marker.erase(it);
		}
		else
		{
			++it;
		}
	}
	this->loadVtkObject();
}
