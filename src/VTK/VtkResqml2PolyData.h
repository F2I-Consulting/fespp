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

#ifndef __VtkResqml2PolyData_h
#define __VtkResqml2PolyData_h

// include Resqml2.0 VTK
#include "VtkAbstractRepresentation.h"

namespace common
{
	class EpcDocument;
}

// include VTK
#include <vtkSmartPointer.h> 
#include <vtkPolyData.h>

class VtkResqml2PolyData : public VtkAbstractRepresentation
{

public:
	/**
	* Constructor
	*/
	VtkResqml2PolyData(const std::string & fileName, const std::string & name, const std::string & uuid, const std::string & uuidParent, common::EpcDocument *epcPackageRepresentation, common::EpcDocument *epcPackageSubRepresentation, const int & idProc=0, const int & maxProc=0);

	/**
	* method : createOutput
	* variable : --
	* return the vtkPolyData.
	*/
	vtkSmartPointer<vtkPolyData> getOutput() const;

	/**
	* method : createOutput
	* variable : std::string uuid
	* delete the vtkPolyData.
	*/
	void remove(const std::string & uuid);

protected:
	vtkSmartPointer<vtkPolyData> vtkOutput;

private:

};
#endif
