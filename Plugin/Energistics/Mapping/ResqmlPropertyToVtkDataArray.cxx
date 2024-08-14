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
#include <fesapi/resqml2/AbstractColorMap.h>
#include <fesapi/resqml2_2/ContinuousColorMap.h>
#include <fesapi/resqml2/DiscreteProperty.h>
#include <fesapi/resqml2_2/DiscreteColorMap.h>
#include <fesapi/eml2/PropertyKind.h>
#include <fesapi/eml2_3/GraphicalInformationSet.h>

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

#include <vtkSMPropertyHelper.h>
#include <vtkSMProxyManager.h>
#include <vtkSMSessionProxyManager.h>
#include <vtkSMTransferFunctionManager.h>
#include <vtkSMTransferFunctionProxy.h>

//----------------------------------------------------------------------------
ResqmlPropertyToVtkDataArray::ResqmlPropertyToVtkDataArray(const RESQML2_NS::AbstractValuesProperty* valuesProperty,
	uint64_t cellCount,
	uint64_t pointCount,
	uint32_t iCellCount,
	uint32_t jCellCount,
	uint32_t kCellCount,
	uint32_t initKIndex,
	uint64_t patch_index)
{
	uint64_t nbElement = isSupported(valuesProperty, cellCount, pointCount);

	if (nbElement > 0)
	{
		const uint32_t elementCountPerValue = valuesProperty->getElementCountPerValue();
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
			float* valuesFloatSet = new float[nbElement]; // deleted by VTK cellData vtkSmartPointer
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
			
			dataArray = cellDataFloat;
			dataArray->Modified();

			eml2::PropertyKind* propKind = valuesProperty->getPropertyKind();
			if (propKind)
			{
				applyResqmlPropKindColorMapToVtkDataArray(propKind);
			}
		}
		else if (typeProperty == RESQML2_NS::DiscreteProperty::XML_TAG)
		{
			vtkSmartPointer<vtkIntArray> cellDataInt = vtkSmartPointer<vtkIntArray>::New();
			int32_t* valuesIntSet = new int32_t[nbElement]; // deleted by VTK cellData vtkSmartPointer
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

			dataArray = cellDataInt;
			dataArray->Modified();

			eml2::PropertyKind* propKind = valuesProperty->getPropertyKind();
			if (propKind)
			{
				applyResqmlPropKindColorMapToVtkDataArray(propKind);
			}
		}
		else if (typeProperty == RESQML2_NS::CategoricalProperty::XML_TAG)
		{
			vtkSmartPointer<vtkIntArray> cellDataInt = vtkSmartPointer<vtkIntArray>::New();
			int32_t* valuesIntSet = new int32_t[nbElement]; // deleted by VTK cellData vtkSmartPointer
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

			dataArray = cellDataInt;
			dataArray->Modified();

			eml2::PropertyKind* propKind = valuesProperty->getPropertyKind();
			if (propKind)
			{
				applyResqmlPropKindColorMapToVtkDataArray(propKind);
			}
		}
		else
		{
			vtkOutputWindowDisplayErrorText("property not supported...  (hdfDatatypeEnum)\n");
		}
	}
}

