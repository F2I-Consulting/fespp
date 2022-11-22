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
#include <vtkPyramid.h>
#include <vtkUnstructuredGrid.h>
#include <vtkWedge.h>

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
	std::vector<RESQML2_NS::AbstractValuesProperty *> valuesPropertySet = this->resqmlData->getValuesPropertySet();
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
		throw std::invalid_argument("The property " + uuid + "cannot be added since it is not contained in the representation " + this->resqmlData->getUuid());
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
		throw std::invalid_argument("The property " + uuid + "cannot be deleted from representation " + resqmlData->getUuid() + " since it has never been added");
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
	return this->resqmlData->getUuid();
}

//----------------------------------------------------------------------------
void ResqmlAbstractRepresentationToVtkPartitionedDataSet::cellVtkWedgeOrPyramid(vtkSmartPointer<vtkUnstructuredGrid> vtk_unstructuredGrid,
	const RESQML2_NS::UnstructuredGridRepresentation *unstructuredGridRep,
	ULONG64 const *cumulativeFaceCountPerCell, unsigned char const *cellFaceNormalOutwardlyDirected,
	ULONG64 cellIndex)
{
	// The global index of the first face of the polyhedron in the cellFaceNormalOutwardlyDirected array
	const size_t globalFirstFaceIndex = unstructuredGridRep->isFaceCountOfCellsConstant() || cellIndex == 0
		? cellIndex * 5
		: cumulativeFaceCountPerCell[cellIndex - 1];

	std::vector<unsigned int> localFaceIndexWith4Nodes;
	for (unsigned int localFaceIndex = 0; localFaceIndex < 5; ++localFaceIndex)
	{
		if (unstructuredGridRep->getNodeCountOfFaceOfCell(cellIndex, localFaceIndex) == 4)
		{
			localFaceIndexWith4Nodes.push_back(localFaceIndex);
		}
	}
	if (localFaceIndexWith4Nodes.size() == 3)
	{ // VTK_WEDGE
		std::array<uint64_t, 6> nodes;
		// Set the triangle base of the wedge
		unsigned int triangleIndex = 0;
		for (; triangleIndex < 5; ++triangleIndex)
		{
			const unsigned int localNodeCount = unstructuredGridRep->getNodeCountOfFaceOfCell(cellIndex, triangleIndex);
			if (localNodeCount == 3)
			{
				uint64_t const *nodeIndices = unstructuredGridRep->getNodeIndicesOfFaceOfCell(cellIndex, triangleIndex);
				if (cellFaceNormalOutwardlyDirected[globalFirstFaceIndex + triangleIndex] == 0)
				{
					for (size_t i = 0; i < 3; ++i)
					{
						nodes[i] = nodeIndices[2 - i];
					}
				}
				else
				{
					// The RESQML orientation of face 0 honors the VTK orientation of face 0 i.e. the face 0 normal defined using a right hand rule is outwardly directed.
					for (size_t i = 0; i < 3; ++i)
					{
						nodes[i] = nodeIndices[i];
					}
				}
				++triangleIndex;
				break;
			}
		}
		// Find the index of the vertex at the opposite triangle regarding the triangle base
		for (unsigned int localFaceIndex = 0; localFaceIndex < 5; ++localFaceIndex)
		{
			const unsigned int localNodeCount = unstructuredGridRep->getNodeCountOfFaceOfCell(cellIndex, localFaceIndex);
			if (localNodeCount == 4)
			{
				uint64_t const *nodeIndices = unstructuredGridRep->getNodeIndicesOfFaceOfCell(cellIndex, localFaceIndex);
				if (nodeIndices[0] == nodes[0]) {
					nodes[3] = nodeIndices[1] == nodes[1] || nodeIndices[1] == nodes[2]
						? nodeIndices[3] : nodeIndices[1];
					break;
				}
				else if (nodeIndices[1] == nodes[0]) {
					nodes[3] = nodeIndices[2] == nodes[1] || nodeIndices[2] == nodes[2]
						? nodeIndices[0] : nodeIndices[2];
					break;
				}
				else if (nodeIndices[2] == nodes[0]) {
					nodes[3] = nodeIndices[3] == nodes[1] || nodeIndices[3] == nodes[2]
						? nodeIndices[1] : nodeIndices[3];
					break;
				}
				else if (nodeIndices[3] == nodes[0]) {
					nodes[3] = nodeIndices[0] == nodes[1] || nodeIndices[0] == nodes[2]
						? nodeIndices[2] : nodeIndices[0];
					break;
				}
			}
		}
		// Set the other triangle of the wedge
		for (; triangleIndex < 5; ++triangleIndex)
		{
			const unsigned int localNodeCount = unstructuredGridRep->getNodeCountOfFaceOfCell(cellIndex, triangleIndex);
			if (localNodeCount == 3)
			{
				uint64_t const *nodeIndices = unstructuredGridRep->getNodeIndicesOfFaceOfCell(cellIndex, triangleIndex);
				if (nodeIndices[0] == nodes[3])
				{
					if (cellFaceNormalOutwardlyDirected[globalFirstFaceIndex + triangleIndex] == 0)
					{
						nodes[4] = nodeIndices[1];
						nodes[5] = nodeIndices[2];
					}
					else
					{
						nodes[4] = nodeIndices[2];
						nodes[5] = nodeIndices[1];
					}
				}
				else if (nodeIndices[1] == nodes[3])
				{
					if (cellFaceNormalOutwardlyDirected[globalFirstFaceIndex + triangleIndex] == 0)
					{
						nodes[4] = nodeIndices[2];
						nodes[5] = nodeIndices[0];
					}
					else
					{
						nodes[4] = nodeIndices[0];
						nodes[5] = nodeIndices[2];
					}
				}
				else if (nodeIndices[2] == nodes[3])
				{
					if (cellFaceNormalOutwardlyDirected[globalFirstFaceIndex + triangleIndex] == 0)
					{
						nodes[4] = nodeIndices[0];
						nodes[5] = nodeIndices[1];
					}
					else
					{
						nodes[4] = nodeIndices[1];
						nodes[5] = nodeIndices[0];
					}
				}
				break;
			}
		}

		vtkSmartPointer<vtkWedge> wedge = vtkSmartPointer<vtkWedge>::New();
		for (int nodesIndex = 0; nodesIndex < 6; ++nodesIndex)
		{
			wedge->GetPointIds()->SetId(nodesIndex, nodes[nodesIndex]);
		}
		vtk_unstructuredGrid->InsertNextCell(wedge->GetCellType(), wedge->GetPointIds());
	}
	else if (localFaceIndexWith4Nodes.size() == 1)
	{ // VTK_PYRAMID
		ULONG64 nodes[5];

		ULONG64 const *nodeIndices = unstructuredGridRep->getNodeIndicesOfFaceOfCell(cellIndex, localFaceIndexWith4Nodes[0]);
		size_t cellFaceIndex = (unstructuredGridRep->isFaceCountOfCellsConstant() || cellIndex == 0
			? cellIndex * 5
			: cumulativeFaceCountPerCell[cellIndex - 1]) +
			localFaceIndexWith4Nodes[0];
		if (cellFaceNormalOutwardlyDirected[cellFaceIndex] == 0)
		{ // The RESQML orientation of the face honors the VTK orientation of face 0 i.e. the face 0 normal defined using a right hand rule is inwardly directed.
			nodes[0] = nodeIndices[0];
			nodes[1] = nodeIndices[1];
			nodes[2] = nodeIndices[2];
			nodes[3] = nodeIndices[3];
		}
		else
		{ // The RESQML orientation of the face does not honor the VTK orientation of face 0
			nodes[0] = nodeIndices[3];
			nodes[1] = nodeIndices[2];
			nodes[2] = nodeIndices[1];
			nodes[3] = nodeIndices[0];
		}

		// Face with 3 points
		nodeIndices = unstructuredGridRep->getNodeIndicesOfFaceOfCell(cellIndex, localFaceIndexWith4Nodes[0] == 0 ? 1 : 0);

		for (size_t index = 0; index < 3; ++index)
		{
			if (std::find(nodes, nodes + 4, nodeIndices[index]) == nodes + 4)
			{
				nodes[4] = nodeIndices[index];
				break;
			}
		}

		vtkSmartPointer<vtkPyramid> pyramid = vtkSmartPointer<vtkPyramid>::New();
		for (int nodesIndex = 0; nodesIndex < 5; ++nodesIndex)
		{
			pyramid->GetPointIds()->SetId(nodesIndex, nodes[nodesIndex]);
		}
		vtk_unstructuredGrid->InsertNextCell(pyramid->GetCellType(), pyramid->GetPointIds());
	}
	else
	{
		throw std::invalid_argument("The cell index " + std::to_string(cellIndex) + " is malformed : 5 faces but not a pyramid, not a wedge.");
	}
}
