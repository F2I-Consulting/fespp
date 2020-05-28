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
#include "VtkAbstractRepresentation.h"

#include <vtkDataArray.h>

#include "VtkProperty.h"

//----------------------------------------------------------------------------
VtkAbstractRepresentation::VtkAbstractRepresentation(const std::string & fileName, const std::string & name, const std::string & uuid, const std::string & uuidParent, COMMON_NS::DataObjectRepository const * pckEPCRep, COMMON_NS::DataObjectRepository const * pckEPCSubRep, int idProc, int maxProc) :
	VtkAbstractObject(fileName, name, uuid, uuidParent, idProc, maxProc), epcPackageRepresentation(pckEPCRep), epcPackageSubRepresentation(pckEPCSubRep),
	subRepresentation(pckEPCSubRep != nullptr)
{}

VtkAbstractRepresentation::~VtkAbstractRepresentation()
{
	if (epcPackageRepresentation != nullptr) {
		epcPackageRepresentation = nullptr;
	}

	if (epcPackageSubRepresentation != nullptr) {
		epcPackageSubRepresentation = nullptr;
	}

	for (auto i : uuidToVtkProperty) {
		delete i.second;
	}
	uuidToVtkProperty.clear();

	points = nullptr;
}
//----------------------------------------------------------------------------
void VtkAbstractRepresentation::createTreeVtk(const std::string & uuid, const std::string & parent, const std::string & name, VtkEpcCommon::Resqml2Type resqmlType)
{
	if (resqmlType == VtkEpcCommon::Resqml2Type::PROPERTY &&
		uuidToVtkProperty.find(uuid) == uuidToVtkProperty.end()) {
		uuidToVtkProperty[uuid] = new VtkProperty(getFileName(), name, uuid, parent, subRepresentation ? epcPackageSubRepresentation : epcPackageRepresentation);
	}
}

//----------------------------------------------------------------------------
void VtkAbstractRepresentation::visualize(const std::string & uuid)
{
	createOutput(uuid);
}

vtkSmartPointer<vtkPoints> VtkAbstractRepresentation::createVtkPoints(ULONG64 pointCount, const double * allXyzPoints, const RESQML2_NS::AbstractLocal3dCrs * localCRS)
{
	points = vtkSmartPointer<vtkPoints>::New();

	const double zIndice = localCRS->isDepthOriented() ? -1 : 1;

	for (ULONG64 nodeIndex = 0; nodeIndex < pointCount * 3; nodeIndex += 3) {
		points->InsertNextPoint(allXyzPoints[nodeIndex], allXyzPoints[nodeIndex + 1], allXyzPoints[nodeIndex + 2] * zIndice);
	}
	return points;
}

vtkSmartPointer<vtkPoints> VtkAbstractRepresentation::getVtkPoints()
{
	return points;
}