// -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
ResqmlPropertyToVtkDataArray::ResqmlPropertyToVtkDataArray(resqml2::AbstractValuesProperty const* valuesProperty,
	uint64_t cellCount,
	uint64_t pointCount,
	uint64_t patch_index)
{
	uint64_t nbElement = isSupported(valuesProperty, cellCount, pointCount);

	if (nbElement > 0)
	{
		const uint32_t elementCountPerValue = valuesProperty->getElementCountPerValue();
		const std::string name = valuesProperty->getTitle();
		const std::string xmlTag = valuesProperty->getXmlTag();
		if (xmlTag == resqml2::ContinuousProperty::XML_TAG)
		{
			const uint64_t totalHDFElementcount = nbElement * elementCountPerValue;
			if (totalHDFElementcount != valuesProperty->getValuesCountOfPatch(patch_index))
			{
				throw std::invalid_argument("Property values count of hdfDataset \"" + std::to_string(valuesProperty->getValuesCountOfPatch(patch_index)) + "\" does not match the indexable element count in the supporting representation\"" + std::to_string(totalHDFElementcount) + "\"");
			}

			double* valuesDoubleSet = new double[totalHDFElementcount]; // deleted by VTK data vtkSmartPointer
			valuesProperty->getDoubleValuesOfPatch(patch_index, valuesDoubleSet);

			vtkSmartPointer<vtkDoubleArray> cellDataDouble = vtkSmartPointer<vtkDoubleArray>::New();
			cellDataDouble->SetNumberOfComponents(elementCountPerValue);
			cellDataDouble->SetName(name.c_str());
			cellDataDouble->SetArray(valuesDoubleSet, nbElement * elementCountPerValue, 0, vtkAbstractArray::VTK_DATA_ARRAY_DELETE);
			dataArray = cellDataDouble;
			
			dataArray->Modified();

			eml2::PropertyKind* propKind = valuesProperty->getPropertyKind();
			if (propKind)
			{
				applyResqmlPropKindColorMapToVtkDataArray(propKind);
			}
		}
		else if (xmlTag == resqml2::DiscreteProperty::XML_TAG ||
			(xmlTag == resqml2::CategoricalProperty::XML_TAG &&
				static_cast<resqml2::CategoricalProperty const*>(valuesProperty)->getStringLookup() != nullptr))
		{
			int32_t* values = new int32_t[nbElement * elementCountPerValue]; // deleted by VTK data vtkSmartPointer
			valuesProperty->getInt32ValuesOfPatch(patch_index, values);

			vtkSmartPointer<vtkIntArray> cellDataInt = vtkSmartPointer<vtkIntArray>::New();
			cellDataInt->SetNumberOfComponents(elementCountPerValue);
			cellDataInt->SetName(name.c_str());
			cellDataInt->SetArray(values, nbElement * elementCountPerValue, 0, vtkAbstractArray::VTK_DATA_ARRAY_DELETE);
			dataArray = cellDataInt;
			
			dataArray->Modified();

			eml2::PropertyKind* propKind = valuesProperty->getPropertyKind();
			if (propKind)
			{
				applyResqmlPropKindColorMapToVtkDataArray(propKind);
			}
		}
		else
		{
			throw std::invalid_argument("does not support property which are not discrete or categorical or continuous yet");
		}
	}
}

// -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
uint64_t ResqmlPropertyToVtkDataArray::isSupported(resqml2::AbstractValuesProperty const* valuesProperty,
	uint64_t cellCount,
	uint64_t pointCount)
{
	uint64_t nbElement = 0;

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
	return nbElement;
}

