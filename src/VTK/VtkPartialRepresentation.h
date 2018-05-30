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

#ifndef __VtkPartialRepresentation_h
#define __VtkPartialRepresentation_h

// include system
#if (defined(_WIN32) && _MSC_VER >= 1600)
#include <unordered_map>
#else
#include <tr1/unordered_map>
#endif

#include<string>
#include "VtkAbstractObject.h"

class VtkEpcDocument;
class VtkProperty;

namespace common
{
	class EpcDocument;
}

class VtkPartialRepresentation
{
public:
	/**
	* Constructor
	*/
	VtkPartialRepresentation(const std::string & fileName, const std::string & uuid, VtkEpcDocument *vtkEpcDowumentWithCompleteRep, common::EpcDocument *pck);

	/**
	* Destructor
	*/
	~VtkPartialRepresentation();

	/**
	* method : visualize
	* variable : std::string uuid 
	*/
	void visualize(const std::string & uuid);
	
	/**
	* method : createTreeVtk
	* variable : std::string uuid, std::string parent, std::string name, Resqml2Type resqmlTypeParent
	*/
	void createTreeVtk(const std::string & uuid, const std::string & parent, const std::string & name, const VtkEpcTools::Resqml2Type & resqmlTypeParent);
	
	/**
	* method : remove
	* variable : std::string uuid
	* delete the vtkDataArray.
	*/
	void remove(const std::string & uuid);

	VtkEpcTools::Resqml2Type getType();

	common::EpcDocument * getEpcSource();

private:
#if (defined(_WIN32) && _MSC_VER >= 1600)
	std::unordered_map<std::string, VtkProperty *> uuidToVtkProperty;
#else
	std::tr1::unordered_map<std::string, VtkProperty *> uuidToVtkProperty;
#endif

	// EPC DOCUMENT
	common::EpcDocument *epcPackage;

	VtkEpcDocument *vtkEpcDocumentSource;
	std::string vtkPartialReprUuid;
	std::string fileName;
};
#endif
