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
#include "ResqmlMapping/ResqmlPropertyToVtkDataArray.h"
#include "vtkMath.h"

// FESAPI
//#include <fesapi/resqml2/AbstractValuesProperty.h>
#include <fesapi/resqml2/CategoricalProperty.h>
#include <fesapi/resqml2/ContinuousProperty.h>
#include <fesapi/resqml2/DiscreteProperty.h>
#include <fesapi/resqml2/AbstractIjkGridRepresentation.h>
#include <fesapi/resqml2/PolylineSetRepresentation.h>
#include <fesapi/resqml2/TriangulatedSetRepresentation.h>
#include <fesapi/resqml2/Grid2dRepresentation.h>
#include <fesapi/resqml2/UnstructuredGridRepresentation.h>
#include <fesapi/resqml2/WellboreTrajectoryRepresentation.h>

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
ResqmlPropertyToVtkDataArray::ResqmlPropertyToVtkDataArray(resqml2::AbstractValuesProperty *valuesProperty,
														   uint32_t cellCount,
														   uint64_t pointCount,
														   uint32_t iCellCount,
														   uint32_t jCellCount,
														   uint32_t kCellCount,
														   uint32_t initKIndex)
{
	int nbElement = 0;

	setTypeSupport(valuesProperty);

	if (this->support == typeSupport::POINTS)
	{
		nbElement = pointCount;
	}
	else if (this->support == typeSupport::CELLS)
	{
		nbElement = cellCount;
	}

	// verify nbElement != 0
	if (nbElement == 0)
	{
		vtkOutputWindowDisplayErrorText("property not supported...  (resqml2__IndexableElements: not cells or triangles or nodes)\n");
	}

	unsigned int elementCountPerValue = valuesProperty->getElementCountPerValue();
	if (elementCountPerValue > 1)
	{
		cerr << "does not support vectorial property yet" << endl;
	}

	std::string typeProperty = valuesProperty->getXmlTag();

	unsigned long long numValuesInEachDimension = cellCount;						 // cellCount/kCellCount; //3834;//iCellCount*jCellCount*kCellCount;
	unsigned long long offsetInEachDimension = iCellCount * jCellCount * initKIndex; // initKIndex;//iCellCount*jCellCount*initKIndex;

	const std::string name = valuesProperty->getTitle();
	if (typeProperty == "ContinuousProperty")
	{
		vtkSmartPointer<vtkFloatArray> cellDataFloat = vtkSmartPointer<vtkFloatArray>::New();
		cellDataFloat->Allocate(nbElement);
		float *valuesFloatSet = new float[nbElement]; // deleted by VTK cellData vtkSmartPointer
		resqml2::ContinuousProperty *propertyValue = static_cast<resqml2::ContinuousProperty *>(valuesProperty);
		if (propertyValue->getElementCountPerValue() == 1)
		{
			if (propertyValue->getDimensionsCountOfPatch(0) == 3)
			{
				propertyValue->getFloatValuesOf3dPatch(0, valuesFloatSet, iCellCount, jCellCount, kCellCount, 0, 0, initKIndex);
			}
			else if (propertyValue->getDimensionsCountOfPatch(0) == 1)
			{
				propertyValue->getFloatValuesOfPatch(0, valuesFloatSet, &numValuesInEachDimension, &offsetInEachDimension, 1);
			}
			else
			{
				vtkOutputWindowDisplayErrorText("error in : propertyValue->getDimensionsCountOfPatch (values different of 1 or 3)\n");
			}
		}
		cellDataFloat->SetName(name.c_str());
		cellDataFloat->SetArray(valuesFloatSet, nbElement, 0, vtkAbstractArray::VTK_DATA_ARRAY_DELETE);
		this->dataArray = cellDataFloat;
	}
	else if (typeProperty == "DiscreteProperty")
	{
		vtkSmartPointer<vtkIntArray> cellDataInt = vtkSmartPointer<vtkIntArray>::New();
		cellDataInt->Allocate(nbElement);
		int *valuesIntSet = new int[nbElement]; // deleted by VTK cellData vtkSmartPointer
		resqml2::DiscreteProperty *propertyValue = static_cast<resqml2::DiscreteProperty *>(valuesProperty);
		if (propertyValue->getElementCountPerValue() == 1)
		{
			if (propertyValue->getDimensionsCountOfPatch(0) == 3)
			{
				propertyValue->getIntValuesOf3dPatch(0, valuesIntSet, iCellCount, jCellCount, kCellCount, 0, 0, initKIndex);
			}
			else if (propertyValue->getDimensionsCountOfPatch(0) == 1)
			{
				propertyValue->getIntValuesOfPatch(0, valuesIntSet, &numValuesInEachDimension, &offsetInEachDimension, 1);
			}
			else
			{
				vtkOutputWindowDisplayErrorText("error in : propertyValue->getDimensionsCountOfPatch (values different of 1 or 3)\n");
			}
		}
		cellDataInt->SetName(name.c_str());
		cellDataInt->SetArray(valuesIntSet, nbElement, 0, vtkAbstractArray::VTK_DATA_ARRAY_DELETE);
		this->dataArray = cellDataInt;
	}
	else if (typeProperty == "CategoricalProperty")
	{
		vtkSmartPointer<vtkIntArray> cellDataInt = vtkSmartPointer<vtkIntArray>::New();
		cellDataInt->Allocate(nbElement);
		int *valuesIntSet = new int[nbElement]; // deleted by VTK cellData vtkSmartPointer
		resqml2::CategoricalProperty *propertyValue = static_cast<resqml2::CategoricalProperty *>(valuesProperty);
		if (propertyValue->getElementCountPerValue() == 1)
		{
			if (propertyValue->getDimensionsCountOfPatch(0) == 3)
			{
				propertyValue->getIntValuesOf3dPatch(0, valuesIntSet, iCellCount, jCellCount, kCellCount, 0, 0, initKIndex);
			}
			else if (propertyValue->getDimensionsCountOfPatch(0) == 1)
			{
				propertyValue->getIntValuesOfPatch(0, valuesIntSet, &numValuesInEachDimension, &offsetInEachDimension, 1);
			}
			else
			{
				vtkOutputWindowDisplayErrorText("error in : propertyValue->getDimensionsCountOfPatch (values different of 1 or 3)\n");
			}
		}
		cellDataInt->SetName(name.c_str());
		cellDataInt->SetArray(valuesIntSet, nbElement, 0, vtkAbstractArray::VTK_DATA_ARRAY_DELETE);
		this->dataArray = cellDataInt;
	}
	else
	{
		vtkOutputWindowDisplayErrorText("property not supported...  (hdfDatatypeEnum)\n");
	}
}