void ResqmlPropertyToVtkDataArray::applyResqmlPropKindColorMapToVtkDataArray(eml2::PropertyKind* propertyKind)
{
	std::vector<EML2_3_NS::GraphicalInformationSet*> gisSet = propertyKind->getRepository()->getDataObjects<EML2_3_NS::GraphicalInformationSet>();
	for (unsigned int gisIndex = 0; gisIndex < gisSet.size(); ++gisIndex)
	{
		EML2_3_NS::GraphicalInformationSet* graphicalInformationSet = gisSet[gisIndex];
		for (unsigned int i = 0; i < graphicalInformationSet->getGraphicalInformationSetCount(); ++i)
		{
			for (unsigned int targetIndex = 0; targetIndex < graphicalInformationSet->getTargetObjectCount(i); ++targetIndex)
			{
				COMMON_NS::AbstractObject const* targetObject = graphicalInformationSet->getTargetObject(i, targetIndex);
				if (targetObject->getUuid() == propertyKind->getUuid())
				{
					if (graphicalInformationSet->hasContinuousColorMap(targetObject))
					{
						RESQML2_NS::ContinuousColorMap* continuousColorMap = graphicalInformationSet->getContinuousColorMap(targetObject);
						vtkSMSessionProxyManager* activeSessionProxyManager = vtkSMProxyManager::GetProxyManager()->GetActiveSessionProxyManager();
						if (!activeSessionProxyManager)
						{
							vtkOutputWindowDisplayErrorText("vtkSMSessionProxyManager not found.\n");
						}
						else
						{
							vtkNew<vtkSMTransferFunctionManager> mgr;
							vtkSMTransferFunctionProxy* lutProxy = vtkSMTransferFunctionProxy::SafeDownCast(
								mgr->GetColorTransferFunction(dataArray->GetName(), activeSessionProxyManager));
							if (!lutProxy)
							{
								vtkOutputWindowDisplayErrorText((std::string(dataArray->GetName()) + " not found.\n").c_str());
							}
							else
							{
								/*
								<resqml22:UseLogarithmicMapping xmlns : xsd = "http://www.w3.org/2001/XMLSchema" xsi : type = "xsd:boolean">false< / resqml22:UseLogarithmicMapping>
									<resqml22:UseReverseMapping xmlns : xsd = "http://www.w3.org/2001/XMLSchema" xsi : type = "xsd:boolean">false< / resqml22:UseReverseMapping>

										<resqml22:NullColor xsi:type="resqml22:HsvColor">
											<resqml22:InterpolationMethod xsi:type="resqml22:InterpolationMethod">linear</resqml22:InterpolationMethod>
									*/
								double* range;
								if (graphicalInformationSet->hasColorMapMinMax(targetObject)) {
									range[0] = graphicalInformationSet->getColorMapMin(targetObject);
									range[1] = graphicalInformationSet->getColorMapMax(targetObject);
								}
								else { // get range data values
									double* range = dataArray->GetRange();
								}



								lutProxy->UpdateVTKObjects();
							}
						}
					}
					else if (graphicalInformationSet->hasDiscreteColorMap(targetObject))
					{
					
						RESQML2_NS::DiscreteColorMap* discreteColorMap = graphicalInformationSet->getDiscreteColorMap(targetObject);

						vtkSMSessionProxyManager* activeSessionProxyManager = vtkSMProxyManager::GetProxyManager()->GetActiveSessionProxyManager();
						if (!activeSessionProxyManager)
						{
							vtkOutputWindowDisplayErrorText("vtkSMSessionProxyManager not found.\n");
						}
						else
						{
							vtkNew<vtkSMTransferFunctionManager> mgr;
							vtkSMTransferFunctionProxy* lutProxy = vtkSMTransferFunctionProxy::SafeDownCast(
								mgr->GetColorTransferFunction(dataArray->GetName(), activeSessionProxyManager));
							if (!lutProxy)
							{
								vtkOutputWindowDisplayErrorText((std::string(dataArray->GetName()) + " not found.\n").c_str());
							}
							else
							{
								vtkSMPropertyHelper(lutProxy, "IndexedLookup", true).Set(1);
								std::vector<double> colors;

								bool hasNullColor = discreteColorMap->hasNullColor();
								uint64_t nbColors = discreteColorMap->getColorCount();

								vtkSMPropertyHelper(lutProxy, "Annotations").SetNumberOfElements(0);
								vtkSMPropertyHelper(lutProxy, "Annotations").SetNumberOfElements(hasNullColor? nbColors +1: nbColors);

								double color[3] = { 0.0 /*RED*/, 0.0 /*GREEN*/, 0.0 /*BLUE*/ };
								for (unsigned int colorIndex = 0; colorIndex < nbColors; ++colorIndex) {
									int64_t location = discreteColorMap->getColorLocationInColorMap(colorIndex);
									discreteColorMap->getRgbColor(colorIndex, color[0], color[1], color[2]);
									
									vtkSMPropertyHelper(lutProxy, "Annotations").Set(2 * colorIndex, std::to_string(location).c_str()); // value
									vtkSMPropertyHelper(lutProxy, "Annotations").Set(2 * colorIndex +1, std::to_string(location).c_str()); // annotation
									colors.push_back(color[0]);
									colors.push_back(color[1]);
									colors.push_back(color[2]);
								}

								double nanColor[3] = { 1.0 /*RED*/, 1.0 /*GREEN*/, 0.0 /*BLUE*/ }; //default paraview value
								if (discreteColorMap->hasNullColor())
								{
									discreteColorMap->getNullRgbColor(nanColor[0], nanColor[1], nanColor[2]);

									vtkSMPropertyHelper(lutProxy, "NanColor").Set(0, nanColor[0]);
									vtkSMPropertyHelper(lutProxy, "NanColor").Set(1, nanColor[1]);
									vtkSMPropertyHelper(lutProxy, "NanColor").Set(2, nanColor[2]);
									vtkSMPropertyHelper(lutProxy, "Annotations").Set(2 * nbColors, "2147483647"); // value
									vtkSMPropertyHelper(lutProxy, "Annotations").Set(2 * nbColors+1, "null value"); // annotation
									colors.push_back(nanColor[0]);
									colors.push_back(nanColor[1]);
									colors.push_back(nanColor[2]);
								}
								
								vtkSMPropertyHelper(lutProxy, "IndexedColors").Set(colors.data(), colors.size());

								lutProxy->UpdateVTKObjects();
							}
						}
					}
				}
			}
		}
	}
}

