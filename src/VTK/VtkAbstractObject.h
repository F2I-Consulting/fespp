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

#ifndef __VtkAbstractObject_h
#define __VtkAbstractObject_h

// include system
#include <string>
#include "VtkEpcTools.h"

#if (defined(_WIN32) && _MSC_VER < 1600) || (defined(__GNUC__) && (__GNUC__ < 4 || (__GNUC__ == 4 && __GNUC_MINOR__ < 6)))
#include "tools/nullptr_emulation.h"
#endif

class VtkAbstractObject
{
public:
	VtkAbstractObject (const std::string & fileName, const std::string & name="", const std::string & uuid="", const std::string & uuidParent="", const int & idProc=0, const int & maxProc=0);
	/**
	* Destructor
	*/
	~VtkAbstractObject();

	std::string getFileName() const;
	std::string getName() const;
	std::string getUuid() const;
	std::string getParent() const;
	
	int getIdProc() const;
	int getMaxProc() const;

	void setFileName(const std::string &);
	void setName(const std::string &);
	void setUuid(const std::string &);
	void setParent(const std::string &);

	/**
	* load & display representation uuid's
	*/
	virtual void visualize(const std::string & uuid) = 0;
	
	/**
	* create vtk resqml2 element
	*/
	virtual void createTreeVtk(const std::string & uuid, const std::string & parent, const std::string & name, const VtkEpcTools::Resqml2Type & resqmlType) =0;

	/**
	* remove representation uuid's
	*/
	virtual void remove(const std::string & uuid) = 0;

	/**
	*/
	virtual long getAttachmentPropertyCount(const std::string & uuid, const VtkEpcTools::FesppAttachmentProperty propertyUnit) = 0;
protected:

private:
	std::string fileName;
	std::string name;
	std::string uuid;
	std::string uuidParent;

	int idProc;
	int maxProc;

};
#endif
