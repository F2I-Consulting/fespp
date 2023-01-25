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
#include "ResqmlMapping/ResqmlAbstractRepresentationToVtkPartitionedDataSet.h"

#include <algorithm>
#include <array>

// include VTK library
#include <vtkCellData.h>
#include <vtkDataSet.h>
#include <vtkPointData.h>

// FESAPI
#include <fesapi/resqml2/AbstractValuesProperty.h>
#include <fesapi/resqml2/UnstructuredGridRepresentation.h>

// include F2i-consulting Energistics Paraview Plugin
#include "ResqmlMapping/ResqmlPropertyToVtkDataArray.h"

//----------------------------------------------------------------------------
ResqmlAbstractRepresentationToVtkPartitionedDataSet::ResqmlAbstractRepresentationToVtkPartitionedDataSet(RESQML2_NS::AbstractRepresentation *abstract_representation, int proc_number, int max_proc):
	subrep_pointer_on_points_count(0),
	procNumber(proc_number),
	maxProc(max_proc),
	resqmlData(abstract_representation),
	vtkData(nullptr),
	uuidToVtkDataArray()
{
}

void ResqmlAbstractRepresentationToVtkPartitionedDataSet::addDataArray(const std::string &uuid, int patch_index)
{
	std::vector<RESQML2_NS::AbstractValuesProperty *> valuesPropertySet = this->getResqmlData()->getValuesPropertySet();
	std::vector<RESQML2_NS::AbstractValuesProperty *>::iterator it = std::find_if(valuesPropertySet.begin(), valuesPropertySet.end(),
																				  [&uuid](RESQML2_NS::AbstractValuesProperty const *property)
																				  { return property->getUuid() == uuid; });
	if (it != std::end(valuesPropertySet))
	{
		auto const* const resqmlProp = *it;
			ResqmlPropertyToVtkDataArray* fesppProperty = isHyperslabed
				? new ResqmlPropertyToVtkDataArray(resqmlProp,
					this->iCellCount * this->jCellCount * (this->maxKIndex - this->initKIndex),
					this->pointCount,
					this->iCellCount,
					this->jCellCount,
					this->maxKIndex - this->initKIndex,
					this->initKIndex,
					patch_index)
				: new ResqmlPropertyToVtkDataArray(resqmlProp,
					this->iCellCount * this->jCellCount * this->kCellCount,
					this->pointCount,
					patch_index);
		switch (resqmlProp->getAttachmentKind())
		{
		case gsoap_eml2_3::resqml22__IndexableElement::cells:
		case gsoap_eml2_3::resqml22__IndexableElement::triangles:
			this->vtkData->GetPartition(0)->GetCellData()->AddArray(fesppProperty->getVtkData());
			break;
		case gsoap_eml2_3::resqml22__IndexableElement::nodes:
			this->vtkData->GetPartition(0)->GetPointData()->AddArray(fesppProperty->getVtkData());
			break;
		default:
			throw std::invalid_argument("The property " + uuid + " is attached on a non supported topological element i.e. not cell, not point.");
		}
		uuidToVtkDataArray[uuid] = fesppProperty;
		this->vtkData->Modified();
	}
	else
	{
		throw std::invalid_argument("The property " + uuid + "cannot be added since it is not contained in the representation " + this->getResqmlData()->getUuid());
	}
}

void ResqmlAbstractRepresentationToVtkPartitionedDataSet::deleteDataArray(const std::string &uuid)
{
	ResqmlPropertyToVtkDataArray* vtkDataArray = uuidToVtkDataArray[uuid];
	if (vtkDataArray != nullptr)
	{
		char* dataArrayName = vtkDataArray->getVtkData()->GetName();
		if (vtkData->GetPartition(0)->GetCellData()->HasArray(dataArrayName)) {
			vtkData->GetPartition(0)->GetCellData()->RemoveArray(dataArrayName);
		}
		if (vtkData->GetPartition(0)->GetPointData()->HasArray(dataArrayName)) {
			vtkData->GetPartition(0)->GetPointData()->RemoveArray(dataArrayName);
		}

		// Cleaning
		delete vtkDataArray;
		uuidToVtkDataArray.erase(uuid);
	}
	else
	{
		throw std::invalid_argument("The property " + uuid + "cannot be deleted from representation " + this->getResqmlData()->getUuid() + " since it has never been added");
	}
}

void ResqmlAbstractRepresentationToVtkPartitionedDataSet::unloadVtkObject()
{
	this->vtkData = vtkSmartPointer<vtkPartitionedDataSet>::New();
}

void ResqmlAbstractRepresentationToVtkPartitionedDataSet::registerSubRep() 
{ 
	++subrep_pointer_on_points_count; 
}

void ResqmlAbstractRepresentationToVtkPartitionedDataSet::unregisterSubRep() 
{
	--subrep_pointer_on_points_count; 
}

unsigned int ResqmlAbstractRepresentationToVtkPartitionedDataSet::subRepLinkedCount()
{ 
	return subrep_pointer_on_points_count; 
}

std::string ResqmlAbstractRepresentationToVtkPartitionedDataSet::getUuid() const
{
	return this->getResqmlData()->getUuid();
}
