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

#include <algorithm>
#include <cstdint>
#include <limits>
#include <utility>
#include <vector>

// FESAPI
#include <fesapi/resqml2/CategoricalProperty.h>
#include <fesapi/resqml2/ContinuousProperty.h>
#include <fesapi/resqml2/AbstractColorMap.h>
#include <fesapi/resqml2_2/ContinuousColorMap.h>
#include <fesapi/resqml2/DiscreteProperty.h>
#include <fesapi/resqml2_2/DiscreteColorMap.h>
#include <fesapi/resqml2/StringTableLookup.h>
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
	uint64_t patch_index,
	const std::string& nameSuffix)
{
	uint64_t nbElement = getNumberOfValues(valuesProperty, cellCount, pointCount);

	// Final VTK array name = sanitized title + caller-supplied suffix.
	// Empty suffix (default) reproduces the legacy "name == sanitized
	// title" behaviour; a non-empty suffix (e.g. "_real_3") is used by
	// the per-property multi-realization load to disambiguate
	// concurrent realizations of the same property.
	const std::string w_baseName = MakeValidNodeName(valuesProperty->getTitle().c_str());
	const std::string w_arrayName = w_baseName + nameSuffix;

	if (nbElement > 0)
	{
		const auto elementCountPerValue = valuesProperty->getValueCountPerIndexableElement();
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
			cellDataFloat->SetName(w_arrayName.c_str());
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
			else if (valuesProperty->getDimensionsCountOfPatch(patch_index) == 1)
			{
				valuesProperty->getIntValuesOfPatch(patch_index, valuesIntSet, &numValuesInEachDimension, &offsetInEachDimension, 1);
			}
			else
			{
				vtkOutputWindowDisplayErrorText("error in : propertyValue->getDimensionsCountOfPatch (values different of 1 or 3)\n");
			}
			cellDataInt->SetName(w_arrayName.c_str());
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
			cellDataInt->SetName(w_arrayName.c_str());
			cellDataInt->SetArray(valuesIntSet, nbElement, 0, vtkAbstractArray::VTK_DATA_ARRAY_DELETE);

			dataArray = cellDataInt;
			dataArray->Modified();

			eml2::PropertyKind* propKind = valuesProperty->getPropertyKind();
			if (propKind)
			{
				applyResqmlPropKindColorMapToVtkDataArray(propKind);
			}

			// Propagate the StringTableLookup labels to the LUT (same
			// rationale as in the single-processor constructor below).
			auto const* catProp = static_cast<RESQML2_NS::CategoricalProperty const*>(valuesProperty);
			RESQML2_NS::StringTableLookup* lookup = catProp->getStringLookup();
			if (lookup != nullptr && lookup->getItemCount() > 0)
			{
				applyStringTableLookupToLut(lookup);
			}
		}
		else
		{
			vtkOutputWindowDisplayErrorText("property not supported...  (hdfDatatypeEnum)\n");
		}
	}
}

std::string ResqmlPropertyToVtkDataArray::MakeValidNodeName(const char* p_name)
{
	if (p_name == nullptr || p_name[0] == '\0')
	{
		return std::string();
	}

	const char w_sortedValidChars[] =
		"-.0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ_abcdefghijklmnopqrstuvwxyz";
	const auto w_sortedValidCharsLen = strlen(w_sortedValidChars);

	std::string w_result;
	w_result.reserve(strlen(p_name));
	for (size_t w_cc = 0, max = strlen(p_name); w_cc < max; ++w_cc)
	{
		if (std::binary_search(
			w_sortedValidChars, w_sortedValidChars + w_sortedValidCharsLen, p_name[w_cc]))
		{
			w_result += p_name[w_cc];
		}
	}

	if (w_result.empty() ||
		((w_result[0] < 'a' || w_result[0] > 'z') && (w_result[0] < 'A' || w_result[0] > 'Z') &&
			w_result[0] != '_'))
	{
		return "_" + w_result;
	}
	return w_result;
}

