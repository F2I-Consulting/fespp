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
#ifndef __ResqmlIjkGridSubRepToVtkExplicitStructuredGrid__h__
#define __ResqmlIjkGridSubRepToVtkExplicitStructuredGrid__h__

/** @brief	transform a resqml ijkGrid Subrepresentation to vtkUnstructuredGrid
 */

#include "ResqmlAbstractRepresentationToVtkPartitionedDataSet.h"

 // include VTK
#include <vtkSmartPointer.h>
#include <vtkUnstructuredGrid.h>

namespace RESQML2_NS
{
	class SubRepresentation;
}

class ResqmlIjkGridToVtkExplicitStructuredGrid;

class ResqmlIjkGridSubRepToVtkExplicitStructuredGrid : public ResqmlAbstractRepresentationToVtkPartitionedDataSet
{
public:
	/**
	 * Constructor
	 */
	ResqmlIjkGridSubRepToVtkExplicitStructuredGrid(const RESQML2_NS::SubRepresentation *ijkGridSubRep, ResqmlIjkGridToVtkExplicitStructuredGrid* support, int proc_number = 0, int max_proc = 1);

	/**
	 * load vtkDataSet with resqml data
	 */
	void loadVtkObject() override;

	/**
	*
	*/
	std::string unregisterToMapperSupportingGrid();

protected:
	const RESQML2_NS::SubRepresentation* getResqmlData() const;

	ResqmlIjkGridToVtkExplicitStructuredGrid* mapperIjkGrid;

private:

	vtkSmartPointer<vtkPoints> getMapperVtkPoint();

};
#endif
