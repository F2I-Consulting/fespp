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

#ifndef __VtkEpcDocument_h
#define __VtkEpcDocument_h

#include <vtkDataArray.h>

#include "VtkResqml2MultiBlockDataSet.h"
#include "VtkEpcDocumentSet.h"

class VtkIjkGridRepresentation;
class VtkUnstructuredGridRepresentation;
class VtkPartialRepresentation;
class VtkGrid2DRepresentation;
class VtkPolylineRepresentation;
class VtkTriangulatedRepresentation;
class VtkSetPatch;
class VtkWellboreTrajectoryRepresentation;

namespace common
{
	class EpcDocument;
}

class VtkEpcDocument : public VtkResqml2MultiBlockDataSet
{

public:
	/**
	* Constructor
	*/
	VtkEpcDocument (const std::string & fileName, const int & idProc=0, const int & maxProc=0, VtkEpcDocumentSet * epcDocSet=nullptr);
	/**
	* Destructor
	*/
	~VtkEpcDocument();

	/**
	* method : visualize
	* variable : std::string uuid 
	* create uuid representation.
	*/
	void visualize(const std::string & uuid);
	

	/**
	* method : remove
	* variable : std::string uuid 
	* delete uuid representation.
	*/
	void remove(const std::string & uuid);
	
	/**
	* method : get TreeView
	* variable :
	*
	* if timeIndex = -1 then no time series link.
	*/
	std::vector<VtkEpcCommon*> getTreeView() const;

	/**
	* method : attach
	* variable : --
	*/
	void attach();

	std::vector<std::string> getListUuid();

	void addProperty(const std::string & uuidProperty, vtkDataArray* dataProperty);

	VtkEpcCommon::Resqml2Type getType(std::string);
	VtkEpcCommon* getInfoUuid(std::string);

	long getAttachmentPropertyCount(const std::string & uuid, const VtkEpcCommon::FesppAttachmentProperty propertyUnit) ;
	int getICellCount(const std::string & uuid) ;
	int getJCellCount(const std::string & uuid) ;
	int getKCellCount(const std::string & uuid) ;
	int getInitKIndex(const std::string & uuid) ;

	common::EpcDocument *getEpcDocument();
	
protected:

private:
	void addGrid2DTreeVtk(const std::string & uuid, const std::string & parent, const std::string & name);
	void addPolylineSetTreeVtk(const std::string & uuid, const std::string & parent, const std::string & name);
	void addTriangulatedSetTreeVtk(const std::string & uuid, const std::string & parent, const std::string & name);
	void addWellTrajTreeVtk(const std::string & uuid, const std::string & parent, const std::string & name);
	void addIjkGridTreeVtk(const std::string & uuid, const std::string & parent, const std::string & name);
	void addUnstrucGridTreeVtk(const std::string & uuid, const std::string & parent, const std::string & name);
	void addSubRepTreeVtk(const std::string & uuid, const std::string & parent, const std::string & name);
	void addPropertyTreeVtk(const std::string & uuid, const std::string & parent, const std::string & name);
	
	/**
	* method : createTreeVtkPartialRep
	* variable : uuid, VtkEpcDocument with complete representation
	* prepare VtkEpcDocument & Children.
	*/
	void createTreeVtkPartialRep(const std::string & uuid, /*const std::string & parent,*/ VtkEpcDocument *vtkEpcDowumentWithCompleteRep);

	/**
	* method : createTreeVtk
	* variable : uuid, parent uuid, name, type
	* prepare VtkEpcDocument & Children.
	*/
	void createTreeVtk(const std::string & uuid, const std::string & parent, const std::string & name, const VtkEpcCommon::Resqml2Type & resqmlType);



	// EPC DOCUMENT
	common::EpcDocument *epcPackage;

#if _MSC_VER < 1600
	//std::tr1::unordered_map<std::string, VtkFeature*> uuidToVtKFeature;	
	std::tr1::unordered_map<std::string, VtkGrid2DRepresentation*> uuidToVtkGrid2DRepresentation;
	std::tr1::unordered_map<std::string, VtkPolylineRepresentation*> uuidToVtkPolylineRepresentation;
	std::tr1::unordered_map<std::string, VtkTriangulatedRepresentation*> uuidToVtkTriangulatedRepresentation;
	std::tr1::unordered_map<std::string, VtkSetPatch*> uuidToVtkSetPatch;
	std::tr1::unordered_map<std::string, VtkWellboreTrajectoryRepresentation*> uuidToVtkWellboreTrajectoryRepresentation;
	std::tr1::unordered_map<std::string, VtkIjkGridRepresentation*> uuidToVtkIjkGridRepresentation;
	std::tr1::unordered_map<std::string, VtkUnstructuredGridRepresentation*> uuidToVtkUnstructuredGridRepresentation;
	std::tr1::unordered_map<std::string, VtkPartialRepresentation*> uuidToVtkPartialRepresentation;
#else
	//std::unordered_map<std::string, VtkFeature*> uuidToVtKFeature;	
	std::unordered_map<std::string, VtkGrid2DRepresentation*> uuidToVtkGrid2DRepresentation;
	std::unordered_map<std::string, VtkPolylineRepresentation*> uuidToVtkPolylineRepresentation;
	std::unordered_map<std::string, VtkTriangulatedRepresentation*> uuidToVtkTriangulatedRepresentation;
	std::unordered_map<std::string, VtkSetPatch*> uuidToVtkSetPatch;
	std::unordered_map<std::string, VtkWellboreTrajectoryRepresentation*> uuidToVtkWellboreTrajectoryRepresentation;
	std::unordered_map<std::string, VtkIjkGridRepresentation*> uuidToVtkIjkGridRepresentation;
	std::unordered_map<std::string, VtkUnstructuredGridRepresentation*> uuidToVtkUnstructuredGridRepresentation;
	std::unordered_map<std::string, VtkPartialRepresentation*> uuidToVtkPartialRepresentation;
#endif

	std::vector<std::string> uuidPartialRep;
	std::vector<std::string> uuidRep;

	VtkEpcDocumentSet * epcSet;

	std::vector<VtkEpcCommon*> treeView; // Tree
};
#endif
