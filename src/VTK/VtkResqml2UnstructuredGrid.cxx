#include "VtkResqml2UnstructuredGrid.h"

#include <vtkCellData.h>

//----------------------------------------------------------------------------
VtkResqml2UnstructuredGrid::VtkResqml2UnstructuredGrid(const std::string & fileName, const std::string & name, const std::string & uuid, const std::string & uuidParent, common::EpcDocument *pckRep, common::EpcDocument *pckSubRep, const int & idProc, const int & maxProc) :
	VtkAbstractRepresentation(fileName, name, uuid, uuidParent, pckRep, pckSubRep, idProc, maxProc)
{
}

//----------------------------------------------------------------------------
VtkResqml2UnstructuredGrid::~VtkResqml2UnstructuredGrid()
{
	vtkOutput = NULL;
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
		vtkOutput = nullptr;
	}
	else if(uuidToVtkProperty[uuid])
	{
		vtkOutput->GetCellData()->RemoveArray(0);
	}
}

