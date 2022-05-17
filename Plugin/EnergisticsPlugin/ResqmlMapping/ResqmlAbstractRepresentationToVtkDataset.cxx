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
#include "ResqmlMapping/ResqmlAbstractRepresentationToVtkDataset.h"

// include VTK library
#include <vtkPointData.h>
#include <vtkCellData.h>
#include <vtkDataSet.h>

// include F2i-consulting Energistics Standards API
#include <fesapi/resqml2/AbstractValuesProperty.h>

// include F2i-consulting Energistics Paraview Plugin
#include "ResqmlMapping/ResqmlPropertyToVtkDataArray.h"

//----------------------------------------------------------------------------
ResqmlAbstractRepresentationToVtkDataset::ResqmlAbstractRepresentationToVtkDataset(RESQML2_NS::AbstractRepresentation *abstract_representation, int proc_number, int max_proc) :
	pointCount(0),
	iCellCount(0),
	jCellCount(1),
	kCellCount(1),
	initKIndex(0),
	maxKIndex(0),
	isHyperslabed(false),
	procNumber(proc_number),
	maxProc(max_proc),
	resqmlData(abstract_representation),
	vtkData(nullptr)
{
}

void ResqmlAbstractRepresentationToVtkDataset::addDataArray(const std::string &uuid)
{
	std::vector<RESQML2_NS::AbstractValuesProperty *> valuesPropertySet = this->resqmlData->getValuesPropertySet();
	for (auto &property : valuesPropertySet)
	{
		if (property->getUuid() == uuid)
		{
			ResqmlPropertyToVtkDataArray* fesppProperty = isHyperslabed
				? new ResqmlPropertyToVtkDataArray(property,
					this->iCellCount * this->jCellCount * (this->maxKIndex - this->initKIndex),
					this->pointCount,
					this->iCellCount,
					this->jCellCount,
					this->maxKIndex - this->initKIndex,
					this->initKIndex)
				: new ResqmlPropertyToVtkDataArray(property,
					this->iCellCount * this->jCellCount * this->kCellCount,
					this->pointCount);
			switch (fesppProperty->getSupport())
			{
			case ResqmlPropertyToVtkDataArray::typeSupport::CELLS:
				this->vtkData->GetPartition(0)->GetCellData()->AddArray(fesppProperty->getVtkData());
				break;
			case ResqmlPropertyToVtkDataArray::typeSupport::POINTS:
				this->vtkData->GetPartition(0)->GetPointData()->AddArray(fesppProperty->getVtkData());
				break;
			default:
				throw std::invalid_argument("The property is attached on a non supported topological element i.e. not cell, not point.");
			}
			uuidToVtkDataArray[uuid] = fesppProperty;
		}
	}
	this->vtkData->Modified();
}

void ResqmlAbstractRepresentationToVtkDataset::deleteDataArray(const std::string &uuid)
{
	if (uuidToVtkDataArray[uuid] != nullptr)
	{
		switch (uuidToVtkDataArray[uuid]->getSupport())
		{
		case ResqmlPropertyToVtkDataArray::typeSupport::CELLS:
			vtkData->GetPartition(0)->GetCellData()->RemoveArray(uuidToVtkDataArray[uuid]->getVtkData()->GetName());
			break;
		case ResqmlPropertyToVtkDataArray::typeSupport::POINTS:
			vtkData->GetPartition(0)->GetPointData()->RemoveArray(uuidToVtkDataArray[uuid]->getVtkData()->GetName());
			break;
		default:
			throw std::invalid_argument("The property is attached on a non supported topological element i.e. not cell, not point.");
		}
	}
}
