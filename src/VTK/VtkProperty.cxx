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
#include <fesapi/resqml2_0_1/CategoricalProperty.h>
#include <fesapi/resqml2_0_1/ContinuousProperty.h>
#include <fesapi/resqml2_0_1/DiscreteProperty.h>
#include <fesapi/resqml2_0_1/AbstractIjkGridRepresentation.h>
#include <fesapi/resqml2_0_1/PolylineSetRepresentation.h>
#include <fesapi/resqml2_0_1/TriangulatedSetRepresentation.h>
#include <fesapi/resqml2_0_1/Grid2dRepresentation.h>
#include <fesapi/resqml2_0_1/UnstructuredGridRepresentation.h>
#include <fesapi/resqml2_0_1/WellboreTrajectoryRepresentation.h>

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
VtkProperty::VtkProperty(const std::string & fileName, const std::string & name, const std::string & uuid, const std::string & uuidParent, COMMON_NS::DataObjectRepository *repo, int idProc, int maxProc) :
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
void VtkProperty::visualize(const std::string & uuid)
{
}

//----------------------------------------------------------------------------
void VtkProperty::createTreeVtk(const std::string & uuid, const std::string & parent, const std::string & name, VtkEpcCommon::Resqml2Type resqmlType)
{
}

//----------------------------------------------------------------------------
void VtkProperty::remove(const std::string & uuid)
{
	cellData=nullptr;
}

//----------------------------------------------------------------------------
unsigned int VtkProperty::getSupport()
{
	return support;
}

//----------------------------------------------------------------------------
vtkDataArray* VtkProperty::visualize(const std::string & uuid, resqml2_0_1::PolylineSetRepresentation const * polylineSetRepresentation)
{
	std::vector<resqml2::AbstractValuesProperty*> valuesPropertySet = polylineSetRepresentation->getValuesPropertySet();

	long pointCount = polylineSetRepresentation->getXyzPointCountOfPatch(0);
	return loadValuesPropertySet(valuesPropertySet, 0, pointCount);
}

//----------------------------------------------------------------------------
vtkDataArray* VtkProperty::visualize(const std::string & uuid, resqml2_0_1::TriangulatedSetRepresentation const * triangulatedSetRepresentation)
{
	std::vector<resqml2::AbstractValuesProperty*> valuesPropertySet = triangulatedSetRepresentation->getValuesPropertySet();
	long pointCount = triangulatedSetRepresentation->getXyzPointCountOfAllPatches();
	return loadValuesPropertySet(valuesPropertySet, 0, pointCount);
}

//----------------------------------------------------------------------------
vtkDataArray* VtkProperty::visualize(const std::string & uuid, resqml2_0_1::Grid2dRepresentation const * grid2dRepresentation)
{
	std::vector<resqml2::AbstractValuesProperty*> valuesPropertySet = grid2dRepresentation->getValuesPropertySet();
	long pointCount = grid2dRepresentation->getNodeCountAlongIAxis() * grid2dRepresentation->getNodeCountAlongJAxis();
	return loadValuesPropertySet(valuesPropertySet, 0, pointCount);
}

//----------------------------------------------------------------------------
vtkDataArray* VtkProperty::visualize(const std::string & uuid, resqml2_0_1::AbstractIjkGridRepresentation const * ijkGridRepresentation )
{
	std::vector<resqml2::AbstractValuesProperty*> valuesPropertySet = ijkGridRepresentation->getValuesPropertySet();

	long cellCount = ijkGridRepresentation->getCellCount();
	return loadValuesPropertySet(valuesPropertySet,cellCount, 0);
}

//----------------------------------------------------------------------------
vtkDataArray* VtkProperty::visualize(const std::string & uuid, resqml2_0_1::UnstructuredGridRepresentation const * unstructuredGridRepresentation)
{
	std::vector<resqml2::AbstractValuesProperty*> valuesPropertySet = unstructuredGridRepresentation->getValuesPropertySet();

	const ULONG64 cellCount = unstructuredGridRepresentation->getCellCount();
	const ULONG64 pointCount = unstructuredGridRepresentation->getXyzPointCountOfAllPatches();
	return loadValuesPropertySet(valuesPropertySet,cellCount, pointCount);
}

