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

#include <string>

#include <fesapi/nsDefinitions.h>

namespace RESQML2_NS
{
	class AbstractValuesProperty;
	class StringTableLookup;
}
namespace eml2
{
	class PropertyKind;
}

class vtkScalarsToColors;

/** @brief	the data table of a property
 */

class ResqmlPropertyToVtkDataArray
{
public:
	/**
	 * Constructor for multi-processor.
	 *
	 * `nameSuffix` (default empty) is appended verbatim to the
	 * sanitized property title used as the VTK array name. Used by the
	 * per-property multi-realization load path to disambiguate
	 * concurrent realizations of the same property (e.g. "K_real_3"
	 * vs "K_real_7" both attached to the same partition). When empty,
	 * the array name is the property's sanitized title as before.
	 */
	ResqmlPropertyToVtkDataArray(const RESQML2_NS::AbstractValuesProperty* resqmlProperty,
		uint64_t cellCount,
		uint64_t pointCount,
		uint32_t iCellCount,
		uint32_t jCellCount,
		uint32_t kCellCount,
		uint32_t initKIndex,
		uint64_t patch_index,
		const std::string& nameSuffix = std::string());

	/**
	 * Constructor.
	 *
	 * `nameSuffix` (default empty) — see the multi-processor ctor above.
	 */
	ResqmlPropertyToVtkDataArray(RESQML2_NS::AbstractValuesProperty const* resqmlProperty,
		uint64_t cellCount,
		uint64_t pointCount,
		uint64_t patch_index,
		const std::string& nameSuffix = std::string());

	~ResqmlPropertyToVtkDataArray() = default;

	vtkSmartPointer<vtkDataArray> getVtkData() { return dataArray; }

	/**
	 * Build a VTK-friendly node name from a RESQML title by stripping
	 * characters outside `[-.0-9A-Z_a-z]`. Exposed publicly so callers
	 * (e.g. the per-property MR load) can reconstruct the expected VTK
	 * array name for a given title + suffix without having to know
	 * which constructor was used to load the data.
	 */
	static std::string MakeValidNodeName(const char* p_name);

private:

	uint64_t getNumberOfValues(RESQML2_NS::AbstractValuesProperty const* resqmlProperty,
		uint64_t cellCount,
		uint64_t pointCount);

	/**
	 * Build a vtkDoubleArray from an int32 buffer, substituting the FESAPI
	 * integer null/sentinel value (p_nullValue) with quiet_NaN(). RESQML
	 * discrete/categorical values read here are int32 (|x| < 2^31 < 2^53),
	 * so every non-null value round-trips through double EXACTLY; only the
	 * uncovered/null cells become NaN and thus render with the LUT's
	 * NanColor/NanOpacity (transparent). Do NOT widen the source read to
	 * int64 without revisiting this exactness guarantee. Returns a named,
	 * ready-to-attach double array; the categorical LUT keys on integer
	 * VALUE (not dtype) so the StringTableLookup annotations still match.
	 */
	static vtkSmartPointer<vtkDataArray> buildDoubleArrayWithNullAsNaN(
		const int32_t* p_src,
		int64_t p_nullValue,
		uint64_t p_tupleCount,
		int p_componentCount,
		const std::string& p_name);

	void applyResqmlPropKindColorMapToVtkDataArray(eml2::PropertyKind* propertyKind);

	/**
	 * Populate the active ParaView LUT for `dataArray` from a RESQML
	 * StringTableLookup (the "facies index → name" map). Switches the
	 * LUT into IndexedLookup mode, fills `Annotations` with the
	 * key/label pairs, and seeds `IndexedColors` / `IndexedOpacities`
	 * with a default HSV palette so the categorical color bar renders
	 * out of the box. The trame side's CategoricalColorEditor can
	 * later override the colors per user pick.
	 */
	void applyStringTableLookupToLut(RESQML2_NS::StringTableLookup* lookup);

	vtkSmartPointer<vtkDataArray> dataArray;
};
#endif