// -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
ResqmlPropertyToVtkDataArray::ResqmlPropertyToVtkDataArray(resqml2::AbstractValuesProperty *valuesProperty,
														   long cellCount,
														   long pointCount)
{
	int nbElement = 0;

	setTypeSupport(valuesProperty);

	if (this->support == typeSupport::POINTS)
	{
		nbElement = pointCount;
	}
	else if (this->support == typeSupport::CELLS)
	{
		nbElement = cellCount;
	}

	// verify nbElement != 0
	if (nbElement == 0)
	{
		throw std::invalid_argument("Property indexable element must be points or cells and must contain at least one of this element.");
	}

	unsigned int elementCountPerValue = valuesProperty->getElementCountPerValue();
	if (elementCountPerValue > 1)
	{
		throw std::invalid_argument("Does not support vectorial property yet");
	}

	const std::string name = valuesProperty->getTitle();
	if (valuesProperty->getXmlTag() == resqml2::ContinuousProperty::XML_TAG)
	{
		// defensive code
		const unsigned int totalHDFElementcount = nbElement * elementCountPerValue;
		if (totalHDFElementcount != valuesProperty->getValuesCountOfPatch(0)) 
		{
			throw std::invalid_argument("Property values count of hdfDataset \"" + std::to_string(valuesProperty->getValuesCountOfPatch(0))
				+ "\" does not match the indexable element count in the supporting representation\"" + std::to_string(totalHDFElementcount) + "\"");
		}

		double *valuesDoubleSet = new double[totalHDFElementcount]; // deleted by VTK data vtkSmartPointer
		static_cast<resqml2::ContinuousProperty *>(valuesProperty)->getDoubleValuesOfPatch(0, valuesDoubleSet);

		vtkSmartPointer<vtkDoubleArray> cellDataDouble = vtkSmartPointer<vtkDoubleArray>::New();
		cellDataDouble->Allocate(nbElement * elementCountPerValue);
		cellDataDouble->SetName(name.c_str());
		cellDataDouble->SetArray(valuesDoubleSet, nbElement * elementCountPerValue, 0, vtkAbstractArray::VTK_DATA_ARRAY_DELETE);
		this->dataArray = cellDataDouble;
	}
	else if (valuesProperty->getXmlTag() == resqml2::DiscreteProperty::XML_TAG ||
			 (valuesProperty->getXmlTag() == resqml2::CategoricalProperty::XML_TAG &&
			  static_cast<resqml2::CategoricalProperty *>(valuesProperty)->getStringLookup() != nullptr))
	{
		int *values = new int[nbElement * elementCountPerValue]; // deleted by VTK data vtkSmartPointer
		valuesProperty->getIntValuesOfPatch(0, values);

		vtkSmartPointer<vtkIntArray> cellDataInt = vtkSmartPointer<vtkIntArray>::New();
		cellDataInt->Allocate(nbElement * elementCountPerValue);
		cellDataInt->SetName(name.c_str());
		cellDataInt->SetArray(values, nbElement * elementCountPerValue, 0, vtkAbstractArray::VTK_DATA_ARRAY_DELETE);
		this->dataArray = cellDataInt;
	}
	else
	{
		throw std::invalid_argument("does not support property which are not discrete or categorical or continuous yet");
	}
}

ResqmlPropertyToVtkDataArray::~ResqmlPropertyToVtkDataArray()
{
	this->dataArray = nullptr;
}

//----------------------------------------------------------------------------
void ResqmlPropertyToVtkDataArray::setTypeSupport(resqml2::AbstractValuesProperty *resqmlProperty)
{
	gsoap_eml2_3::resqml22__IndexableElement element = resqmlProperty->getAttachmentKind();
	if (element == gsoap_eml2_3::resqml22__IndexableElement::cells ||
		element == gsoap_eml2_3::resqml22__IndexableElement::triangles)
	{
		this->support = typeSupport::CELLS;
	}
	else if (element == gsoap_eml2_3::resqml22__IndexableElement::nodes)
	{
		this->support = typeSupport::POINTS;
	}
	else
	{
		this->support = typeSupport::UNKNOW;
	}
}
