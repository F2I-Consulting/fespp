#include "VtkResqml2PolyData.h"

#include <EpcDocument.h>

//----------------------------------------------------------------------------
VtkResqml2PolyData::VtkResqml2PolyData(const std::string & fileName, const std::string & name, const std::string & uuid, const std::string & uuidParent, common::EpcDocument *pckRep, common::EpcDocument *pckSubRep) :
	VtkAbstractRepresentation(fileName, name, uuid, uuidParent, pckRep, pckSubRep)
{
}

//----------------------------------------------------------------------------
vtkSmartPointer<vtkPolyData> VtkResqml2PolyData::getOutput() const
{
	return vtkOutput;
}

//----------------------------------------------------------------------------
void VtkResqml2PolyData::remove(const std::string & uuid)
{
	if (uuid == getUuid())
	{
		vtkOutput = NULL;
	}
}




