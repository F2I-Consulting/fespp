#include "VtkResqml2MultiBlockDataSet.h"

//----------------------------------------------------------------------------
VtkResqml2MultiBlockDataSet::VtkResqml2MultiBlockDataSet(const std::string & fileName, const std::string & name, const std::string & uuid, const std::string & uuidParent, const int & idProc, const int & maxProc):
	VtkAbstractObject(fileName, name, uuid, uuidParent, idProc, maxProc)
{
	vtkOutput = vtkSmartPointer<vtkMultiBlockDataSet>::New();
}

//----------------------------------------------------------------------------
VtkResqml2MultiBlockDataSet::~VtkResqml2MultiBlockDataSet()
{
	vtkOutput = NULL;

	uuidIsChildOf.clear();

	attachUuids.clear();
}

//----------------------------------------------------------------------------
vtkSmartPointer<vtkMultiBlockDataSet> VtkResqml2MultiBlockDataSet::getOutput() const
{
	return vtkOutput;
}

//----------------------------------------------------------------------------
bool VtkResqml2MultiBlockDataSet::isEmpty()
{
	return attachUuids.empty();
}

//----------------------------------------------------------------------------
void VtkResqml2MultiBlockDataSet::detach()
{

	for (unsigned int blockIndex = 0; blockIndex < attachUuids.size() ; ++blockIndex)
	{
		vtkOutput->RemoveBlock(0);
	}

}



