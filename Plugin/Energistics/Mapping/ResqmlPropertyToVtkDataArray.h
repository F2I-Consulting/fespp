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
#ifndef __ResqmlPropertyToVtkDataArray_h
#define __ResqmlPropertyToVtkDataArray_h

// include VTK library
#include <vtkSmartPointer.h>
#include <vtkDataArray.h>

#include <fesapi/nsDefinitions.h>

namespace RESQML2_NS
{
	class AbstractValuesProperty;
}

/** @brief	the data table of a property
 */

class ResqmlPropertyToVtkDataArray
{
public:
	/**
	 * Constructor for multi-processor
	 */
	ResqmlPropertyToVtkDataArray(const RESQML2_NS::AbstractValuesProperty *resqmlProperty,
								 uint64_t cellCount,
								 uint64_t pointCount,
								 uint32_t iCellCount,
								 uint32_t jCellCount,
								 uint32_t kCellCount,
								 uint32_t initKIndex,
								 int patch_index);

	/**
	 * Constructor
	 */
	ResqmlPropertyToVtkDataArray(RESQML2_NS::AbstractValuesProperty const *resqmlProperty,
								 long cellCount,
								 long pointCount,
								 int patch_index);

	~ResqmlPropertyToVtkDataArray() = default;

	vtkSmartPointer<vtkDataArray> getVtkData() { return dataArray; }

private:
	vtkSmartPointer<vtkDataArray> dataArray;
};
#endif
