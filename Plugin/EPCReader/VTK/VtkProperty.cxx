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
#include "VtkProperty.h"
#include "vtkMath.h"

// FESAPI
#include <fesapi/resqml2/AbstractValuesProperty.h>
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
VtkProperty::VtkProperty(const std::string & fileName, const std::string & name, const std::string & uuid, const std::string & uuidParent, COMMON_NS::DataObjectRepository const * repo, int idProc, int maxProc) :
	VtkAbstractObject(fileName, name, uuid, uuidParent, idProc, maxProc), repository(repo)
{
	support = typeSupport::CELLS;
}

//----------------------------------------------------------------------------
VtkProperty::~VtkProperty()
{
	cellData = NULL;

	if (repository != nullptr) {
		repository = nullptr;
	}
}

//----------------------------------------------------------------------------
void VtkProperty::visualize(const std::string &)
{
}

//----------------------------------------------------------------------------
void VtkProperty::createTreeVtk(const std::string &, const std::string &, const std::string &, VtkEpcCommon::Resqml2Type)
{
}

//----------------------------------------------------------------------------
void VtkProperty::remove(const std::string &)
{
	cellData=nullptr;
}

//----------------------------------------------------------------------------
unsigned int VtkProperty::getSupport()
{
	return support;
}

//----------------------------------------------------------------------------
vtkDataArray* VtkProperty::visualize(const std::string &, RESQML2_NS::PolylineSetRepresentation const * polylineSetRepresentation)
{
	std::vector<RESQML2_NS::AbstractValuesProperty*> valuesPropertySet = polylineSetRepresentation->getValuesPropertySet();
	long pointCount = polylineSetRepresentation->getXyzPointCountOfPatch(0);
	return loadValuesPropertySet(valuesPropertySet, 0, pointCount);
}

//----------------------------------------------------------------------------
vtkDataArray* VtkProperty::visualize(const std::string &, RESQML2_NS::TriangulatedSetRepresentation const * triangulatedSetRepresentation)
{
	std::vector<RESQML2_NS::AbstractValuesProperty*> valuesPropertySet = triangulatedSetRepresentation->getValuesPropertySet();
	long cellCount = triangulatedSetRepresentation->getTriangleCountOfAllPatches();
	long pointCount = triangulatedSetRepresentation->getXyzPointCountOfAllPatches();
	return loadValuesPropertySet(valuesPropertySet, cellCount, pointCount);
}

//----------------------------------------------------------------------------
vtkDataArray* VtkProperty::visualize(const std::string &, RESQML2_NS::Grid2dRepresentation const * grid2dRepresentation)
{
	std::vector<RESQML2_NS::AbstractValuesProperty*> valuesPropertySet = grid2dRepresentation->getValuesPropertySet();
	long pointCount = grid2dRepresentation->getNodeCountAlongIAxis() * grid2dRepresentation->getNodeCountAlongJAxis();
	return loadValuesPropertySet(valuesPropertySet, 0, pointCount);
}

//----------------------------------------------------------------------------
vtkDataArray* VtkProperty::visualize(const std::string &, RESQML2_NS::AbstractIjkGridRepresentation const * ijkGridRepresentation )
{
	std::vector<RESQML2_NS::AbstractValuesProperty*> valuesPropertySet = ijkGridRepresentation->getValuesPropertySet();

	long cellCount = ijkGridRepresentation->getCellCount();
	return loadValuesPropertySet(valuesPropertySet,cellCount, 0);
}

//----------------------------------------------------------------------------
vtkDataArray* VtkProperty::visualize(const std::string &, RESQML2_NS::UnstructuredGridRepresentation const * unstructuredGridRepresentation)
{
	std::vector<RESQML2_NS::AbstractValuesProperty*> valuesPropertySet = unstructuredGridRepresentation->getValuesPropertySet();

	const ULONG64 cellCount = unstructuredGridRepresentation->getCellCount();
	const ULONG64 pointCount = unstructuredGridRepresentation->getXyzPointCountOfAllPatches();
	return loadValuesPropertySet(valuesPropertySet,cellCount, pointCount);
}

//----------------------------------------------------------------------------
vtkDataArray* VtkProperty::visualize(const std::string &, RESQML2_NS::WellboreTrajectoryRepresentation const * wellboreTrajectoryRepresentation)
{
	std::vector<RESQML2_NS::AbstractValuesProperty*> valuesPropertySet = wellboreTrajectoryRepresentation->getValuesPropertySet();
	long pointCount = wellboreTrajectoryRepresentation->getXyzPointCountOfAllPatches();
	return loadValuesPropertySet(valuesPropertySet, 0, pointCount);
}

