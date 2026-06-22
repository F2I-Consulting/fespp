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
#include "Mapping/ResqmlAbstractRepresentationToVtkPartitionedDataSet.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>

// include VTK library
#include <vtkCellData.h>
#include <vtkDataSet.h>
#include <vtkDoubleArray.h>
#include <vtkPointData.h>

// FESAPI
#include <fesapi/resqml2/AbstractValuesProperty.h>
#include <fesapi/resqml2/UnstructuredGridRepresentation.h>

// include F2i-consulting Energistics Paraview Plugin
#include "Mapping/ResqmlPropertyToVtkDataArray.h"


#include <vtkCollection.h>
#include <vtkSMColorMapEditorHelper.h>
#include <vtkSMPropertyHelper.h>
#include <vtkSMProxyManager.h>
#include <vtkSMProxySelectionModel.h>
#include <vtkSMRepresentationProxy.h>
#include <vtkSMSessionProxyManager.h>
#include <vtkSMViewProxy.h>


//----------------------------------------------------------------------------
ResqmlAbstractRepresentationToVtkPartitionedDataSet::ResqmlAbstractRepresentationToVtkPartitionedDataSet(const RESQML2_NS::AbstractRepresentation* p_abstractRepresentation, uint32_t p_procNumber, uint32_t p_maxProc)
	: CommonAbstractObjectToVtkPartitionedDataSet(p_abstractRepresentation,
		p_procNumber,
		p_maxProc),
	_subrepPointerOnPointsCount(0),
	_resqmlData(p_abstractRepresentation),
	_uuidToVtkDataArray()
{
}

char * ResqmlAbstractRepresentationToVtkPartitionedDataSet::addDataArray(const std::string& p_uuid, uint32_t p_patchIndex, bool p_autoActivate,
	const std::string& p_arrayNameSuffix)
{
	std::vector<RESQML2_NS::AbstractValuesProperty*> w_valuesPropertySet = getResqmlData()->getValuesPropertySet();
	std::vector<RESQML2_NS::AbstractValuesProperty*>::iterator w_it = std::find_if(w_valuesPropertySet.begin(), w_valuesPropertySet.end(),
		[&p_uuid](RESQML2_NS::AbstractValuesProperty const* w_property)
		{ return w_property->getUuid() == p_uuid; });

	if (w_it != std::end(w_valuesPropertySet))
	{
		// If the uuid is already loaded but with a different VTK array
		// name than what `p_arrayNameSuffix` would produce, force a
		// reload. Without this guard the per-property multi-realization
		// flow can't transition a uuid loaded in legacy mode (suffix="")
		// to per-property mode (suffix="_real_<N>") — the count > 0
		// check would otherwise short-circuit the re-add.
		if (_uuidToVtkDataArray.count(p_uuid) > 0)
		{
			// Construct the expected name using the same logic each ctor
			// uses. BOTH ctors now go through MakeValidNodeName (the
			// single-proc ctor was aligned), so this must too or the
			// reload guard mismatches and forces a needless delete+recreate.
			auto const* const w_resqmlProp = *w_it;
			const std::string w_expectedName =
				ResqmlPropertyToVtkDataArray::MakeValidNodeName(w_resqmlProp->getTitle().c_str())
				+ p_arrayNameSuffix;
			const std::string w_actualName = getDataArrayName(p_uuid);
			if (w_actualName == w_expectedName)
			{
				return nullptr; // already loaded under the right name
			}
			// Name mismatch (typically a mode transition) — drop and
			// fall through to recreate with the correct name.
			deleteDataArray(p_uuid);
		}
		{
			auto const* const w_resqmlProp = *w_it;
			// The suffix flows through to ResqmlPropertyToVtkDataArray's ctor
			// where it's appended to the property's sanitized title before
			// SetName is called on the underlying vtkDataArray. Empty (the
			// default) means "behave exactly like the legacy code path".
			ResqmlPropertyToVtkDataArray* w_fesppProperty = _isHyperslabed
				? new ResqmlPropertyToVtkDataArray(w_resqmlProp,
					_iCellCount * _jCellCount * (_maxKIndex - _initKIndex),
					_pointCount,
					_iCellCount,
					_jCellCount,
					_maxKIndex - _initKIndex,
					_initKIndex,
					p_patchIndex,
					p_arrayNameSuffix)
				: new ResqmlPropertyToVtkDataArray(w_resqmlProp,
					_iCellCount * _jCellCount * _kCellCount,
					_pointCount,
					p_patchIndex,
					p_arrayNameSuffix);
			switch (w_resqmlProp->getAttachmentKind())
			{
			case gsoap_eml2_3::eml23__IndexableElement::cells:
			case gsoap_eml2_3::eml23__IndexableElement::triangles:
				_vtkData->GetPartition(0)->GetCellData()->AddArray(w_fesppProperty->getVtkData());
				if (p_autoActivate)
					ActiveProperty(w_fesppProperty->getVtkData()->GetName(), vtkDataObject::AttributeTypes::CELL);
				break;
			case gsoap_eml2_3::eml23__IndexableElement::nodes:
				_vtkData->GetPartition(0)->GetPointData()->AddArray(w_fesppProperty->getVtkData());
				if (p_autoActivate)
					ActiveProperty(w_fesppProperty->getVtkData()->GetName(), vtkDataObject::AttributeTypes::POINT);
				break;
			default:
				throw std::invalid_argument("The property " + p_uuid + " is attached on a non supported topological element i.e. not cell, not point.");
			}
			_uuidToVtkDataArray[p_uuid] = w_fesppProperty;
			_vtkData->Modified();
			return w_fesppProperty->getVtkData()->GetName();
		}
	}
	else
	{
		throw std::invalid_argument("The property " + p_uuid + "cannot be added since it is not contained in the representation " + getResqmlData()->getUuid());
	}
	return nullptr;
}

