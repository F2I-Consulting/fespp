/*-----------------------------------------------------------------------
Copyright F2I-CONSULTING, (2014)

cedric.robert@f2i-consulting.com

This software is a computer program whose purpose is to display data formatted using Energistics standards.

This software is governed by the CeCILL license under French law and
abiding by the rules of distribution of free software.  You can  use,
modify and/ or redistribute the software under the terms of the CeCILL
license as circulated by CEA, CNRS and INRIA at the following URL
"http://www.cecill.info".

As a counterpart to the access to the source code and  rights to copy,
modify and redistribute granted by the license, users are provided only
with a limited warranty  and the software's author,  the holder of the
economic rights,  and the successive licensors  have only  limited
liability.

In this respect, the user's attention is drawn to the risks associated
with loading,  using,  modifying and/or developing or reproducing the
software by the user in light of its specific status of free software,
that may mean  that it is complicated to manipulate,  and  that  also
therefore means  that it is reserved for developers  and  experienced
professionals having in-depth computer knowledge. Users are therefore
encouraged to load and test the software's suitability as regards their
requirements in conditions enabling the security of their systems and/or
data to be ensured and,  more generally, to use and operate it in the
same conditions as regards security.

The fact that you are presently reading this means that you have had
knowledge of the CeCILL license and that you accept its terms.
-----------------------------------------------------------------------*/
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

	for (const auto& vtkEpc : vtkEpcList) {
		delete vtkEpc;
	}
	vtkEpcList.clear();

	treeView.clear(); // Tree

	vtkOutput = NULL;

	procRank = 0;
	nbProc = 0;
}

//----------------------------------------------------------------------------
std::string VtkEpcDocumentSet::visualize(const std::string & uuid)
{
if(representationMode) {
		try
		{
			uuidToVtkEpc[uuid]->visualize(uuid);
		}
		catch  (const std::exception & e)
		{
			return "EXCEPTION in fesapi " + uuidToVtkEpc[uuid]->getFileName() + " : " + e.what();
		}
	}
return std::string();
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
VtkEpcCommon VtkEpcDocumentSet::getInfoUuid(std::string uuid)
{
	return  uuidToVtkEpc[uuid]->getInfoUuid(uuid);
}

//----------------------------------------------------------------------------
vtkSmartPointer<vtkMultiBlockDataSet> VtkEpcDocumentSet::getVisualization() const
{
if(representationMode) {
		vtkOutput->Initialize();
		unsigned int index = 0;
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
std::string VtkEpcDocumentSet::addEpcDocument(const std::string & fileName)
{
	if (std::find(vtkEpcNameList.begin(), vtkEpcNameList.end(),fileName)==vtkEpcNameList.end())	{
		auto vtkEpc = new VtkEpcDocument(fileName, procRank, nbProc, this);
		auto msg_error = vtkEpc->getError();
		auto uuidList = vtkEpc->getListUuid();
		for (auto &uuidListElem : uuidList) {
			uuidToVtkEpc[uuidListElem] = vtkEpc;
		}
		vtkEpcList.push_back(vtkEpc);
		vtkEpcNameList.push_back(fileName);

		auto tmpTree = vtkEpc->getTreeView();
		treeView.insert( treeView.end(), tmpTree.begin(), tmpTree.end() );

		return msg_error;
	}
	return std::string();
}

//----------------------------------------------------------------------------
VtkEpcDocument* VtkEpcDocumentSet::getVtkEpcDocument(const std::string & uuid)
{
	return uuidToVtkEpc.find(uuid) != uuidToVtkEpc.end() ? uuidToVtkEpc[uuid] : nullptr;
}

//----------------------------------------------------------------------------
VtkEpcCommon::Resqml2Type VtkEpcDocumentSet::getTypeInEpcDocument(const std::string & uuid)
{

	auto epcDoc = uuidToVtkEpc.find(uuid) != uuidToVtkEpc.end() ? uuidToVtkEpc[uuid] : nullptr;
	if (epcDoc!=nullptr) {
		return epcDoc->getType(uuid);
	}
	return VtkEpcCommon::UNKNOW;
}

//----------------------------------------------------------------------------
std::vector<VtkEpcCommon> VtkEpcDocumentSet::getTreeView() const
{
	return treeView;
}
