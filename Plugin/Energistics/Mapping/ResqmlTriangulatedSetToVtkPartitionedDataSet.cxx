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
#include "ResqmlTriangulatedSetToVtkPartitionedDataSet.h"

#include <sstream>

// include VTK library
#include <vtkPartitionedDataSet.h>
#include <vtkInformation.h>

// include F2i-consulting Energistics Standards API
#include <fesapi/resqml2/TriangulatedSetRepresentation.h>
#include <fesapi/resqml2/AbstractLocal3dCrs.h>
#include <fesapi/resqml2/AbstractValuesProperty.h>
#include <fesapi/resqml2/SubRepresentation.h>

// include F2i-consulting Energistics Paraview Plugin
#include "Mapping/ResqmlPropertyToVtkDataArray.h"
#include "Mapping/ResqmlTriangulatedToVtkPolyData.h"

//----------------------------------------------------------------------------
ResqmlTriangulatedSetToVtkPartitionedDataSet::ResqmlTriangulatedSetToVtkPartitionedDataSet(RESQML2_NS::TriangulatedSetRepresentation *triangulated, int proc_number, int max_proc)
	: ResqmlAbstractRepresentationToVtkPartitionedDataSet(triangulated,
											   proc_number,
											   max_proc)
{
	this->vtkData = vtkSmartPointer<vtkPartitionedDataSet>::New();
	this->pointCount = resqmlData->getXyzPointCountOfAllPatches();
	this->vtkData->Modified();
}

//----------------------------------------------------------------------------
RESQML2_NS::TriangulatedSetRepresentation * ResqmlTriangulatedSetToVtkPartitionedDataSet::getResqmlData() const
{
	return static_cast<RESQML2_NS::TriangulatedSetRepresentation *>(resqmlData);
}

//----------------------------------------------------------------------------
void ResqmlTriangulatedSetToVtkPartitionedDataSet::loadVtkObject()
{
	vtkSmartPointer<vtkPartitionedDataSet> partition = vtkSmartPointer<vtkPartitionedDataSet>::New();

	RESQML2_NS::TriangulatedSetRepresentation * triangulatedSet = getResqmlData();
	const unsigned int patchCount = triangulatedSet->getPatchCount();

	for (unsigned int patchIndex = 0; patchIndex < patchCount; ++patchIndex)
	{
		auto rep = new ResqmlTriangulatedToVtkPolyData(triangulatedSet, patchIndex, this->procNumber, this->maxProc);
		partition->SetPartition(patchIndex, rep->getOutput()->GetPartitionAsDataObject(0));
		partition->GetMetaData(patchIndex)->Set(vtkCompositeDataSet::NAME(), ("Patch " + std::to_string(patchIndex)).c_str());
		patchIndex_to_ResqmlTriangulated[patchIndex] = rep;
	}

	this->vtkData = partition;
	this->vtkData->Modified();
}

void ResqmlTriangulatedSetToVtkPartitionedDataSet::addDataArray(const std::string& uuid)
{
	vtkSmartPointer<vtkPartitionedDataSet> partition = vtkSmartPointer<vtkPartitionedDataSet>::New();

	for (auto& map : patchIndex_to_ResqmlTriangulated)
	{
		map.second->addDataArray(uuid, map.first);
		partition->SetPartition(map.first, map.second->getOutput()->GetPartitionAsDataObject(0));
		partition->GetMetaData(map.first)->Set(vtkCompositeDataSet::NAME(), ("Patch " + std::to_string(map.first)).c_str());
	}

	this->vtkData = partition;
	this->vtkData->Modified();

}
