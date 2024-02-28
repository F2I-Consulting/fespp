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
#include "Mapping/ResqmlWellboreFrameToVtkPartitionedDataSet.h"

#include <algorithm>

#include <vtkInformation.h>

#include <fesapi/resqml2/WellboreFrameRepresentation.h>
#include <fesapi/resqml2/AbstractValuesProperty.h>

// include F2i-consulting Energistics Paraview Plugin
#include "Mapping/ResqmlWellboreChannelToVtkPolyData.h"

//----------------------------------------------------------------------------
ResqmlWellboreFrameToVtkPartitionedDataSet::ResqmlWellboreFrameToVtkPartitionedDataSet(const resqml2::WellboreFrameRepresentation *p_frame, uint32_t p_procNumber, uint32_t p_maxProc)
	: CommonAbstractObjectSetToVtkPartitionedDataSetSet(p_frame,
														  p_procNumber,
														  p_maxProc)
{
}

//----------------------------------------------------------------------------
void ResqmlWellboreFrameToVtkPartitionedDataSet::addChannel(const std::string& p_uuid, resqml2::AbstractValuesProperty* p_property)
{
	const resqml2::WellboreFrameRepresentation* w_wellFrame = dynamic_cast<const resqml2::WellboreFrameRepresentation*>(_resqmlData);
	_mapperSet.push_back(new ResqmlWellboreChannelToVtkPolyData(w_wellFrame, p_property, p_uuid));
}