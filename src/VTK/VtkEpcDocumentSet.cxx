#include "VtkEpcDocumentSet.h"
#include "VtkEpcDocument.h"

// include Vtk
#include <vtkInformation.h>
#include <vtkSmartPointer.h>
#include <vtkMultiBlockDataSet.h>

// include system
#include <algorithm>
#include <sstream>


//----------------------------------------------------------------------------
VtkEpcDocumentSet::VtkEpcDocumentSet(const int & idProc, const int & maxProc, const VtkEpcCommon::modeVtkEpc & mode) :
procRank(idProc), nbProc(maxProc)
{
	treeViewMode = (mode==VtkEpcCommon::Both || mode==VtkEpcCommon::TreeView);
	representationMode = (mode==VtkEpcCommon::Both || mode==VtkEpcCommon::Representation);

	vtkOutput = vtkSmartPointer<vtkMultiBlockDataSet>::New();
	treeView = {};
}

//----------------------------------------------------------------------------
VtkEpcDocumentSet::~VtkEpcDocumentSet()
{
	vtkEpcNameList.clear();
	uuidToVtkEpc.clear();
	vtkEpcList.clear();
	treeView.clear(); // Tree

	vtkOutput = NULL;

	procRank = 0;
	nbProc = 0;
}

//----------------------------------------------------------------------------
void VtkEpcDocumentSet::visualize(const std::string & uuid)
{
	if(representationMode) {
		uuidToVtkEpc[uuid]->visualize(uuid);
	}
}

//----------------------------------------------------------------------------
void VtkEpcDocumentSet::visualizeFull()
{
	if(representationMode) {
		for (auto &vtkEpcElem : vtkEpcList) {
			auto uuidList = vtkEpcElem->getListUuid();
			for (auto &uuidListElem : uuidList)	{
				vtkEpcElem->visualize(uuidListElem);
			}
		}
	}
}

//----------------------------------------------------------------------------
void VtkEpcDocumentSet::unvisualize(const std::string & uuid)
{
	if(representationMode) {
		uuidToVtkEpc[uuid]->remove(uuid);
	}
}

//----------------------------------------------------------------------------
VtkEpcCommon::Resqml2Type VtkEpcDocumentSet::getType(std::string uuid)
{
	return uuidToVtkEpc[uuid]->getType(uuid);
}

//----------------------------------------------------------------------------
VtkEpcCommon* VtkEpcDocumentSet::getInfoUuid(std::string uuid)
{
	return  uuidToVtkEpc[uuid]->getInfoUuid(uuid);
}

//----------------------------------------------------------------------------
vtkSmartPointer<vtkMultiBlockDataSet> VtkEpcDocumentSet::getVisualization() const
{
	if(representationMode) {
		vtkOutput->Initialize();
		auto index = 0;
		for (auto &vtkEpcElem : vtkEpcList) {
			if(vtkEpcElem->getOutput()->GetNumberOfBlocks()>0) {
				vtkOutput->SetBlock(index, vtkEpcElem->getOutput());
				vtkOutput->GetMetaData(index++)->Set(vtkCompositeDataSet::NAME(), vtkEpcElem->getFileName().c_str());
			}
		}
	}
	return vtkOutput;
}

//----------------------------------------------------------------------------
void VtkEpcDocumentSet::addEpcDocument(const std::string & fileName)
{
	if (std::find(vtkEpcNameList.begin(), vtkEpcNameList.end(),fileName)==vtkEpcNameList.end())	{
		auto vtkEpc = new VtkEpcDocument(fileName, procRank, nbProc, this);
		auto uuidList = vtkEpc->getListUuid();
		for (auto &uuidListElem : uuidList) {
			uuidToVtkEpc[uuidListElem] = vtkEpc;
		}
		vtkEpcList.push_back(vtkEpc);
		vtkEpcNameList.push_back(fileName);

		auto tmpTree = vtkEpc->getTreeView();
		treeView.insert( treeView.end(), tmpTree.begin(), tmpTree.end() );
	}
}

VtkEpcDocument* VtkEpcDocumentSet::getVtkEpcDocument(const std::string & uuid)
{
	if (uuidToVtkEpc[uuid])
		return uuidToVtkEpc[uuid];
	return nullptr;
}

//----------------------------------------------------------------------------
std::vector<VtkEpcCommon*> VtkEpcDocumentSet::getTreeView() const
{
	return treeView;
}