char * ResqmlAbstractRepresentationToVtkPartitionedDataSet::addNaNFillDataArray(
	const std::string& p_baseName,
	const std::string& p_arrayNameSuffix,
	bool p_autoActivate)
{
	const uint64_t cellCount = _isHyperslabed
		? static_cast<uint64_t>(_iCellCount) * _jCellCount * (_maxKIndex - _initKIndex)
		: static_cast<uint64_t>(_iCellCount) * _jCellCount * _kCellCount;
	if (cellCount == 0)
		return nullptr;

	const std::string w_arrayName = resolveDataArrayName(p_baseName, p_arrayNameSuffix);

	auto* w_cellData = _vtkData->GetPartition(0)->GetCellData();

	// Idempotent: if the array already exists (e.g. the previous time
	// step is still resident under "keep previous" semantics), do not
	// overwrite it.
	if (w_cellData->HasArray(w_arrayName.c_str()))
		return w_cellData->GetArray(w_arrayName.c_str())->GetName();

	vtkSmartPointer<vtkDoubleArray> w_nanArr = vtkSmartPointer<vtkDoubleArray>::New();
	w_nanArr->SetName(w_arrayName.c_str());
	w_nanArr->SetNumberOfComponents(1);
	w_nanArr->SetNumberOfTuples(cellCount);
	const double w_nan = std::numeric_limits<double>::quiet_NaN();
	double* w_buf = w_nanArr->GetPointer(0);
	std::fill(w_buf, w_buf + cellCount, w_nan);
	w_cellData->AddArray(w_nanArr);
	if (p_autoActivate)
		ActiveProperty(w_arrayName.c_str(), vtkDataObject::AttributeTypes::CELL);
	_vtkData->Modified();
	return w_cellData->GetArray(w_arrayName.c_str())->GetName();
}

std::string ResqmlAbstractRepresentationToVtkPartitionedDataSet::resolveDataArrayName(
	const std::string& p_title, const std::string& p_arrayNameSuffix) const
{
	if (p_title.empty())
		return std::string();
	// Mirror the ctor naming rule exactly. BOTH ctors now sanitize via
	// MakeValidNodeName (the single-proc ctor was aligned to the multi-proc
	// one), so the NaN placeholder lands under the SAME name as the real
	// array — the data<->empty handoff stays a single shared name regardless
	// of proc mode.
	return ResqmlPropertyToVtkDataArray::MakeValidNodeName(p_title.c_str()) + p_arrayNameSuffix;
}

void ResqmlAbstractRepresentationToVtkPartitionedDataSet::removeDataArrayByName(
	const std::string& p_arrayName)
{
	if (p_arrayName.empty())
		return;
	// SAFETY: never remove a REAL (UUID-tracked) array. The NaN-fill
	// placeholder and the real per-step arrays share the SAME VTK array
	// name (the property title), so a blind RemoveArray here would tear
	// out the real array on a normal load — and because the UUID entry
	// in _uuidToVtkDataArray survives, addDataArray's collision guard
	// (which trusts the map) would then SKIP re-adding it, leaving the
	// property invisible. Only proceed when NO tracked array currently
	// owns this name, i.e. the name belongs to an untracked NaN
	// placeholder.
	for (const auto& w_kv : _uuidToVtkDataArray)
	{
		if (w_kv.second != nullptr)
		{
			vtkSmartPointer<vtkDataArray> w_vtk = w_kv.second->getVtkData();
			if (w_vtk != nullptr && w_vtk->GetName() != nullptr
				&& p_arrayName == w_vtk->GetName())
			{
				return; // real array under this name — leave it for addDataArray to manage
			}
		}
	}
	auto* w_cellData = _vtkData->GetPartition(0)->GetCellData();
	if (w_cellData->HasArray(p_arrayName.c_str()))
		w_cellData->RemoveArray(p_arrayName.c_str());
	auto* w_pointData = _vtkData->GetPartition(0)->GetPointData();
	if (w_pointData->HasArray(p_arrayName.c_str()))
		w_pointData->RemoveArray(p_arrayName.c_str());
	_vtkData->Modified();
}

