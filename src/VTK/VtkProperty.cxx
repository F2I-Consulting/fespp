/*-----------------------------------------------------------------------
Copyright F2I-CONSULTING, (2014)

cedric.robert@f2i-consulting.com

This software is a computer program whose purpose is to display data formatted using Energistics standards.

This software is governed by the CeCILL license under French law and
abiding by the rules of distribution of free software.  You can  use,
modify and/ or redistribute the software under the terms of the CeCILL
license as circulated by CEA, CNRS and INRIA at the following URL
"http://www.cecill.info".

As a counterpart to the access to the source code and  rights to copy,
modify and redistribute granted by the license, users are provided only
with a limited warranty  and the software's author,  the holder of the
economic rights,  and the successive licensors  have only  limited
liability.

In this respect, the user's attention is drawn to the risks associated
with loading,  using,  modifying and/or developing or reproducing the
software by the user in light of its specific status of free software,
that may mean  that it is complicated to manipulate,  and  that  also
therefore means  that it is reserved for developers  and  experienced
professionals having in-depth computer knowledge. Users are therefore
encouraged to load and test the software's suitability as regards their
requirements in conditions enabling the security of their systems and/or
data to be ensured and,  more generally, to use and operate it in the
same conditions as regards security.

The fact that you are presently reading this means that you have had
knowledge of the CeCILL license and that you accept its terms.
-----------------------------------------------------------------------*/
#include "VtkProperty.h"
#include "vtkMath.h"

// include F2i-consulting Energistics Standards API
#include <resqml2/AbstractValuesProperty.h>
#include <resqml2_0_1/CategoricalProperty.h>
#include <resqml2_0_1/ContinuousProperty.h>
#include <resqml2_0_1/DiscreteProperty.h>
#include <resqml2_0_1/AbstractIjkGridRepresentation.h>
#include <resqml2_0_1/PolylineSetRepresentation.h>
#include <resqml2_0_1/TriangulatedSetRepresentation.h>
#include <resqml2_0_1/Grid2dRepresentation.h>
#include <resqml2_0_1/UnstructuredGridRepresentation.h>
#include <resqml2_0_1/WellboreTrajectoryRepresentation.h>

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
VtkProperty::VtkProperty(const std::string & fileName, const std::string & name, const std::string & uuid, const std::string & uuidParent, common::EpcDocument *pck, const int & idProc, const int & maxProc) :
VtkAbstractObject(fileName, name, uuid, uuidParent, idProc, maxProc), epcPackage(pck)
{
	support = typeSupport::CELLS;
}

//----------------------------------------------------------------------------
VtkProperty::~VtkProperty()
{
	cellData = NULL;

	if (epcPackage != nullptr) {
		epcPackage = nullptr;
	}
}

//----------------------------------------------------------------------------
void VtkProperty::visualize(const std::string & uuid)
{
}

//----------------------------------------------------------------------------
void VtkProperty::createTreeVtk(const std::string & uuid, const std::string & parent, const std::string & name, const VtkEpcCommon::Resqml2Type & resqmlType)
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
vtkDataArray* VtkProperty::visualize(const std::string & uuid, resqml2_0_1::PolylineSetRepresentation* polylineSetRepresentation)
{
	std::vector<resqml2::AbstractValuesProperty*> valuesPropertySet = polylineSetRepresentation->getValuesPropertySet();

	long pointCount = polylineSetRepresentation->getXyzPointCountOfPatch(0);
	return loadValuesPropertySet(valuesPropertySet, 0, pointCount);
}

//----------------------------------------------------------------------------
vtkDataArray* VtkProperty::visualize(const std::string & uuid, resqml2_0_1::TriangulatedSetRepresentation* triangulatedSetRepresentation)
{
	std::vector<resqml2::AbstractValuesProperty*> valuesPropertySet = triangulatedSetRepresentation->getValuesPropertySet();
	long pointCount = triangulatedSetRepresentation->getXyzPointCountOfAllPatches();
	return loadValuesPropertySet(valuesPropertySet, 0, pointCount);
}

//----------------------------------------------------------------------------
vtkDataArray* VtkProperty::visualize(const std::string & uuid, resqml2_0_1::Grid2dRepresentation* grid2dRepresentation)
{
	std::vector<resqml2::AbstractValuesProperty*> valuesPropertySet = grid2dRepresentation->getValuesPropertySet();
	long pointCount = grid2dRepresentation->getNodeCountAlongIAxis() * grid2dRepresentation->getNodeCountAlongJAxis();
	return loadValuesPropertySet(valuesPropertySet, 0, pointCount);
}

//----------------------------------------------------------------------------
vtkDataArray* VtkProperty::visualize(const std::string & uuid, resqml2_0_1::AbstractIjkGridRepresentation* ijkGridRepresentation )
{
	std::vector<resqml2::AbstractValuesProperty*> valuesPropertySet = ijkGridRepresentation->getValuesPropertySet();

	long cellCount = ijkGridRepresentation->getCellCount();
	return loadValuesPropertySet(valuesPropertySet,cellCount, 0);
}