// -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
vtkSmartPointer<vtkDataArray> ResqmlPropertyToVtkDataArray::buildDoubleArrayWithNullAsNaN(
	const int32_t* p_src,
	int64_t p_nullValue,
	uint64_t p_tupleCount,
	int p_componentCount,
	const std::string& p_name)
{
	const uint64_t w_total = p_tupleCount * static_cast<uint64_t>(p_componentCount);
	double* w_dbl = new double[w_total]; // handed to VTK below (VTK_DATA_ARRAY_DELETE)
	const double w_nan = std::numeric_limits<double>::quiet_NaN();
	for (uint64_t i = 0; i < w_total; ++i)
	{
		// int32 values are exact in double; only the FESAPI null becomes NaN.
		w_dbl[i] = (static_cast<int64_t>(p_src[i]) == p_nullValue)
			? w_nan
			: static_cast<double>(p_src[i]);
	}
	vtkSmartPointer<vtkDoubleArray> w_arr = vtkSmartPointer<vtkDoubleArray>::New();
	w_arr->SetNumberOfComponents(p_componentCount);
	w_arr->SetName(p_name.c_str());
	w_arr->SetArray(w_dbl, w_total, 0, vtkAbstractArray::VTK_DATA_ARRAY_DELETE);
	return w_arr;
}

// -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
ResqmlPropertyToVtkDataArray::ResqmlPropertyToVtkDataArray(resqml2::AbstractValuesProperty const* valuesProperty,
	uint64_t cellCount,
	uint64_t pointCount,
	uint64_t patch_index,
	const std::string& nameSuffix)
{
	uint64_t numberOfValues = getNumberOfValues(valuesProperty, cellCount, pointCount);

	if (numberOfValues > 0)
	{
		const uint64_t elementCountPerValue = valuesProperty->getValueCountPerIndexableElement();
		// Sanitize the title through MakeValidNodeName, exactly like the
		// multi-processor ctor, then append the optional suffix (per-property
		// MR realizations). The single-proc path is the DEFAULT (non-MPI)
		// path, so without this its VTK array names carried raw titles
		// (spaces/parens/units) while the Python side computes
		// make_valid_vtk_name(title) — a silent mismatch that blanked the
		// COE / broke stats for any title with stripped chars.
		const std::string name = MakeValidNodeName(valuesProperty->getTitle().c_str()) + nameSuffix;
		const std::string xmlTag = valuesProperty->getXmlTag();
		if (xmlTag == resqml2::ContinuousProperty::XML_TAG)
		{
			const uint64_t totalNumberOfValues = numberOfValues * elementCountPerValue;
			if (totalNumberOfValues != valuesProperty->getValuesCountOfPatch(patch_index))
			{
				throw std::invalid_argument("Property values count of hdfDataset \"" + std::to_string(valuesProperty->getValuesCountOfPatch(patch_index)) + "\" does not match the indexable element count in the supporting representation\"" + std::to_string(totalNumberOfValues) + "\"");
			}

			double* valuesDoubleSet = new double[totalNumberOfValues]; // deleted by VTK data vtkSmartPointer
			valuesProperty->getArrayOfValuesOfPatch(patch_index, valuesDoubleSet);

			vtkSmartPointer<vtkDoubleArray> cellDataDouble = vtkSmartPointer<vtkDoubleArray>::New();
			cellDataDouble->SetNumberOfComponents(elementCountPerValue);
			cellDataDouble->SetName(name.c_str());
			cellDataDouble->SetArray(valuesDoubleSet, numberOfValues * elementCountPerValue, 0, vtkAbstractArray::VTK_DATA_ARRAY_DELETE);
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
			int32_t* values = new int32_t[numberOfValues * elementCountPerValue]; // freed below after copy into the double array
			// getArrayOfValuesOfPatch fills `values` and returns NumberArrayStatistics
			// whose getNullValue() is the property's integer null/sentinel (in the
			// default path this comes from the persisted statistics, which derive
			// from getNullValueOfPatch — no extra HDF5 read). For PVTregionPVTNUM
			// this is INT_MAX on the cells the property does not cover.
			const int64_t w_nullValue =
				valuesProperty->getArrayOfValuesOfPatch(patch_index, values).getNullValue();

			// Emit a vtkDoubleArray mapping the null value -> quiet_NaN so
			// uncovered cells render via the LUT's NanColor/NanOpacity
			// (transparent) instead of clamping to the max-LUT color. Integer
			// category values are preserved exactly; the categorical editor and
			// the StringTableLookup annotations key on integer VALUE, not dtype.
			dataArray = buildDoubleArrayWithNullAsNaN(
				values, w_nullValue, numberOfValues,
				static_cast<int>(elementCountPerValue), name);
			delete[] values;

			dataArray->Modified();

			eml2::PropertyKind* propKind = valuesProperty->getPropertyKind();
			if (propKind)
			{
				applyResqmlPropKindColorMapToVtkDataArray(propKind);
			}

			// Propagate the StringTableLookup (RESQML "facies index → name"
			// map) into the corresponding ParaView LUT's Annotations so the
			// color bar, the Color Editor and the threshold panel all show
			// the human-readable labels. The LUT proxy is fetched (and
			// created on demand) via vtkSMTransferFunctionManager so the
			// annotations are in place by the time ColorBy first runs.
			if (xmlTag == resqml2::CategoricalProperty::XML_TAG)
			{
				auto const* catProp = static_cast<resqml2::CategoricalProperty const*>(valuesProperty);
				RESQML2_NS::StringTableLookup* lookup = catProp->getStringLookup();
				if (lookup != nullptr && lookup->getItemCount() > 0)
				{
					applyStringTableLookupToLut(lookup);
				}
			}
		}
		else
		{
			throw std::invalid_argument("does not support property which are not discrete or categorical or continuous yet");
		}
	}
}

