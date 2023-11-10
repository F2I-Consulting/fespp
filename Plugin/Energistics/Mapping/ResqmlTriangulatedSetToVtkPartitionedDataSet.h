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
#ifndef __ResqmlTriangulatedSetToVtkPartitionedDataSet_h
#define __ResqmlTriangulatedSetToVtkPartitionedDataSet_h

/** @brief	transform a RESQML TriangulatedSetRepresentation representation to vtkPolyData
 */

// include system
#include <map>
#include <string>

#include "ResqmlAbstractRepresentationToVtkPartitionedDataSet.h"

class ResqmlTriangulatedToVtkPolyData;

namespace RESQML2_NS
{
	class TriangulatedSetRepresentation;
}

class ResqmlTriangulatedSetToVtkPartitionedDataSet : public ResqmlAbstractRepresentationToVtkPartitionedDataSet
{
public:
	/**
	* Constructor
	*/
	explicit ResqmlTriangulatedSetToVtkPartitionedDataSet(const RESQML2_NS::TriangulatedSetRepresentation *triangulated, int proc_number = 0, int max_proc = 1);
	
	/**
	* load vtkDataSet with RESQML data
	*/
	void loadVtkObject() override;

	/**
	 * add a RESQML property to vtkDataSet
	 */
	void addDataArray(const std::string& uuid);

protected:
	const RESQML2_NS::TriangulatedSetRepresentation * getResqmlData() const;

	std::map<int, ResqmlTriangulatedToVtkPolyData*> patchIndex_to_ResqmlTriangulated;									// index of VtkDataAssembly to RESQML UUID
};
#endif
