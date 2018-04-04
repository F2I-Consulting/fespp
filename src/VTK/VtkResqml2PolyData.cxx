#include "VtkResqml2PolyData.h"

#include <common/EpcDocument.h>
#include <vtkPointData.h>

//----------------------------------------------------------------------------
VtkResqml2PolyData::VtkResqml2PolyData(const std::string & fileName, const std::string & name, const std::string & uuid, const std::string & uuidParent, common::EpcDocument *pckRep, common::EpcDocument *pckSubRep, const int & idProc, const int & maxProc) :
	VtkAbstractRepresentation(fileName, name, uuid, uuidParent, pckRep, pckSubRep, idProc, maxProc)
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
	else if(uuidToVtkProperty[uuid])
	{
		vtkOutput->GetPointData()->RemoveArray(0);
	}
}




