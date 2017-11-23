#include "VtkResqml2UnstructuredGrid.h"

//----------------------------------------------------------------------------
VtkResqml2UnstructuredGrid::VtkResqml2UnstructuredGrid(const std::string & fileName, const std::string & name, const std::string & uuid, const std::string & uuidParent, common::EpcDocument *pckRep, common::EpcDocument *pckSubRep) :
	VtkAbstractRepresentation(fileName, name, uuid, uuidParent, pckRep, pckSubRep)
{
}

//----------------------------------------------------------------------------
vtkSmartPointer<vtkUnstructuredGrid> VtkResqml2UnstructuredGrid::getOutput() const
{
	return vtkOutput;
}

//----------------------------------------------------------------------------
void VtkResqml2UnstructuredGrid::remove(const std::string & uuid)
{
	if (uuid == getUuid())
	{
		vtkOutput = NULL;
	}
}

