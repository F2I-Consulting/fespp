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

#ifndef __VtkSetPatch_h
#define __VtkSetPatch_h

// include system
#include <unordered_map>

#include <vtkDataArray.h>

#include "VtkResqml2MultiBlockDataSet.h"

class VtkPolylineRepresentation;
class VtkTriangulatedRepresentation;
class VtkProperty;

namespace common
{
	class EpcDocument;
}

class VtkSetPatch : public VtkResqml2MultiBlockDataSet
{

public:
	/**
	* Constructor
	*/
	VtkSetPatch (const std::string & fileName, const std::string & name, const std:: string & uuid, const std::string & uuidParent, common::EpcDocument *pckEPC, const int & idProc=0, const int & maxProc=0);
	
	/**
	* Destructor
	*/
	~VtkSetPatch();

	/**
	* method : createTreeVtk
	* variable : std::string uuid, std::string parent, std::string name, Resqml2Type resqml Type
	* prepare the vtk object for represent the set representation.
	*/
	void createTreeVtk(const std::string & uuid, const std::string & parent, const std::string & name, const VtkEpcCommon::Resqml2Type & resqmlType);

	/**
	* method : visualize
	* variable : std::string uuid 
	* create the vtk objects.
	*/
	void visualize(const std::string & uuid);

	/**
	* method : remove
	* variable : std::string uuid 
	* delete this object.
	*/
	void remove(const std::string & uuid);

	void addProperty(const std::string & uuidProperty, vtkDataArray* dataProperty);
	
	long getAttachmentPropertyCount(const std::string & uuid, const VtkEpcCommon::FesppAttachmentProperty propertyUnit) ;
protected:
	
	/**
	* method : attach
	* variable : --
	* attach to this object the differents representations.
	*/
	void attach();
	
private:
	// EPC DOCUMENT
	common::EpcDocument *epcPackage;
	
	// All representation
#if _MSC_VER < 1600
	std::tr1::unordered_map<std::string, std::vector<VtkPolylineRepresentation *>> uuidToVtkPolylineRepresentation;
	std::tr1::unordered_map<std::string, std::vector<VtkTriangulatedRepresentation *>> uuidToVtkTriangulatedRepresentation;
	std::tr1::unordered_map<std::string, VtkProperty *> uuidToVtkProperty;
#else
	std::unordered_map<std::string, std::vector<VtkPolylineRepresentation *>> uuidToVtkPolylineRepresentation;
	std::unordered_map<std::string, std::vector<VtkTriangulatedRepresentation *>> uuidToVtkTriangulatedRepresentation;
	std::unordered_map<std::string, VtkProperty *> uuidToVtkProperty;
#endif
};
#endif
