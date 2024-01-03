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
ResqmlWellboreMarkerFrameToVtkPartitionedDataSet::ResqmlWellboreMarkerFrameToVtkPartitionedDataSet(const RESQML2_NS::WellboreMarkerFrameRepresentation* p_markerFrame, int p_procNumber, int p_maxProc)
	: CommonAbstractObjectSetToVtkPartitionedDataSetSet(p_markerFrame,
		p_procNumber,
		p_maxProc)
{
}

//----------------------------------------------------------------------------
void ResqmlWellboreMarkerFrameToVtkPartitionedDataSet::addMarker(const RESQML2_NS::WellboreMarkerFrameRepresentation* p_marker, const std::string & p_uuid, bool p_orientation, int p_size)
{
	_mapperSet.push_back(new ResqmlWellboreMarkerToVtkPolyData(p_marker, p_uuid, p_orientation, p_size));
}

void ResqmlWellboreMarkerFrameToVtkPartitionedDataSet::changeOrientationAndSize(const std::string& p_uuid, bool p_orientation, int p_size)
{
	if ((static_cast<ResqmlWellboreMarkerToVtkPolyData*>(_mapperSet[0]))->getMarkerOrientation() != p_orientation ||
		(static_cast<ResqmlWellboreMarkerToVtkPolyData*>(_mapperSet[0]))->getMarkerSize() != p_size)
	{
		for (auto* mapper : _mapperSet)
		{
			if (mapper->getUuid() == p_uuid)
			{
				ResqmlWellboreMarkerToVtkPolyData* marker = static_cast<ResqmlWellboreMarkerToVtkPolyData*>(mapper);
				marker->setMarkerOrientation(p_orientation);
				marker->setMarkerOrientation(p_size);
				marker->loadVtkObject();
			}
		}
	}
}