//----------------------------------------------------------------------------
vtkDataArray* VtkProperty::visualize(const std::string & uuid, resqml2_0_1::WellboreTrajectoryRepresentation const * wellboreTrajectoryRepresentation)
{
	std::vector<resqml2::AbstractValuesProperty*> valuesPropertySet = wellboreTrajectoryRepresentation->getValuesPropertySet();
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
			gsoap_resqml2_0_1::resqml20__IndexableElements element = valuesProperty->getAttachmentKind();
			if (element == gsoap_resqml2_0_1::resqml20__IndexableElements__cells ) {
				nbElement = cellCount;
				support = typeSupport::CELLS;
			}
			else if (element == gsoap_resqml2_0_1::resqml20__IndexableElements__nodes ) {
				support = typeSupport::POINTS ;
				nbElement = pointCount;
			}
			else {
				vtkOutputWindowDisplayDebugText("property not supported...  (resqml2__IndexableElements: not cells or nodes)");
				continue;
			}

			unsigned int elementCountPerValue = valuesProperty->getElementCountPerValue();
			if (elementCountPerValue > 1) {
				cerr << "does not support vectorial property yet" << endl;
				continue;
			}

			const std::string name = valuesProperty->getTitle();
			if (valuesProperty->getXmlTag() == RESQML2_0_1_NS::ContinuousProperty::XML_TAG) {
				double* valuesDoubleSet = new double[nbElement * elementCountPerValue]; // deleted by VTK cellData vtkSmartPointer
				static_cast<RESQML2_0_1_NS::ContinuousProperty*>(valuesProperty)->getDoubleValuesOfPatch(0, valuesDoubleSet);

				vtkSmartPointer<vtkDoubleArray> cellDataDouble = vtkSmartPointer<vtkDoubleArray>::New();
				cellDataDouble->SetName(name.c_str());
				cellDataDouble->SetArray(valuesDoubleSet, nbElement * elementCountPerValue, 0, vtkAbstractArray::VTK_DATA_ARRAY_DELETE);
				cellData = cellDataDouble;
			}
			else if (valuesProperty->getXmlTag() == RESQML2_0_1_NS::DiscreteProperty::XML_TAG || valuesProperty->getXmlTag() == RESQML2_0_1_NS::CategoricalProperty::XML_TAG)	{
				int* values = new int[nbElement * elementCountPerValue]; // deleted by VTK cellData vtkSmartPointer
				valuesProperty->getIntValuesOfPatch(0, values);

				vtkSmartPointer<vtkIntArray> cellDataInt = vtkSmartPointer<vtkIntArray>::New();
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
			gsoap_resqml2_0_1::resqml20__IndexableElements element = valuesPropertySet[i]->getAttachmentKind();
			if (element == gsoap_resqml2_0_1::resqml20__IndexableElements__cells) {
				nbElement = cellCount;
				support = typeSupport::CELLS;
			}
			else if (element == gsoap_resqml2_0_1::resqml20__IndexableElements__nodes) {
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
				float* valuesFloatSet = new float[nbElement];
				resqml2_0_1::ContinuousProperty *propertyValue = static_cast<resqml2_0_1::ContinuousProperty*>(valuesPropertySet[i]);
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
				int* valuesIntSet = new int[nbElement];
				resqml2_0_1::DiscreteProperty *propertyValue = static_cast<resqml2_0_1::DiscreteProperty*>(valuesPropertySet[i]);
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
				int* valuesIntSet = new int[nbElement];
				resqml2_0_1::CategoricalProperty *propertyValue = static_cast<resqml2_0_1::CategoricalProperty*>(valuesPropertySet[i]);
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
long VtkProperty::getAttachmentPropertyCount(const std::string & uuid, VtkEpcCommon::FesppAttachmentProperty propertyUnit)
{
	return 0;
}