std::string ResqmlAbstractRepresentationToVtkPartitionedDataSet::getDataArrayName(const std::string& p_uuid) const
{
	auto it = _uuidToVtkDataArray.find(p_uuid);
	if (it == _uuidToVtkDataArray.end() || it->second == nullptr)
		return {};
	auto vtkData = it->second->getVtkData();
	if (!vtkData || !vtkData->GetName())
		return {};
	return std::string(vtkData->GetName());
}

void ResqmlAbstractRepresentationToVtkPartitionedDataSet::deleteDataArray(const std::string& p_uuid)
{
	if (auto it = _uuidToVtkDataArray.find(p_uuid); it != _uuidToVtkDataArray.end())
	{
		auto* w_vtkDataArray = it->second;
		const char* w_dataArrayName = w_vtkDataArray->getVtkData()->GetName();
		if (_vtkData->GetPartition(0)->GetCellData()->HasArray(w_dataArrayName))
		{
			_vtkData->GetPartition(0)->GetCellData()->RemoveArray(w_dataArrayName);
		}
		if (_vtkData->GetPartition(0)->GetPointData()->HasArray(w_dataArrayName))
		{
			_vtkData->GetPartition(0)->GetPointData()->RemoveArray(w_dataArrayName);
		}

		// Cleaning
		delete w_vtkDataArray;
		_uuidToVtkDataArray.erase(it);
	}
	// else: property was never added (e.g. time step changed before any load) — nothing to delete
}

void ResqmlAbstractRepresentationToVtkPartitionedDataSet::registerSubRep()
{
	++_subrepPointerOnPointsCount;
}

void ResqmlAbstractRepresentationToVtkPartitionedDataSet::unregisterSubRep()
{
	--_subrepPointerOnPointsCount;
}

unsigned int ResqmlAbstractRepresentationToVtkPartitionedDataSet::subRepLinkedCount()
{
	return _subrepPointerOnPointsCount;
}


int ResqmlAbstractRepresentationToVtkPartitionedDataSet::ActiveProperty(const char* arrayName, vtkDataObject::AttributeTypes type)
{
	vtkSMSessionProxyManager* activeSessionProxyManager = vtkSMProxyManager::GetProxyManager()->GetActiveSessionProxyManager();
	if (!activeSessionProxyManager)
	{
		vtkOutputWindowDisplayErrorText("vtkSMSessionProxyManager not found.\n");
		return 0;
	}
	else
	{
		vtkSMProxySelectionModel* selectionModel = activeSessionProxyManager->GetSelectionModel("ActiveView");
		if (!selectionModel)
		{
			vtkOutputWindowDisplayErrorText("Failed to get ActiveView selection model.");
			return 0;
		}
		else
		{
			vtkSMViewProxy* activeView = vtkSMViewProxy::SafeDownCast(selectionModel->GetCurrentProxy());
			if (!activeView)
			{
				vtkOutputWindowDisplayErrorText("No active view found.\n");
				return 0;
			}
			else
			{
				// Search for the representation for our source
				vtkNew<vtkCollection> representations;
				activeSessionProxyManager->GetProxies("representations", representations);

				for (int i = 0; i < representations->GetNumberOfItems(); i++)
				{
					vtkSMRepresentationProxy* representation =
						vtkSMRepresentationProxy::SafeDownCast(representations->GetItemAsObject(i));
					if (representation && representation->GetProperty("Input"))
					{
						vtkSMPropertyHelper helper(representation->GetProperty("Input"));
						if (helper.GetNumberOfElements() > 0)
						{
							vtkSMColorMapEditorHelper::SetScalarColoring(representation, arrayName, type);
							//strcpy(activeArrayName, arrayName);
							//activeType = type;
							return 1;
						}
					}
				}
			}
		}
	}
	return 1;
}