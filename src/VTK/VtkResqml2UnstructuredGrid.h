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
#ifndef __VtkResqml2UnstructuredGrid_h
#define __VtkResqml2UnstructuredGrid_h

// include VTK
#include <vtkSmartPointer.h> 
#include <vtkUnstructuredGrid.h>

// include Resqml2.0 VTK
#include "VtkAbstractRepresentation.h"

class VtkResqml2UnstructuredGrid : public VtkAbstractRepresentation
{
public:
	/**
	* Constructor
	*/
	VtkResqml2UnstructuredGrid(const std::string & fileName, const std::string & name, const std::string & uuid, const std::string & uuidParent, COMMON_NS::DataObjectRepository const * repoRepresentation, COMMON_NS::DataObjectRepository const * repoSubRepresentation, int idProc=0, int maxProc=0);

	/**
	* Destructor
	*/
	virtual ~VtkResqml2UnstructuredGrid() {}

	/**
	* method : getOutput
	* variable : --
	* return the vtkUnstructuredGrid
	*/
	vtkSmartPointer<vtkUnstructuredGrid> getOutput() const;
	
	/**
	* method : remove
	* variable : std::string uuid
	* delete the vtkUnstructuredGrid
	*/
	void remove(const std::string & uuid);

	/**
	* method : createOutput (Virtual)
	* variable : std::string uuid
	*/
	virtual void createOutput(const std::string & uuid) = 0;

protected:
	vtkSmartPointer<vtkUnstructuredGrid> vtkOutput;
};
#endif
