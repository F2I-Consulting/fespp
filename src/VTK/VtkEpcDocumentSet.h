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

#ifndef __VtkEpcDocumentSet_h
#define __VtkEpcDocumentSet_h

#include "VtkEpcTools.h"

// include VTK
#include <vtkSmartPointer.h>
#include <vtkMultiBlockDataSet.h>

#include <vector>
// include system
#if (defined(_WIN32) && _MSC_VER >= 1600)
	#include <unordered_map>
#else
	#include <tr1/unordered_map>
#endif

class VtkEpcDocument;

class VtkEpcDocumentSet
{

public:

	/**
	* Constructor
	*/
	VtkEpcDocumentSet (const int & idProc=0, const int & maxProc=0, const VtkEpcTools::modeVtkEpc & mode=VtkEpcTools::Both);
	/**
	* Destructor
	*/
	~VtkEpcDocumentSet();

	/**
	* method : visualize
	* variable : std::string uuid 
	* create uuid representation.
	*/
	void visualize(const std::string & uuid);
	void visualizeFull();
	
	/**
	* method : remove
	* variable : std::string uuid 
	* delete uuid representation.
	*/
	void unvisualize(const std::string & uuid);

	VtkEpcTools::Resqml2Type getType(std::string uuid);

	/**
	* method : getOutput
	* variable : --
	* return the vtkMultiBlockDataSet for each epcdocument.
	*/
	vtkSmartPointer<vtkMultiBlockDataSet> getVisualization() const;
	std::vector<VtkEpcTools::infoUuid> getTreeView() const;

	void addEpcDocument(const std::string & fileName);

	VtkEpcDocument* getVtkEpcDocument(const std::string & uuid);


protected:

private:

	std::vector<VtkEpcDocument*> vtkEpcList;
	std::vector<std::string> vtkEpcNameList;

#if _MSC_VER < 1600
	std::tr1::unordered_map<std::string, VtkEpcDocument*> uuidToVtkEpc; // link uuid/VtkEpcdocument
#else
	std::unordered_map<std::string, VtkEpcDocument*> uuidToVtkEpc; // link uuid/VtkEpcdocument
#endif

	std::vector<VtkEpcTools::infoUuid> treeView; // Tree

	vtkSmartPointer<vtkMultiBlockDataSet> vtkOutput;

	int procRank;
	int nbProc;

	bool treeViewMode;
	bool representationMode;

};
#endif