// -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
uint64_t ResqmlPropertyToVtkDataArray::getNumberOfValues(resqml2::AbstractValuesProperty const* valuesProperty,
	uint64_t cellCount,
	uint64_t pointCount)
{
	const gsoap_eml2_3::eml23__IndexableElement element = valuesProperty->getAttachmentKind();
	if (element == gsoap_eml2_3::eml23__IndexableElement::cells ||
		element == gsoap_eml2_3::eml23__IndexableElement::triangles)
	{
		return cellCount;
	}
	else if (element == gsoap_eml2_3::eml23__IndexableElement::nodes)
	{
		return pointCount;
	}
	else
	{
		throw std::invalid_argument("Property indexable element must be points or cells.");
	}
}

void ResqmlPropertyToVtkDataArray::applyStringTableLookupToLut(RESQML2_NS::StringTableLookup* lookup)
{
	if (lookup == nullptr || dataArray == nullptr)
	{
		return;
	}
	vtkSMSessionProxyManager* activeSessionProxyManager =
		vtkSMProxyManager::GetProxyManager()->GetActiveSessionProxyManager();
	if (!activeSessionProxyManager)
	{
		vtkOutputWindowDisplayErrorText("vtkSMSessionProxyManager not found.\n");
		return;
	}
	vtkNew<vtkSMTransferFunctionManager> mgr;
	vtkSMTransferFunctionProxy* lutProxy = vtkSMTransferFunctionProxy::SafeDownCast(
		mgr->GetColorTransferFunction(dataArray->GetName(), activeSessionProxyManager));
	if (!lutProxy)
	{
		vtkOutputWindowDisplayErrorText(
			(std::string(dataArray->GetName()) + " LUT not found.\n").c_str());
		return;
	}

	// Switch the LUT into IndexedLookup mode — ParaView's categorical
	// rendering: discrete swatches, one annotation per (value, label)
	// pair, color bar shows labels instead of a gradient. Without
	// IndexedLookup=1 the Annotations would be ignored and the color
	// bar would stay continuous (the bug that prompted this code).
	vtkSMPropertyHelper(lutProxy, "IndexedLookup", true).Set(1);

	// Annotations: ParaView expects a flat string list
	// [value0, label0, value1, label1, ...]. We get the (key, value)
	// pairs via the StringTableLookup's map and serialize the keys to
	// strings.
	const std::unordered_map<int64_t, std::string> w_map = lookup->getMap();
	const uint64_t n = w_map.size();

	// Sort by key so the categorical UI shows entries in a stable order.
	std::vector<std::pair<int64_t, std::string>> w_sorted(w_map.begin(), w_map.end());
	std::sort(w_sorted.begin(), w_sorted.end(),
		[](const std::pair<int64_t, std::string>& p_a,
			const std::pair<int64_t, std::string>& p_b) {
				return p_a.first < p_b.first;
		});

	vtkSMPropertyHelper(lutProxy, "Annotations").SetNumberOfElements(0);
	vtkSMPropertyHelper(lutProxy, "Annotations").SetNumberOfElements(static_cast<unsigned int>(n * 2));
	for (uint64_t i = 0; i < n; ++i)
	{
		vtkSMPropertyHelper(lutProxy, "Annotations").Set(
			static_cast<unsigned int>(2 * i), std::to_string(w_sorted[i].first).c_str());
		vtkSMPropertyHelper(lutProxy, "Annotations").Set(
			static_cast<unsigned int>(2 * i + 1), w_sorted[i].second.c_str());
	}

	// Seed IndexedColors with a default palette when the LUT doesn't
	// have one yet (first activation). We use a simple HSV cycle —
	// the CategoricalColorEditor on the trame side overrides these
	// with user-edited colors when they exist. Without seeding the
	// indexed colors here the LUT would still be in IndexedLookup
	// mode but with no swatches, producing a black color bar.
	vtkSMPropertyHelper colHelper(lutProxy, "IndexedColors");
	if (colHelper.GetNumberOfElements() < n * 3)
	{
		std::vector<double> w_colors(n * 3);
		for (uint64_t i = 0; i < n; ++i)
		{
			// HSV: hue spread across the wheel, full saturation and
			// value. Skip pure red (h=0) by offsetting so categories
			// don't look like a Continuous rainbow.
			const double w_hue = (static_cast<double>(i) + 0.5) / static_cast<double>(n);
			double w_rgb[3];
			vtkMath::HSVToRGB(w_hue, 1.0, 1.0, w_rgb, w_rgb + 1, w_rgb + 2);
			w_colors[3 * i + 0] = w_rgb[0];
			w_colors[3 * i + 1] = w_rgb[1];
			w_colors[3 * i + 2] = w_rgb[2];
		}
		colHelper.Set(w_colors.data(), static_cast<unsigned int>(w_colors.size()));
	}

	// IndexedOpacities seeding: one entry per category, fully opaque.
	// Same rationale as IndexedColors — without this the LUT's
	// EnableOpacityMapping wouldn't pick up the alpha channel.
	vtkSMPropertyHelper opHelper(lutProxy, "IndexedOpacities");
	if (opHelper.GetNumberOfElements() < n)
	{
		std::vector<double> w_op(n, 1.0);
		opHelper.Set(w_op.data(), static_cast<unsigned int>(w_op.size()));
		vtkSMPropertyHelper(lutProxy, "EnableOpacityMapping").Set(1);
	}

	lutProxy->UpdateVTKObjects();
}

