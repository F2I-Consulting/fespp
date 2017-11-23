#include "VtkResqml2StructuredGrid.h"

//----------------------------------------------------------------------------
VtkResqml2StructuredGrid::VtkResqml2StructuredGrid(const std::string & fileName, const std::string & name, const std::string & uuid, const std::string & uuidParent, common::EpcDocument *pckRep, common::EpcDocument *pckSubRep) :
	VtkAbstractRepresentation(fileName, name, uuid, uuidParent, pckRep, pckSubRep)
{
}

//----------------------------------------------------------------------------
vtkSmartPointer<vtkStructuredGrid> VtkResqml2StructuredGrid::getOutput() const
{
	return vtkOutput;
}

//----------------------------------------------------------------------------
void VtkResqml2StructuredGrid::remove(const std::string & uuid)
{
	if (uuid == getUuid())
	{
		vtkOutput = NULL;
	}
}
