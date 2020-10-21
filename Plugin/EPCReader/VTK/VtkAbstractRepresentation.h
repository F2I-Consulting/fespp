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

namespace COMMON_NS
{
	class DataObjectRepository;
}

/** @brief	An abstract of data object representation.
 */
class VtkAbstractRepresentation : public VtkAbstractObject
{
public:
	/**
	* Constructor
	*/
	VtkAbstractRepresentation(const std::string & fileName, const std::string & name, const std::string & uuid, const std::string & uuidParent, COMMON_NS::DataObjectRepository const * epcPackageRepresentation, COMMON_NS::DataObjectRepository const * epcPackageSubRepresentation, int idProc=0, int maxProc=0);

	/**
	* Destructor
	*/
	virtual ~VtkAbstractRepresentation();

	/**
	* Add to trees structure a property.
	*
	* @param uuid		uuid property.
	* @param parent		uuid property parent.
	* @param name		name (i.e. title) property
	* @param resqmlType		verification of property type
	*
	*/
	void createTreeVtk(const std::string & uuid, const std::string & parent, const std::string & name, VtkEpcCommon::Resqml2Type resqmlType);

protected:

	/**
	* Create a vtkPoints from a C double array representing XYZ triplets. Invert Z coordinates in case the XYZ triplets are depth oriented.
	* This method takes ownership of the C double array meaning that it will delete it (using delete[]). Don't delete the C array somewhere else in the code.
	*
	* @param pointCount		The count of points in allXyzPoints. allXyzPoints must be 3*pointCount size.
	* @param allXyzPoints	The XYZ triplets of the VTK points to be created. They must lie in the localCRS.
	*						Their ownership will be transfered to VTK in this method. Do not delete them.
	* @param localCRS		The CRS where the XYZ triplets are given.
	*
	*/
	vtkSmartPointer<vtkPoints> createVtkPoints(ULONG64 pointCount, double * allXyzPoints, const RESQML2_NS::AbstractLocal3dCrs * localCRS);

	std::unordered_map<std::string, class VtkProperty *> uuidToVtkProperty;

	// EPC DOCUMENT
	COMMON_NS::DataObjectRepository const * epcPackageRepresentation;
	COMMON_NS::DataObjectRepository const * epcPackageSubRepresentation;

	bool subRepresentation;
};
#endif
