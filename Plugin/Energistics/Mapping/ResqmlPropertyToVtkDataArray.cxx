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
#include "Mapping/ResqmlPropertyToVtkDataArray.h"
#include "vtkMath.h"

// FESAPI
#include <fesapi/resqml2/CategoricalProperty.h>
#include <fesapi/resqml2/ContinuousProperty.h>
#include <fesapi/resqml2/DiscreteProperty.h>

// VTK
#include <vtkDoubleArray.h>
#include <vtkFloatArray.h>
#include <vtkLongArray.h>
#include <vtkUnsignedLongArray.h>
#include <vtkIntArray.h>
#include <vtkUnsignedIntArray.h>
#include <vtkShortArray.h>
#include <vtkUnsignedShortArray.h>
#include <vtkCharArray.h>
#include <vtkUnsignedCharArray.h>

//----------------------------------------------------------------------------
ResqmlPropertyToVtkDataArray::ResqmlPropertyToVtkDataArray(RESQML2_NS::AbstractValuesProperty const *valuesProperty,
														   uint64_t cellCount,
														   uint64_t pointCount,
														   uint32_t iCellCount,
														   uint32_t jCellCount,
														   uint32_t kCellCount,
														   uint32_t initKIndex,
															int patch_index)
{
	int nbElement = 0;

	const gsoap_eml2_3::eml23__IndexableElement element = valuesProperty->getAttachmentKind();
	if (element == gsoap_eml2_3::eml23__IndexableElement::cells ||
		element == gsoap_eml2_3::eml23__IndexableElement::triangles)
	{
		nbElement = cellCount;
	}
	else if (element == gsoap_eml2_3::eml23__IndexableElement::nodes)
	{
		nbElement = pointCount;
	}
	else
	{
		throw std::invalid_argument("Property indexable element must be points or cells.");
	}

	const unsigned int elementCountPerValue = valuesProperty->getElementCountPerValue();
	if (elementCountPerValue != 1)
	{
		vtkOutputWindowDisplayErrorText("does not support vectorial property yet\n");
	}

	const uint64_t numValuesInEachDimension = cellCount;						 // cellCount/kCellCount; //3834;//iCellCount*jCellCount*kCellCount;
	const uint64_t offsetInEachDimension = iCellCount * jCellCount * initKIndex; // initKIndex;//iCellCount*jCellCount*initKIndex;

	std::string typeProperty = valuesProperty->getXmlTag();
	if (typeProperty == RESQML2_NS::ContinuousProperty::XML_TAG)
	{
		vtkSmartPointer<vtkFloatArray> cellDataFloat = vtkSmartPointer<vtkFloatArray>::New();
		float *valuesFloatSet = new float[nbElement]; // deleted by VTK cellData vtkSmartPointer
		if (valuesProperty->getDimensionsCountOfPatch(patch_index) == 3)
		{
			valuesProperty->getFloatValuesOf3dPatch(patch_index, valuesFloatSet, iCellCount, jCellCount, kCellCount, 0, 0, initKIndex);
		}
		else if (valuesProperty->getDimensionsCountOfPatch(patch_index) == 1)
		{
			valuesProperty->getFloatValuesOfPatch(patch_index, valuesFloatSet, &numValuesInEachDimension, &offsetInEachDimension, 1);
		}
		else
		{
			vtkOutputWindowDisplayErrorText("error in : propertyValue->getDimensionsCountOfPatch (values different of 1 or 3)\n");
		}
		cellDataFloat->SetName(valuesProperty->getTitle().c_str());
		cellDataFloat->SetArray(valuesFloatSet, nbElement, 0, vtkAbstractArray::VTK_DATA_ARRAY_DELETE);
		this->dataArray = cellDataFloat;
	}
	else if (typeProperty == RESQML2_NS::DiscreteProperty::XML_TAG)
	{
		vtkSmartPointer<vtkIntArray> cellDataInt = vtkSmartPointer<vtkIntArray>::New();
		int *valuesIntSet = new int[nbElement]; // deleted by VTK cellData vtkSmartPointer
		if (valuesProperty->getDimensionsCountOfPatch(patch_index) == 3)
		{
			valuesProperty->getIntValuesOf3dPatch(patch_index, valuesIntSet, iCellCount, jCellCount, kCellCount, 0, 0, initKIndex);
		}
		else if (valuesProperty->getDimensionsCountOfPatch(0) == 1)
		{
			valuesProperty->getIntValuesOfPatch(patch_index, valuesIntSet, &numValuesInEachDimension, &offsetInEachDimension, 1);
		}
		else
		{
			vtkOutputWindowDisplayErrorText("error in : propertyValue->getDimensionsCountOfPatch (values different of 1 or 3)\n");
		}
		cellDataInt->SetName(valuesProperty->getTitle().c_str());
		cellDataInt->SetArray(valuesIntSet, nbElement, 0, vtkAbstractArray::VTK_DATA_ARRAY_DELETE);
		this->dataArray = cellDataInt;
	}
	else if (typeProperty == RESQML2_NS::CategoricalProperty::XML_TAG)
	{
		vtkSmartPointer<vtkIntArray> cellDataInt = vtkSmartPointer<vtkIntArray>::New();
		int *valuesIntSet = new int[nbElement]; // deleted by VTK cellData vtkSmartPointer
		if (valuesProperty->getDimensionsCountOfPatch(patch_index) == 3)
		{
			valuesProperty->getIntValuesOf3dPatch(patch_index, valuesIntSet, iCellCount, jCellCount, kCellCount, 0, 0, initKIndex);
		}
		else if (valuesProperty->getDimensionsCountOfPatch(patch_index) == 1)
		{
			valuesProperty->getIntValuesOfPatch(patch_index, valuesIntSet, &numValuesInEachDimension, &offsetInEachDimension, 1);
		}
		else
		{
			vtkOutputWindowDisplayErrorText("error in : propertyValue->getDimensionsCountOfPatch (values different of 1 or 3)\n");
		}
		cellDataInt->SetName(valuesProperty->getTitle().c_str());
		cellDataInt->SetArray(valuesIntSet, nbElement, 0, vtkAbstractArray::VTK_DATA_ARRAY_DELETE);
		this->dataArray = cellDataInt;
	}
	else
	{
		vtkOutputWindowDisplayErrorText("property not supported...  (hdfDatatypeEnum)\n");
	}
}