void ResqmlPropertyToVtkDataArray::applyResqmlPropKindColorMapToVtkDataArray(eml2::PropertyKind* propertyKind)
{
	for (auto const* graphicalInformationSet: propertyKind->getRepository()->getGraphicalInformationSetSet())
	{
		for (unsigned int i = 0; i < graphicalInformationSet->getGraphicalInformationSetCount(); ++i)
		{
			for (unsigned int targetIndex = 0; targetIndex < graphicalInformationSet->getTargetObjectCount(i); ++targetIndex)
			{
				COMMON_NS::AbstractObject const* targetObject = graphicalInformationSet->getTargetObject(i, targetIndex);
				if (targetObject->getUuid() == propertyKind->getUuid())
				{
					if (graphicalInformationSet->hasContinuousColorMap(targetObject))
					{
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
								/* GIS
									<resqml22:UseLogarithmicMapping xmlns : xsd = "http://www.w3.org/2001/XMLSchema" xsi : type = "xsd:boolean">false< / resqml22:UseLogarithmicMapping>
									<resqml22:UseReverseMapping xmlns : xsd = "http://www.w3.org/2001/XMLSchema" xsi : type = "xsd:boolean">false< / resqml22:UseReverseMapping>
								*/
								double range[2];
								if (graphicalInformationSet->hasColorMapMinMax(targetObject)) {
									range[0] = graphicalInformationSet->getColorMapMin(targetObject);
									range[1] = graphicalInformationSet->getColorMapMax(targetObject);
								}
								else { // get range data values
									dataArray->GetRange(range);
								}

								RESQML2_NS::ContinuousColorMap* continuousColorMap = graphicalInformationSet->getContinuousColorMap(targetObject);

								double nanColor[3] = { 1.0 /*RED*/, 1.0 /*GREEN*/, 0.0 /*BLUE*/ }; //default paraview value
								if (continuousColorMap->hasNullColor())
								{
									continuousColorMap->getNullRgbColor(nanColor[0], nanColor[1], nanColor[2]);

									vtkSMPropertyHelper(lutProxy, "NanColor").Set(0, nanColor[0]);
									vtkSMPropertyHelper(lutProxy, "NanColor").Set(1, nanColor[1]);
									vtkSMPropertyHelper(lutProxy, "NanColor").Set(2, nanColor[2]);
									
									vtkSMPropertyHelper(lutProxy, "NanOpacity").Set(continuousColorMap->getNullAlpha());
								}

								/* name="ColorSpace"
								        <EnumerationDomain name="enum">
											<Entry text="RGB" value="0" />
											<Entry text="HSV" value="1" />
											<Entry text="Lab" value="2" />
											<Entry text="Diverging" value="3" />
											<Entry text="Lab/CIEDE2000" value="4" />
											<Entry text="Step" value="5" />
										</EnumerationDomain>
								*/
								if (continuousColorMap->getInterpolationDomain() == gsoap_eml2_3::resqml22__InterpolationDomain::hsv)
								{
									vtkSMPropertyHelper(lutProxy, "ColorSpace").Set(1);
								}
								else if (continuousColorMap->getInterpolationDomain() == gsoap_eml2_3::resqml22__InterpolationDomain::rgb)
								{
									vtkSMPropertyHelper(lutProxy, "ColorSpace").Set(0);
								}

								std::vector<double> colors;
								std::vector<double> opacities;
								colors.reserve(continuousColorMap->getColorCount() * 4);
								opacities.reserve(continuousColorMap->getColorCount() * 2);
								double color[3] = { 0.0 /*RED*/, 0.0 /*GREEN*/, 0.0 /*BLUE*/ };
								for (unsigned int colorIndex = 0; colorIndex < continuousColorMap->getColorCount(); ++colorIndex) {
									continuousColorMap->getRgbColor(colorIndex, color[0], color[1], color[2]);

									colors.push_back(continuousColorMap->getColorLocationInColorMap(colorIndex));
									colors.push_back(color[0]);
									colors.push_back(color[1]);
									colors.push_back(color[2]);

									opacities.push_back(continuousColorMap->getColorLocationInColorMap(colorIndex));
									opacities.push_back(continuousColorMap->getAlpha(colorIndex));
									opacities.push_back(0.5);
									opacities.push_back(0.0);
								}

								/* name = "AutomaticRescaleRangeMode"
									<EnumerationDomain name = "enum">
										<Entry value = "-1" text = "Never" / >
										<Entry value = "0" text = "Grow and update on 'Apply'" / >
										<Entry value = "1" text = "Grow and update every timestep" / >
										<Entry value = "2" text = "Update on 'Apply'" / >
										<Entry value = "3" text = "Clamp and update every timestep" / >
									< / EnumerationDomain>
								*/
								vtkSMPropertyHelper(lutProxy, "AutomaticRescaleRangeMode").Set(-1); 
								vtkSMPropertyHelper(lutProxy, "RGBPoints").Set(colors.data(), colors.size());
								
								vtkSMTransferFunctionProxy* opacityProxy = vtkSMTransferFunctionProxy::SafeDownCast(mgr->GetOpacityTransferFunction(dataArray->GetName(), activeSessionProxyManager));
								if (!opacityProxy)
								{
									vtkOutputWindowDisplayErrorText((std::string(dataArray->GetName()) + " not found.\n").c_str());
								}
								else
								{
									vtkSMPropertyHelper(lutProxy, "EnableOpacityMapping").Set(1);

									vtkSMPropertyHelper(opacityProxy, "Points").Set(opacities.data(), opacities.size());
									
									opacityProxy->UpdateVTKObjects();
								}
								lutProxy->UpdateVTKObjects();

							}
						}
					}
					else if (graphicalInformationSet->hasDiscreteColorMap(targetObject))
					{
					
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
								RESQML2_NS::DiscreteColorMap* discreteColorMap = graphicalInformationSet->getDiscreteColorMap(targetObject);

								vtkSMPropertyHelper(lutProxy, "IndexedLookup", true).Set(1);

								bool hasNullColor = discreteColorMap->hasNullColor();
								uint64_t nbColors = discreteColorMap->getColorCount();

								vtkSMPropertyHelper(lutProxy, "Annotations").SetNumberOfElements(0);
								vtkSMPropertyHelper(lutProxy, "Annotations").SetNumberOfElements(hasNullColor? nbColors +1: nbColors);

								std::vector<double> colors;
								colors.reserve(hasNullColor ? (nbColors + 1) * 3 : nbColors * 3);
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

