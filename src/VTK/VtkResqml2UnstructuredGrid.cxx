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
#include "VtkResqml2UnstructuredGrid.h"

#include <stdexcept>

#include <vtkCellData.h>
#include <vtkPointData.h>
// FESPP
#include "VtkProperty.h"

//----------------------------------------------------------------------------
VtkResqml2UnstructuredGrid::VtkResqml2UnstructuredGrid(const std::string & fileName, const std::string & name, const std::string & uuid, const std::string & uuidParent,
	COMMON_NS::DataObjectRepository const * repoRepresentation, COMMON_NS::DataObjectRepository const * repoSubRepresentation, int idProc, int maxProc) :
	VtkAbstractRepresentation(fileName, name, uuid, uuidParent, repoRepresentation, repoSubRepresentation, idProc, maxProc)
{
}

//----------------------------------------------------------------------------
vtkSmartPointer<vtkUnstructuredGrid> VtkResqml2UnstructuredGrid::getOutput() const
{
	return vtkOutput;
}

//----------------------------------------------------------------------------
void VtkResqml2UnstructuredGrid::remove(const std::string & uuid)
{
	if (uuid == getUuid()) {
		vtkOutput = nullptr;
	}
	else if(uuidToVtkProperty[uuid] != nullptr) {
		switch (uuidToVtkProperty[uuid]->getSupport()) {
		case VtkProperty::typeSupport::CELLS: vtkOutput->GetCellData()->RemoveArray(uuidToVtkProperty[uuid]->getName().c_str()); break;
		case VtkProperty::typeSupport::POINTS: vtkOutput->GetPointData()->RemoveArray(uuidToVtkProperty[uuid]->getName().c_str()); break;
		default: throw std::invalid_argument("The property is attached on a non supported topological element i.e. not cell, not point.");
		}
	}
}