// -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
ResqmlPropertyToVtkDataArray::ResqmlPropertyToVtkDataArray(resqml2::AbstractValuesProperty const *valuesProperty,
														   long cellCount,
														   long pointCount,
															int patch_index)
{
	int nbElement = 0;

	const gsoap_eml2_3::eml23__IndexableElement element = valuesProperty->getAttachmentKind();
	if (element == gsoap_eml2_3::eml23__IndexableElement::cells ||
		element == gsoap_eml2_3::eml23__IndexableElement::triangles)
	{
		nbElement = cellCount;
	}
	else if (element == gsoap_eml2_3::eml23__IndexableElement::nodes)
	{
		nbElement = pointCount;
	}
	else
	{
		throw std::invalid_argument("Property indexable element must be points or cells.");
	}

	const unsigned int elementCountPerValue = valuesProperty->getElementCountPerValue();
	const std::string name = valuesProperty->getTitle();
	if (valuesProperty->getXmlTag() == resqml2::ContinuousProperty::XML_TAG)
	{
		// defensive code
		const unsigned int totalHDFElementcount = nbElement * elementCountPerValue;
		if (totalHDFElementcount != valuesProperty->getValuesCountOfPatch(patch_index))
		{
			throw std::invalid_argument("Property values count of hdfDataset \"" + std::to_string(valuesProperty->getValuesCountOfPatch(patch_index))
				+ "\" does not match the indexable element count in the supporting representation\"" + std::to_string(totalHDFElementcount) + "\"");
		}

		double *valuesDoubleSet = new double[totalHDFElementcount]; // deleted by VTK data vtkSmartPointer
		valuesProperty->getDoubleValuesOfPatch(patch_index, valuesDoubleSet);

		vtkSmartPointer<vtkDoubleArray> cellDataDouble = vtkSmartPointer<vtkDoubleArray>::New();
		cellDataDouble->SetNumberOfComponents(elementCountPerValue);
		cellDataDouble->SetName(name.c_str());
		cellDataDouble->SetArray(valuesDoubleSet, nbElement * elementCountPerValue, 0, vtkAbstractArray::VTK_DATA_ARRAY_DELETE);
		this->dataArray = cellDataDouble;
	}
	else if (valuesProperty->getXmlTag() == resqml2::DiscreteProperty::XML_TAG ||
			 (valuesProperty->getXmlTag() == resqml2::CategoricalProperty::XML_TAG &&
			  static_cast<resqml2::CategoricalProperty const *>(valuesProperty)->getStringLookup() != nullptr))
	{
		int *values = new int[nbElement * elementCountPerValue]; // deleted by VTK data vtkSmartPointer
		valuesProperty->getInt32ValuesOfPatch(patch_index, values);

		vtkSmartPointer<vtkIntArray> cellDataInt = vtkSmartPointer<vtkIntArray>::New();
		cellDataInt->SetNumberOfComponents(elementCountPerValue);
		cellDataInt->SetName(name.c_str());
		cellDataInt->SetArray(values, nbElement * elementCountPerValue, 0, vtkAbstractArray::VTK_DATA_ARRAY_DELETE);
		this->dataArray = cellDataInt;
	}
	else
	{
		throw std::invalid_argument("does not support property which are not discrete or categorical or continuous yet");
	}
}
