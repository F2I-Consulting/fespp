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
#ifndef __VtkResqml2MultiBlockDataSet_h
#define __VtkResqml2MultiBlockDataSet_h

// include system
#include <unordered_map>
#include <vector>
#include <string>

// include VTK
#include <vtkSmartPointer.h> 
#include <vtkMultiBlockDataSet.h>

// include Resqml2.0 VTK
#include "VtkAbstractObject.h"

/** @brief	The fespp tree representation in VtkMultiBlockDataSet.
 */

class VtkResqml2MultiBlockDataSet : public VtkAbstractObject
{
public:
	/**
	* Constructor
	*/
	VtkResqml2MultiBlockDataSet (const std::string & fileName, const std::string & name="", const std::string & uuid="", const std::string & uuidParent="", int idProc=0, int maxProc=0);

	/**
	* Destructor
	*/
	virtual ~VtkResqml2MultiBlockDataSet() = default;

	/**
	* return the vtkMultiBlockDataSet output
	*/
	vtkSmartPointer<vtkMultiBlockDataSet> getOutput() const;
	
	/**
	* Remove all blocks from the vtkMultiBlockDataSet vtkOutput
	*/
	void detach();
	
protected:

	/**
	* Get the block number of a VTK Data object in the multiblock output
	*
	* @return unsigned int maximum if the vtkDataObj is not present in this multiblock output
	*/
	unsigned int getBlockNumberOf(vtkDataObject * vtkDataObj) const;

	/**
	* Remove a VTK data object from the multiblock output.
	*/
	void removeFromVtkOutput(vtkDataObject * vtkDataObj);

	vtkSmartPointer<vtkMultiBlockDataSet> vtkOutput;

	std::unordered_map<std::string, VtkEpcCommon> uuidIsChildOf;

	std::vector<std::string> attachUuids;
};
#endif
