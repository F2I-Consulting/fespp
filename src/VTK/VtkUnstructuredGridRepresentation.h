﻿/*-----------------------------------------------------------------------
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
#ifndef __VtkUnstructuredGridRepresentation_h
#define __VtkUnstructuredGridRepresentation_h

#include "VtkResqml2UnstructuredGrid.h"
class VtkProperty;

namespace COMMON_NS
{
	class DataObjectRepository;
}

namespace resqml2_0_1
{
	class UnstructuredGridRepresentation;
}


class VtkUnstructuredGridRepresentation : public VtkResqml2UnstructuredGrid
{
public:
	/**
	* Constructor
	*/
	VtkUnstructuredGridRepresentation(const std::string & fileName, const std::string & name, const std::string & uuid, const std::string & uuidParent, COMMON_NS::DataObjectRepository *repoRepresentation, COMMON_NS::DataObjectRepository *repoSubRepresentation, const int & idProc=0, const int & maxProc=0);

	/**
	* Destructor
	*/
	~VtkUnstructuredGridRepresentation();

	/**
	* method : createOutput
	* variable : std::string uuid (Unstructured Grid UUID)
	* create the vtk objects for represent Unstructured grid.
	*/
	void createOutput(const std::string & uuid);

	void addProperty(const std::string & uuidProperty, vtkDataArray* dataProperty);

	long getAttachmentPropertyCount(const std::string & uuid, VtkEpcCommon::FesppAttachmentProperty propertyUnit);

protected:

private:
	vtkSmartPointer<vtkCellArray> createOutputVtkTetra(const resqml2_0_1::UnstructuredGridRepresentation*);

	// PROPERTY
	std::string lastProperty;
};
#endif
