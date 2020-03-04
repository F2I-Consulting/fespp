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
#ifndef __VtkAbstractRepresentation_h
#define __VtkAbstractRepresentation_h

// include system
#include <unordered_map>
#include <string>

// include VTK
#include <vtkSmartPointer.h> 
#include <vtkPoints.h>

// include Resqml2.0 VTK
#include <fesapi/resqml2/AbstractLocal3dCrs.h>

#include "VtkAbstractObject.h"

class VtkProperty;
namespace COMMON_NS
{
	class DataObjectRepository;
}

class VtkAbstractRepresentation : public VtkAbstractObject
{
public:
	/**
	* Constructor
	*/
	VtkAbstractRepresentation(const std::string & fileName, const std::string & name, const std::string & uuid, const std::string & uuidParent, COMMON_NS::DataObjectRepository *epcPackageRepresentation, COMMON_NS::DataObjectRepository *epcPackageSubRepresentation, int idProc=0, int maxProc=0);

	/**
	* Destructor
	*/
	virtual ~VtkAbstractRepresentation();

	/**
	* load & display representation uuid's
	*/
	void visualize(const std::string & uuid);

	/**
	* create property
	*/
	void createTreeVtk(const std::string & uuid, const std::string & parent, const std::string & name, VtkEpcCommon::Resqml2Type resqmlType);
	
	/**
	* createOutput the h5 data with vtk structure
	*/
	virtual void createOutput(const std::string & uuid) = 0;
	
	vtkSmartPointer<vtkPoints> createVtkPoints(ULONG64 pointCount, const double * allXyzPoints, const RESQML2_NS::AbstractLocal3dCrs * localCRS);

	virtual void addProperty(const std::string & uuidProperty, vtkDataArray* dataProperty) = 0;

	vtkSmartPointer<vtkPoints> getVtkPoints();

	bool vtkPointsIsCreated();

	void setSubRepresentation() { subRepresentation = true; }

protected:
	std::unordered_map<std::string, VtkProperty *> uuidToVtkProperty;

	// EPC DOCUMENT
	COMMON_NS::DataObjectRepository *epcPackageRepresentation;
	COMMON_NS::DataObjectRepository *epcPackageSubRepresentation;

	vtkSmartPointer<vtkPoints> points;
	bool subRepresentation;
};
#endif
