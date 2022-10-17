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
#include "ResqmlWellboreFrameToVtkPartitionedDataSet.h"

#include <algorithm>

#include <vtkInformation.h>

#include <fesapi/resqml2/WellboreFrameRepresentation.h>
#include <fesapi/resqml2/AbstractValuesProperty.h>

// include F2i-consulting Energistics Paraview Plugin
#include "ResqmlWellboreChannelToVtkPolyData.h"

//----------------------------------------------------------------------------
ResqmlWellboreFrameToVtkPartitionedDataSet::ResqmlWellboreFrameToVtkPartitionedDataSet(resqml2::WellboreFrameRepresentation *frame, int proc_number, int max_proc)
	: ResqmlAbstractRepresentationToVtkPartitionedDataSet(frame,
											   proc_number - 1,
											   max_proc),
	  resqmlData(frame)
{
	this->vtkData = vtkSmartPointer<vtkPartitionedDataSet>::New();
	this->loadVtkObject();
}

//----------------------------------------------------------------------------
void ResqmlWellboreFrameToVtkPartitionedDataSet::loadVtkObject()
{
	for (int idx = 0; idx < this->list_channel.size(); ++idx)
	{
		this->vtkData->SetPartition(idx, this->list_channel[idx]->getOutput()->GetPartitionAsDataObject(0));
		this->vtkData->GetMetaData(idx)->Set(vtkCompositeDataSet::NAME(), this->list_channel[idx]->getTitle().c_str());
	}
}

//----------------------------------------------------------------------------
void ResqmlWellboreFrameToVtkPartitionedDataSet::addChannel(const std::string &uuid, resqml2::AbstractValuesProperty *property)
{
	if (std::find_if(list_channel.begin(), list_channel.end(), [uuid](ResqmlWellboreChannelToVtkPolyData const *channel) { return channel->getUuid() == uuid; }) == list_channel.end())
	{
		auto frame = dynamic_cast<resqml2::WellboreFrameRepresentation *>(this->resqmlData);
		this->list_channel.push_back(new ResqmlWellboreChannelToVtkPolyData(frame, property, uuid));
		this->loadVtkObject();
	}
}

//----------------------------------------------------------------------------
void ResqmlWellboreFrameToVtkPartitionedDataSet::removeChannel(const std::string &uuid)
{
	list_channel.erase(
		std::remove_if(list_channel.begin(), list_channel.end(), [uuid](ResqmlWellboreChannelToVtkPolyData const *channel) { return channel->getUuid() == uuid; }),
		list_channel.end());
	this->loadVtkObject();
}