//----------------------------------------------------------------------------
vtkDataArray* VtkProperty::visualize(const std::string & uuid, resqml2_0_1::UnstructuredGridRepresentation* unstructuredGridRepresentation)
{
	std::vector<resqml2::AbstractValuesProperty*> valuesPropertySet = unstructuredGridRepresentation->getValuesPropertySet();

	const ULONG64 cellCount = unstructuredGridRepresentation->getCellCount();
	const ULONG64 pointCount = unstructuredGridRepresentation->getXyzPointCountOfAllPatches();
	return loadValuesPropertySet(valuesPropertySet,cellCount, pointCount);
}

//----------------------------------------------------------------------------
vtkDataArray* VtkProperty::visualize(const std::string & uuid, resqml2_0_1::WellboreTrajectoryRepresentation* wellboreTrajectoryRepresentation)
{
	std::vector<resqml2::AbstractValuesProperty*> valuesPropertySet = wellboreTrajectoryRepresentation->getValuesPropertySet();
	long pointCount = wellboreTrajectoryRepresentation->getXyzPointCountOfAllPatches();
	return loadValuesPropertySet(valuesPropertySet, 0, pointCount);
}

//----------------------------------------------------------------------------
vtkDataArray* VtkProperty::loadValuesPropertySet(std::vector<resqml2::AbstractValuesProperty*> valuesPropertySet, long cellCount, long pointCount)
{
	for (size_t i = 0; i < valuesPropertySet.size(); ++i) {
		if (valuesPropertySet[i]->getUuid() == getUuid()) {
			resqml2::AbstractValuesProperty* valuesProperty = valuesPropertySet[i];

			int nbElement = 0;
			gsoap_resqml2_0_1::resqml2__IndexableElements element = valuesProperty->getAttachmentKind();
			if (element == gsoap_resqml2_0_1::resqml2__IndexableElements::resqml2__IndexableElements__cells ) {
				nbElement = cellCount;
				support = typeSupport::CELLS;
			}
			else if (element == gsoap_resqml2_0_1::resqml2__IndexableElements::resqml2__IndexableElements__nodes ) {
				support = typeSupport::POINTS ;
				nbElement = pointCount;
			}
			else {
				vtkOutputWindowDisplayDebugText("property not supported...  (resqml2__IndexableElements: not cells or nodes)");
			}

			const std::string name = valuesProperty->getTitle();

			if (valuesProperty->getXmlTag() == "ContinuousProperty") {
				vtkSmartPointer<vtkDoubleArray> cellDataDouble = vtkSmartPointer<vtkDoubleArray>::New();

				if ( valuesProperty->getElementCountPerValue() == 1) {
					double* valuesDoubleSet = new double[nbElement*valuesProperty->getElementCountPerValue()];

					static_cast<resqml2_0_1::ContinuousProperty*>(valuesProperty)->getDoubleValuesOfPatch(0, valuesDoubleSet);
					cellDataDouble->SetName(name.c_str());
					cellDataDouble->SetArray(valuesDoubleSet, nbElement, 0);
					cellData = cellDataDouble;
				}
				else {
					cerr << "does not support vectorial property yet" << endl;
				}
			}
			else if (valuesProperty->getXmlTag() == "DiscreteProperty" || valuesProperty->getXmlTag() == "CategoricalProperty")	{
				vtkSmartPointer<vtkLongArray> cellDataLong = vtkSmartPointer<vtkLongArray>::New();

				if (valuesProperty->getElementCountPerValue() == 1) {
					long* valuesLongSet = new long[nbElement*valuesProperty->getElementCountPerValue()];
					valuesProperty->getLongValuesOfPatch(0, valuesLongSet);

					cellDataLong->SetName(name.c_str());
					cellDataLong->SetArray(valuesLongSet, nbElement, 0);
					cellData = cellDataLong;
				}
				else {
					cerr << "does not support vectorial property yet" << endl;
				}
			}
			else {
				cerr << "does not support property which are not discrete or categorical or continuous yet" << endl;
			}
		}
	}
	return cellData;
}

