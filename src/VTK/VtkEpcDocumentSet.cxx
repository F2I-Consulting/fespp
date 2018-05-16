#include "VtkEpcDocumentSet.h"

// include Vtk
#include <vtkInformation.h>

// include system
#include <algorithm>
#include <sstream>


//----------------------------------------------------------------------------
VtkEpcDocumentSet::VtkEpcDocumentSet(const int & idProc, const int & maxProc, bool tree, bool visu) :
procRank(idProc), nbProc(maxProc), 	treeRep(tree), visualization(visu)
{
	vtkOutput = vtkSmartPointer<vtkMultiBlockDataSet>::New();
}

//----------------------------------------------------------------------------
VtkEpcDocumentSet::~VtkEpcDocumentSet()
{
	vtkEpcNameList.clear();
	uuidToVtkEpc.clear();

	for (std::vector< VtkEpcDocument* >::const_iterator it = vtkEpcList.begin() ; it != vtkEpcList.end(); ++it)
   {
     delete (*it);
   }
	vtkEpcList.clear();

	treeUuid.clear(); // Tree

	vtkOutput = NULL;

	int procRank = 0;
	int nbProc = 0;
}

//----------------------------------------------------------------------------
void VtkEpcDocumentSet::visualize(const std::string & uuid)
{
	if(visualization)
	{
		uuidToVtkEpc[uuid]->visualize(uuid);
	}
}

//----------------------------------------------------------------------------
void VtkEpcDocumentSet::visualizeFull()
{
	if(visualization)
	{
		for (auto &vtkEpcElem : vtkEpcList)
		{
			auto uuidList = vtkEpcElem->getUuid();
			for (auto &uuidListElem : uuidList)
			{
				vtkEpcElem->visualize(uuidListElem);
			}
		}
	}
}

//----------------------------------------------------------------------------
void VtkEpcDocumentSet::unvisualize(const std::string & uuid)
{
	if(visualization)
	{
		uuidToVtkEpc[uuid]->remove(uuid);
	}
}

//----------------------------------------------------------------------------
VtkAbstractObject::Resqml2Type VtkEpcDocumentSet::getType(std::string uuid)
{
	return uuidToVtkEpc[uuid]->getType(uuid);
}

//----------------------------------------------------------------------------
vtkSmartPointer<vtkMultiBlockDataSet> VtkEpcDocumentSet::getVisualization() const
{

	vtkOutput->Initialize();
	auto index = 0;
	for (auto i=0; i < vtkEpcList.size(); ++i)
	{
		if(vtkEpcList[i]->getOutput()->GetNumberOfBlocks()>0)
		{
			vtkOutput->SetBlock(index, vtkEpcList[i]->getOutput());
			vtkOutput->GetMetaData(index++)->Set(vtkCompositeDataSet::NAME(), vtkEpcList[i]->getFileName().c_str());
		}
	}
	return vtkOutput;
}

//----------------------------------------------------------------------------
void VtkEpcDocumentSet::addEpcDocument(const std::string & fileName)
{
	if (std::find(vtkEpcNameList.begin(), vtkEpcNameList.end(),fileName)==vtkEpcNameList.end())
	{
		auto vtkEpc = new VtkEpcDocument(fileName, procRank, nbProc, this);
		auto uuidList = vtkEpc->getUuid();
		for (auto &uuidListElem : uuidList)
		{
			uuidToVtkEpc[uuidListElem] = vtkEpc;
		}
		vtkEpcList.push_back(vtkEpc);
		vtkEpcNameList.push_back(fileName);
	}
}

VtkEpcDocument* VtkEpcDocumentSet::getVtkEpcDocument(const std::string & uuid)
{
	if (uuidToVtkEpc[uuid])
		return uuidToVtkEpc[uuid];
	return nullptr;
}
