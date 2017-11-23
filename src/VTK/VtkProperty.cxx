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
VtkProperty::VtkProperty(const std::string & fileName, const std::string & name, const std::string & uuid, const std::string & uuidParent, common::EpcDocument *pck) :
VtkAbstractObject(fileName, name, uuid, uuidParent), epcPackage(pck)
{
//	this->cellData = vtkSmartPointer<vtkDataArray>::New();
}

//----------------------------------------------------------------------------
void VtkProperty::visualize(const std::string & uuid)
{
}

//----------------------------------------------------------------------------
void VtkProperty::createTreeVtk(const std::string & uuid, const std::string & parent, const std::string & name, const Resqml2Type & resqmlType)
{
}

//----------------------------------------------------------------------------
void VtkProperty::remove(const std::string & uuid)
{
//	this->cellData->Reset();
// vtkSmartPointer<vtkDataArray> cellData = vtkSmartPointer<vtkDataArray>::New();
}

//----------------------------------------------------------------------------
//vtkDataArray* VtkProperty::getCellData() const
//{
//	return cellData;
//}

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
	for (size_t i = 0; i < valuesPropertySet.size(); ++i)
	{
		if (valuesPropertySet[i]->getUuid() == getUuid())
		{
			resqml2::AbstractValuesProperty* valuesProperty = valuesPropertySet[i];

			int nbElement = 0;
			gsoap_resqml2_0_1::resqml2__IndexableElements element = valuesPropertySet[i]->getAttachmentKind();
			if (element == gsoap_resqml2_0_1::resqml2__IndexableElements::resqml2__IndexableElements__cells )
			{
				nbElement = cellCount;
				support = typeSupport::CELLS;
			}
			else
				if (element == gsoap_resqml2_0_1::resqml2__IndexableElements::resqml2__IndexableElements__nodes )
				{
					support = typeSupport::POINTS ;
					nbElement = pointCount;
				}
				else
				vtkOutputWindowDisplayDebugText("property not supported...  (resqml2__IndexableElements: not cells or nodes)");

			resqml2::AbstractValuesProperty::hdfDatatypeEnum hdfDatatype = valuesPropertySet[i]->getValuesHdfDatatype();

			switch (hdfDatatype)
			{
			case resqml2::AbstractValuesProperty::hdfDatatypeEnum::UNKNOWN :
				{
					break;
				}
			case resqml2::AbstractValuesProperty::hdfDatatypeEnum::DOUBLE :
				{	 
					vtkSmartPointer<vtkDoubleArray> cellDataDouble = vtkSmartPointer<vtkDoubleArray>::New();
					double* valuesDoubleSet = new double[nbElement];

					if (valuesProperty->getXmlTag() == "ContinuousProperty")
					{
						resqml2_0_1::ContinuousProperty *propertyValue = static_cast<resqml2_0_1::ContinuousProperty*>(valuesPropertySet[i]);
						if ( propertyValue->getElementCountPerValue() == 1)
						{
							propertyValue->getDoubleValuesOfPatch(0, valuesDoubleSet);
						}
					}
					else if (valuesProperty->getXmlTag() == "DiscreteProperty")
					{
						;
					}
					else if (valuesProperty->getXmlTag() == "CategoricalProperty")
					{
						;
					}

					std::string name = valuesPropertySet[i]->getTitle();
					cellDataDouble->SetName(name.c_str());
					cellDataDouble->SetArray(valuesDoubleSet, nbElement, 0);
					cellData = cellDataDouble;
//					delete[] valuesDoubleSet;
					break;
				} 
			case resqml2::AbstractValuesProperty::hdfDatatypeEnum::FLOAT :
				{	 
					vtkSmartPointer<vtkFloatArray> cellDataFloat = vtkSmartPointer<vtkFloatArray>::New();
					float* valuesFloatSet = new float[nbElement];

					if (valuesProperty->getXmlTag() == "ContinuousProperty")
					{
						resqml2_0_1::ContinuousProperty *propertyValue = static_cast<resqml2_0_1::ContinuousProperty*>(valuesPropertySet[i]);
						if (propertyValue->getElementCountPerValue() == 1)
						{
							propertyValue->getFloatValuesOfPatch(0, valuesFloatSet);
						}
					}
					else if (valuesProperty->getXmlTag() == "DiscreteProperty")
					{
						;
					}
					else if (valuesProperty->getXmlTag() == "CategoricalProperty")
					{
						;
					}

					std::string name = valuesPropertySet[i]->getTitle();
					cellDataFloat->SetName(name.c_str());
					cellDataFloat->SetArray(valuesFloatSet, nbElement, 0);
					cellData = cellDataFloat;
//					delete[] valuesFloatSet;
					break;
			}
			case resqml2::AbstractValuesProperty::hdfDatatypeEnum::LONG :
				{	 
					vtkSmartPointer<vtkLongArray> cellDataLong = vtkSmartPointer<vtkLongArray>::New();
					long* valuesLongSet = new long[nbElement];

					if (valuesProperty->getXmlTag() == "ContinuousProperty")
					{
						;
					}
					else if (valuesProperty->getXmlTag() == "DiscreteProperty")
					{
						resqml2_0_1::DiscreteProperty *propertyValue = static_cast<resqml2_0_1::DiscreteProperty*>(valuesPropertySet[i]);
						if (propertyValue->getElementCountPerValue() == 1)
						{
							propertyValue->getLongValuesOfPatch(0, valuesLongSet);
						}
					}
					else if (valuesProperty->getXmlTag() == "CategoricalProperty")
					{
						resqml2_0_1::CategoricalProperty *propertyValue = static_cast<resqml2_0_1::CategoricalProperty*>(valuesPropertySet[i]);
						if (propertyValue->getElementCountPerValue() == 1)
						{
							propertyValue->getLongValuesOfPatch(0, valuesLongSet);
						}
					}

					std::string name = valuesPropertySet[i]->getTitle();
					cellDataLong->SetName(name.c_str());
					cellDataLong->SetArray(valuesLongSet, nbElement, 0);
					cellData = cellDataLong;
//					delete[] valuesLongSet;
					break;
				}
			case resqml2::AbstractValuesProperty::hdfDatatypeEnum::ULONG :
				{	 
					vtkSmartPointer<vtkUnsignedLongArray> cellDataLong = vtkSmartPointer<vtkUnsignedLongArray>::New();
					unsigned long* valuesULongSet = new unsigned long[nbElement];

					if (valuesProperty->getXmlTag() == "ContinuousProperty")
					{
						;
					}
					else if (valuesProperty->getXmlTag() == "DiscreteProperty")
					{
						resqml2_0_1::DiscreteProperty *propertyValue = static_cast<resqml2_0_1::DiscreteProperty*>(valuesPropertySet[i]);
						if (propertyValue->getElementCountPerValue() == 1)
						{
							propertyValue->getULongValuesOfPatch(0, valuesULongSet);
						}
					}
					else if (valuesProperty->getXmlTag() == "CategoricalProperty")
					{
						resqml2_0_1::CategoricalProperty *propertyValue = static_cast<resqml2_0_1::CategoricalProperty*>(valuesPropertySet[i]);
						if (propertyValue->getElementCountPerValue() == 1)
						{
							propertyValue->getULongValuesOfPatch(0, valuesULongSet);
						}
					}

					std::string name = valuesPropertySet[i]->getTitle();
					cellDataLong->SetName(name.c_str());
					cellDataLong->SetArray(valuesULongSet, nbElement, 0);
					cellData = cellDataLong;
//					delete[] valuesULongSet;
					break;
			}
			case resqml2::AbstractValuesProperty::hdfDatatypeEnum::INT :
				{	 
					vtkSmartPointer<vtkIntArray> cellDataInt = vtkSmartPointer<vtkIntArray>::New();
					int* valuesIntSet = new int[nbElement];

					if (valuesProperty->getXmlTag() == "ContinuousProperty")
					{
						;
					}
					else if (valuesProperty->getXmlTag() == "DiscreteProperty")
					{
						resqml2_0_1::DiscreteProperty *propertyValue = static_cast<resqml2_0_1::DiscreteProperty*>(valuesPropertySet[i]);
						if (propertyValue->getElementCountPerValue() == 1)
						{
							propertyValue->getIntValuesOfPatch(0, valuesIntSet);
						}
					}
					else if (valuesProperty->getXmlTag() == "CategoricalProperty")
					{
						resqml2_0_1::CategoricalProperty *propertyValue = static_cast<resqml2_0_1::CategoricalProperty*>(valuesPropertySet[i]);
						if (propertyValue->getElementCountPerValue() == 1)
						{
							propertyValue->getIntValuesOfPatch(0, valuesIntSet);
						}
					}

					std::string name = valuesPropertySet[i]->getTitle();
					cellDataInt->SetName(name.c_str());
					cellDataInt->SetArray(valuesIntSet, nbElement, 0);
					cellData = cellDataInt;
//					delete[] valuesIntSet;
					break;
			}
			case resqml2::AbstractValuesProperty::hdfDatatypeEnum::UINT :
				{	 
					vtkSmartPointer<vtkUnsignedIntArray> cellDataUInt = vtkSmartPointer<vtkUnsignedIntArray>::New();
					unsigned int* valuesUIntSet = new unsigned int[nbElement];

					if (valuesProperty->getXmlTag() == "ContinuousProperty")
					{
						;
					}
					else if (valuesProperty->getXmlTag() == "DiscreteProperty")
					{
						resqml2_0_1::DiscreteProperty *propertyValue = static_cast<resqml2_0_1::DiscreteProperty*>(valuesPropertySet[i]);
						if (propertyValue->getElementCountPerValue() == 1)
						{
							propertyValue->getUIntValuesOfPatch(0, valuesUIntSet);
						}
					}
					else if (valuesProperty->getXmlTag() == "CategoricalProperty")
					{
						resqml2_0_1::CategoricalProperty *propertyValue = static_cast<resqml2_0_1::CategoricalProperty*>(valuesPropertySet[i]);
						if (propertyValue->getElementCountPerValue() == 1)
						{
							propertyValue->getUIntValuesOfPatch(0, valuesUIntSet);
						}
					}

					std::string name = valuesPropertySet[i]->getTitle();
					cellDataUInt->SetName(name.c_str());
					cellDataUInt->SetArray(valuesUIntSet, nbElement, 0);
					cellData = cellDataUInt;
//					delete[] valuesUIntSet;
					break;
				}																		
			case resqml2::AbstractValuesProperty::hdfDatatypeEnum::SHORT :
				{	 
					vtkSmartPointer<vtkShortArray> cellDataShort = vtkSmartPointer<vtkShortArray>::New();
					short* valuesShortSet = new short[nbElement];

					if (valuesProperty->getXmlTag() == "ContinuousProperty")
					{
						;
					}
					else if (valuesProperty->getXmlTag() == "DiscreteProperty")
					{
						resqml2_0_1::DiscreteProperty *propertyValue = static_cast<resqml2_0_1::DiscreteProperty*>(valuesPropertySet[i]);
						if (propertyValue->getElementCountPerValue() == 1)
						{
							propertyValue->getShortValuesOfPatch(0, valuesShortSet);
						}
					}
					else if (valuesProperty->getXmlTag() == "CategoricalProperty")
					{
						resqml2_0_1::CategoricalProperty *propertyValue = static_cast<resqml2_0_1::CategoricalProperty*>(valuesPropertySet[i]);
						if (propertyValue->getElementCountPerValue() == 1)
						{
							propertyValue->getShortValuesOfPatch(0, valuesShortSet);
						}
					}

					std::string name = valuesPropertySet[i]->getTitle();
					cellDataShort->SetName(name.c_str());
					cellDataShort->SetArray(valuesShortSet, nbElement, 0);
					cellData = cellDataShort;
//					delete[] valuesShortSet;
					break;
				}																		
			case resqml2::AbstractValuesProperty::hdfDatatypeEnum::USHORT :
				{	 
					vtkSmartPointer<vtkUnsignedShortArray> cellDataUShort = vtkSmartPointer<vtkUnsignedShortArray>::New();
					unsigned short* valuesUShortSet = new unsigned short[nbElement];

					if (valuesProperty->getXmlTag() == "ContinuousProperty")
					{
						;
					}
					else if (valuesProperty->getXmlTag() == "DiscreteProperty")
					{
						resqml2_0_1::DiscreteProperty *propertyValue = static_cast<resqml2_0_1::DiscreteProperty*>(valuesPropertySet[i]);
						if (propertyValue->getElementCountPerValue() == 1)
						{
							propertyValue->getUShortValuesOfPatch(0, valuesUShortSet);
						}
					}
					else if (valuesProperty->getXmlTag() == "CategoricalProperty")
					{
						resqml2_0_1::CategoricalProperty *propertyValue = static_cast<resqml2_0_1::CategoricalProperty*>(valuesPropertySet[i]);
						if (propertyValue->getElementCountPerValue() == 1)
						{
							propertyValue->getUShortValuesOfPatch(0, valuesUShortSet);
						}
					}

					std::string name = valuesPropertySet[i]->getTitle();
					cellDataUShort->SetName(name.c_str());
					cellDataUShort->SetArray(valuesUShortSet, nbElement, 0);
					cellData = cellDataUShort;
//					delete[] valuesUShortSet;
					break;
				}																		
			case resqml2::AbstractValuesProperty::hdfDatatypeEnum::CHAR :
				{	 
					vtkSmartPointer<vtkCharArray> cellDataChar = vtkSmartPointer<vtkCharArray>::New();
					char* valuesCharSet = new char[nbElement];

					if (valuesProperty->getXmlTag() == "ContinuousProperty")
					{
						;
					}
					else if (valuesProperty->getXmlTag() == "DiscreteProperty")
					{
						resqml2_0_1::DiscreteProperty *propertyValue = static_cast<resqml2_0_1::DiscreteProperty*>(valuesPropertySet[i]);
						if (propertyValue->getElementCountPerValue() == 1)
						{
							propertyValue->getCharValuesOfPatch(0, valuesCharSet);
						}
					}
					else if (valuesProperty->getXmlTag() == "CategoricalProperty")
					{
						resqml2_0_1::CategoricalProperty *propertyValue = static_cast<resqml2_0_1::CategoricalProperty*>(valuesPropertySet[i]);
						if (propertyValue->getElementCountPerValue() == 1)
						{
							propertyValue->getCharValuesOfPatch(0, valuesCharSet);
						}
					}

					std::string name = valuesPropertySet[i]->getTitle();
					cellDataChar->SetName(name.c_str());
					cellDataChar->SetArray(valuesCharSet, nbElement, 0);
					cellData = cellDataChar;
//					delete[] valuesCharSet;
					break;
				}																		
			case resqml2::AbstractValuesProperty::hdfDatatypeEnum::UCHAR :
				{	 
					vtkSmartPointer<vtkUnsignedCharArray> cellDataUInt = vtkSmartPointer<vtkUnsignedCharArray>::New();
					unsigned char* valuesUCharSet = new unsigned char[nbElement];

					if (valuesProperty->getXmlTag() == "ContinuousProperty")
					{
						;
					}
					else if (valuesProperty->getXmlTag() == "DiscreteProperty")
					{
						resqml2_0_1::DiscreteProperty *propertyValue = static_cast<resqml2_0_1::DiscreteProperty*>(valuesPropertySet[i]);
						if (propertyValue->getElementCountPerValue() == 1)
						{
							propertyValue->getUCharValuesOfPatch(0, valuesUCharSet);
						}
					}
					else if (valuesProperty->getXmlTag() == "CategoricalProperty")
					{
						resqml2_0_1::CategoricalProperty *propertyValue = static_cast<resqml2_0_1::CategoricalProperty*>(valuesPropertySet[i]);
						if (propertyValue->getElementCountPerValue() == 1)
						{
							propertyValue->getUCharValuesOfPatch(0, valuesUCharSet);
						}
					}

					std::string name = valuesPropertySet[i]->getTitle();
					cellDataUInt->SetName(name.c_str());
					cellDataUInt->SetArray(valuesUCharSet, nbElement, 0);
					cellData = cellDataUInt;
//					delete[] valuesUCharSet;
					break;
				}
			default:
				vtkOutputWindowDisplayDebugText("property not supported...  (hdfDatatypeEnum)");
				break;			
			}
		}
	}
	return cellData;
}

long VtkProperty::getAttachmentPropertyCount(const std::string & uuid, const FesppAttachmentProperty propertyUnit)
{
	return 0;
}