//----------------------------------------------------------------------------
vtkSmartPointer<vtkDataArray> VtkProperty::loadValuesPropertySet(const std::vector<RESQML2_NS::AbstractValuesProperty*>& valuesPropertySet, long cellCount, long pointCount)
{
	for (size_t i = 0; i < valuesPropertySet.size(); ++i) {
		if (valuesPropertySet[i]->getUuid() == getUuid()) {
			RESQML2_NS::AbstractValuesProperty* valuesProperty = valuesPropertySet[i];

			int nbElement = 0;
			gsoap_eml2_3::resqml22__IndexableElement element = valuesProperty->getAttachmentKind();
			if (element == gsoap_eml2_3::resqml22__IndexableElement__cells ||
			    element == gsoap_eml2_3::resqml22__IndexableElement__triangles ) {
				nbElement = cellCount;
				support = typeSupport::CELLS;
			}
			else if (element == gsoap_eml2_3::resqml22__IndexableElement__nodes ) {
				support = typeSupport::POINTS ;
				nbElement = pointCount;
			} 
			else {
				vtkOutputWindowDisplayDebugText("property not supported...  (resqml2__IndexableElements: not cells or triangles or nodes)");
				continue;
			}
			// verify nbElement != 0
			if (nbElement == 0) {
				vtkOutputWindowDisplayDebugText("property not supported...");
				continue;
			}

			unsigned int elementCountPerValue = valuesProperty->getElementCountPerValue();
			if (elementCountPerValue > 1) {
				cerr << "does not support vectorial property yet" << endl;
				continue;
			}

			const std::string name = valuesProperty->getTitle();
			if (valuesProperty->getXmlTag() == RESQML2_NS::ContinuousProperty::XML_TAG) {
				double* valuesDoubleSet = new double[nbElement * elementCountPerValue]; // deleted by VTK cellData vtkSmartPointer
				static_cast<RESQML2_NS::ContinuousProperty*>(valuesProperty)->getDoubleValuesOfPatch(0, valuesDoubleSet);

				vtkSmartPointer<vtkDoubleArray> cellDataDouble = vtkSmartPointer<vtkDoubleArray>::New();
				cellDataDouble->Allocate(nbElement * elementCountPerValue);
				cellDataDouble->SetName(name.c_str());
				cellDataDouble->SetArray(valuesDoubleSet, nbElement * elementCountPerValue, 0, vtkAbstractArray::VTK_DATA_ARRAY_DELETE);
				cellData = cellDataDouble;
			}
			else if (valuesProperty->getXmlTag() == RESQML2_NS::DiscreteProperty::XML_TAG || 
				(valuesProperty->getXmlTag() == RESQML2_NS::CategoricalProperty::XML_TAG && static_cast<RESQML2_NS::CategoricalProperty*>(valuesProperty)->getStringLookup() != nullptr))	{
				int* values = new int[nbElement * elementCountPerValue]; // deleted by VTK cellData vtkSmartPointer
				valuesProperty->getIntValuesOfPatch(0, values);

				vtkSmartPointer<vtkIntArray> cellDataInt = vtkSmartPointer<vtkIntArray>::New();
				cellDataInt->Allocate(nbElement * elementCountPerValue);
				cellDataInt->SetName(name.c_str());
				cellDataInt->SetArray(values, nbElement * elementCountPerValue, 0, vtkAbstractArray::VTK_DATA_ARRAY_DELETE);
				cellData = cellDataInt;
			}
			else {
				cerr << "does not support property which are not discrete or categorical or continuous yet" << endl;
			}
		}
	}
	return cellData;
}

