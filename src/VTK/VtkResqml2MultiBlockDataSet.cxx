#include "VtkResqml2MultiBlockDataSet.h"

//----------------------------------------------------------------------------
VtkResqml2MultiBlockDataSet::VtkResqml2MultiBlockDataSet(const std::string & fileName, const std::string & name, const std::string & uuid, const std::string & uuidParent):
	VtkAbstractObject(fileName, name, uuid, uuidParent)
{
	vtkOutput = vtkSmartPointer<vtkMultiBlockDataSet>::New();
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



