#include "VtkWellboreTrajectoryRepresentationText.h"

// include F2i-consulting Energistics Standards API 
#include <EpcDocument.h>

// include F2i-consulting Energistics Standards ParaView Plugin
#include "VtkResqml2PolyData.h"

//----------------------------------------------------------------------------
VtkWellboreTrajectoryRepresentationText::VtkWellboreTrajectoryRepresentationText(const std::string & fileName, const std::string & name, const std::string & uuid, const std::string & uuidParent, common::EpcDocument *pckRep, common::EpcDocument *pckSubRep) :
VtkResqml2PolyData(fileName, name, uuid, uuidParent, pckRep, pckSubRep)
{
}

//----------------------------------------------------------------------------
void VtkWellboreTrajectoryRepresentationText::createOutput(const std::string & uuid)
{
}

//----------------------------------------------------------------------------
void VtkWellboreTrajectoryRepresentationText::addProperty(const std::string uuidProperty, vtkDataArray* dataProperty)
{
}

long VtkWellboreTrajectoryRepresentationText::getAttachmentPropertyCount(const std::string & uuid, const FesppAttachmentProperty propertyUnit)
{
	return 0;
}