//----------------------------------------------------------------------------
vtkSmartPointer<vtkDataArray> VtkProperty::loadValuesPropertySet(const std::vector<RESQML2_NS::AbstractValuesProperty*>& valuesPropertySet, long cellCount, long pointCount,
	int iCellCount, int jCellCount, int kCellCount, int initKIndex)
{
	for (size_t i = 0; i < valuesPropertySet.size(); ++i) {
		if (valuesPropertySet[i]->getUuid() == getUuid()) {
			RESQML2_NS::AbstractValuesProperty* valuesProperty = valuesPropertySet[i];

			int nbElement = 0;
			gsoap_eml2_3::resqml22__IndexableElement element = valuesPropertySet[i]->getAttachmentKind();
			if (element == gsoap_eml2_3::resqml22__IndexableElement__cells) {
				nbElement = cellCount;
				support = typeSupport::CELLS;
			}
			else if (element == gsoap_eml2_3::resqml22__IndexableElement__nodes) {
				support = typeSupport::POINTS ;
				nbElement = pointCount;
			}
			else {
				vtkOutputWindowDisplayDebugText("property not supported...  (resqml2__IndexableElements: not cells or nodes)");
				continue;
			}

			std::string typeProperty = valuesProperty->getXmlTag();

			unsigned long long numValuesInEachDimension = cellCount; //cellCount/kCellCount; //3834;//iCellCount*jCellCount*kCellCount;
			unsigned long long offsetInEachDimension = iCellCount*jCellCount*initKIndex; //initKIndex;//iCellCount*jCellCount*initKIndex;

			const std::string name = valuesPropertySet[i]->getTitle();
			if (typeProperty == "ContinuousProperty") {
				vtkSmartPointer<vtkFloatArray> cellDataFloat = vtkSmartPointer<vtkFloatArray>::New();
				cellDataFloat->Allocate(nbElement);
				float* valuesFloatSet = new float[nbElement]; // deleted by VTK cellData vtkSmartPointer
				RESQML2_NS::ContinuousProperty *propertyValue = static_cast<RESQML2_NS::ContinuousProperty*>(valuesPropertySet[i]);
				if (propertyValue->getElementCountPerValue() == 1) {
					if (propertyValue->getDimensionsCountOfPatch(0) == 3) {
						propertyValue->getFloatValuesOf3dPatch(0, valuesFloatSet,iCellCount, jCellCount, kCellCount, 0, 0, initKIndex);
					}
					else if(propertyValue->getDimensionsCountOfPatch(0) == 1) {
						propertyValue->getFloatValuesOfPatch(0, valuesFloatSet, &numValuesInEachDimension, &offsetInEachDimension, 1);
					}
					else {
						vtkOutputWindowDisplayDebugText("error in : propertyValue->getDimensionsCountOfPatch (values different of 1 or 3)");
					}
				}
				cellDataFloat->SetName(name.c_str());
				cellDataFloat->SetArray(valuesFloatSet, nbElement, 0, vtkAbstractArray::VTK_DATA_ARRAY_DELETE);
				cellData = cellDataFloat;
			}
			else if(typeProperty == "DiscreteProperty") {
				vtkSmartPointer<vtkIntArray> cellDataInt = vtkSmartPointer<vtkIntArray>::New();
				cellDataInt->Allocate(nbElement);
				int* valuesIntSet = new int[nbElement]; // deleted by VTK cellData vtkSmartPointer
				RESQML2_NS::DiscreteProperty *propertyValue = static_cast<RESQML2_NS::DiscreteProperty*>(valuesPropertySet[i]);
				if (propertyValue->getElementCountPerValue() == 1) {
					if (propertyValue->getDimensionsCountOfPatch(0) == 3) {
						propertyValue->getIntValuesOf3dPatch(0, valuesIntSet,iCellCount, jCellCount, kCellCount, 0, 0, initKIndex);
					}
					else if(propertyValue->getDimensionsCountOfPatch(0) == 1) {
						propertyValue->getIntValuesOfPatch(0, valuesIntSet, &numValuesInEachDimension, &offsetInEachDimension, 1);
					}
					else {
						vtkOutputWindowDisplayDebugText("error in : propertyValue->getDimensionsCountOfPatch (values different of 1 or 3)");
					}
				}
				cellDataInt->SetName(name.c_str());
				cellDataInt->SetArray(valuesIntSet, nbElement, 0, vtkAbstractArray::VTK_DATA_ARRAY_DELETE);
				cellData = cellDataInt;
			}
			else if (typeProperty == "CategoricalProperty") {
				vtkSmartPointer<vtkIntArray> cellDataInt = vtkSmartPointer<vtkIntArray>::New();
				cellDataInt->Allocate(nbElement);
				int* valuesIntSet = new int[nbElement]; // deleted by VTK cellData vtkSmartPointer
				RESQML2_NS::CategoricalProperty *propertyValue = static_cast<RESQML2_NS::CategoricalProperty*>(valuesPropertySet[i]);
				if (propertyValue->getElementCountPerValue() == 1) {
					if (propertyValue->getDimensionsCountOfPatch(0) == 3) {
						propertyValue->getIntValuesOf3dPatch(0, valuesIntSet,iCellCount, jCellCount, kCellCount, 0, 0, initKIndex);
					}
					else if(propertyValue->getDimensionsCountOfPatch(0) == 1) {
						propertyValue->getIntValuesOfPatch(0, valuesIntSet, &numValuesInEachDimension, &offsetInEachDimension, 1);
					}
					else {
						vtkOutputWindowDisplayDebugText("error in : propertyValue->getDimensionsCountOfPatch (values different of 1 or 3)");
					}

				}
				cellDataInt->SetName(name.c_str());
				cellDataInt->SetArray(valuesIntSet, nbElement, 0, vtkAbstractArray::VTK_DATA_ARRAY_DELETE);
				cellData = cellDataInt;
			}
			else {
				vtkOutputWindowDisplayDebugText("property not supported...  (hdfDatatypeEnum)");
			}
		}
	}
	return cellData;
}
long VtkProperty::getAttachmentPropertyCount(const std::string &, VtkEpcCommon::FesppAttachmentProperty)
{
	return 0;
}
