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
// VTK
#include <vtkPointData.h>

//FESAPI
#include <fesapi/common/EpcDocument.h>

// FESPP
#include "VtkResqml2PolyData.h"

//----------------------------------------------------------------------------
VtkResqml2PolyData::VtkResqml2PolyData(const std::string & fileName, const std::string & name, const std::string & uuid, const std::string & uuidParent, COMMON_NS::DataObjectRepository *pckRep, COMMON_NS::DataObjectRepository *pckSubRep, const int & idProc, const int & maxProc) :
VtkAbstractRepresentation(fileName, name, uuid, uuidParent, pckRep, pckSubRep, idProc, maxProc)
{
}

//----------------------------------------------------------------------------
VtkResqml2PolyData::~VtkResqml2PolyData()
{
	vtkOutput = nullptr;
}

//----------------------------------------------------------------------------
vtkSmartPointer<vtkPolyData> VtkResqml2PolyData::getOutput() const
{
	return vtkOutput;
}

//----------------------------------------------------------------------------
void VtkResqml2PolyData::remove(const std::string & uuid)
{
	if (uuid == getUuid()) {
		vtkOutput = nullptr;
	}
	else if(uuidToVtkProperty[uuid] != nullptr) {
		vtkOutput->GetPointData()->RemoveArray(0);
	}
}