//----------------------------------------------------------------------------
vtkDataArray* VtkProperty::loadValuesPropertySet(std::vector<resqml2::AbstractValuesProperty*> valuesPropertySet, long cellCount, long pointCount,
		int iCellCount, int jCellCount, int kCellCount, int initKIndex)
{
	for (size_t i = 0; i < valuesPropertySet.size(); ++i) {
		if (valuesPropertySet[i]->getUuid() == getUuid()) {
			resqml2::AbstractValuesProperty* valuesProperty = valuesPropertySet[i];

			int nbElement = 0;
			gsoap_resqml2_0_1::resqml2__IndexableElements element = valuesPropertySet[i]->getAttachmentKind();
			if (element == gsoap_resqml2_0_1::resqml2__IndexableElements::resqml2__IndexableElements__cells ) {
				nbElement = cellCount;
				support = typeSupport::CELLS;
			}
			else if (element == gsoap_resqml2_0_1::resqml2__IndexableElements::resqml2__IndexableElements__nodes ) {
				support = typeSupport::POINTS ;
				nbElement = pointCount;
			}
			else
				vtkOutputWindowDisplayDebugText("property not supported...  (resqml2__IndexableElements: not cells or nodes)");

			auto typeProperty = valuesProperty->getXmlTag();

			unsigned long long numValuesInEachDimension = cellCount; //cellCount/kCellCount; //3834;//iCellCount*jCellCount*kCellCount;
			unsigned long long offsetInEachDimension = iCellCount*jCellCount*initKIndex; //initKIndex;//iCellCount*jCellCount*initKIndex;

			if (typeProperty == "ContinuousProperty") {
				vtkSmartPointer<vtkFloatArray> cellDataFloat = vtkSmartPointer<vtkFloatArray>::New();
				float* valuesFloatSet = new float[nbElement];
				resqml2_0_1::ContinuousProperty *propertyValue = static_cast<resqml2_0_1::ContinuousProperty*>(valuesPropertySet[i]);
				if (propertyValue->getElementCountPerValue() == 1) {
					if (propertyValue->getDimensionsCountOfPatch(0)==3) {
						propertyValue->getFloatValuesOf3dPatch(0, valuesFloatSet,iCellCount, jCellCount, kCellCount, 0, 0, initKIndex);
					} else if(propertyValue->getDimensionsCountOfPatch(0)==1) {
						propertyValue->getFloatValuesOfPatch(0, valuesFloatSet, &numValuesInEachDimension, &offsetInEachDimension, 1);
					} else {
						vtkOutputWindowDisplayDebugText("error in : propertyValue->getDimensionsCountOfPatch (values different of 1 or 3)");
					}
				}
				std::string name = valuesPropertySet[i]->getTitle();
				cellDataFloat->SetName(name.c_str());
				cellDataFloat->SetArray(valuesFloatSet, nbElement, 0);
				cellData = cellDataFloat;
			}
			else if(typeProperty == "DiscreteProperty") {
				vtkSmartPointer<vtkIntArray> cellDataInt = vtkSmartPointer<vtkIntArray>::New();
				int* valuesIntSet = new int[nbElement];
				resqml2_0_1::DiscreteProperty *propertyValue = static_cast<resqml2_0_1::DiscreteProperty*>(valuesPropertySet[i]);
				if (propertyValue->getElementCountPerValue() == 1) {
					if (propertyValue->getDimensionsCountOfPatch(0)==3) {
						propertyValue->getIntValuesOf3dPatch(0, valuesIntSet,iCellCount, jCellCount, kCellCount, 0, 0, initKIndex);
					}else if(propertyValue->getDimensionsCountOfPatch(0)==1) {
						propertyValue->getIntValuesOfPatch(0, valuesIntSet, &numValuesInEachDimension, &offsetInEachDimension, 1);
					}else {
						vtkOutputWindowDisplayDebugText("error in : propertyValue->getDimensionsCountOfPatch (values different of 1 or 3)");
					}
				}
				std::string name = valuesPropertySet[i]->getTitle();
				cellDataInt->SetName(name.c_str());
				cellDataInt->SetArray(valuesIntSet, nbElement, 0);
				cellData = cellDataInt;
			}
			else if (typeProperty == "CategoricalProperty") {
				vtkSmartPointer<vtkIntArray> cellDataInt = vtkSmartPointer<vtkIntArray>::New();
				int* valuesIntSet = new int[nbElement];
				resqml2_0_1::CategoricalProperty *propertyValue = static_cast<resqml2_0_1::CategoricalProperty*>(valuesPropertySet[i]);
				if (propertyValue->getElementCountPerValue() == 1) {
					if (propertyValue->getDimensionsCountOfPatch(0)==3) {
						propertyValue->getIntValuesOf3dPatch(0, valuesIntSet,iCellCount, jCellCount, kCellCount, 0, 0, initKIndex);
					}else if(propertyValue->getDimensionsCountOfPatch(0)==1) {
						propertyValue->getIntValuesOfPatch(0, valuesIntSet, &numValuesInEachDimension, &offsetInEachDimension, 1);
					}else
					{
						vtkOutputWindowDisplayDebugText("error in : propertyValue->getDimensionsCountOfPatch (values different of 1 or 3)");
					}

				}
				std::string name = valuesPropertySet[i]->getTitle();
				cellDataInt->SetName(name.c_str());
				cellDataInt->SetArray(valuesIntSet, nbElement, 0);
				cellData = cellDataInt;
			}
			else {
				vtkOutputWindowDisplayDebugText("property not supported...  (hdfDatatypeEnum)");
			}
		}
	}
	return cellData;
}
long VtkProperty::getAttachmentPropertyCount(const std::string & uuid, const VtkEpcCommon::FesppAttachmentProperty propertyUnit)
{
	return 0;
}
