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

#ifndef __VtkIjkGridRepresentation_h
#define __VtkIjkGridRepresentation_h

#include "VtkResqml2UnstructuredGrid.h"

namespace COMMON_NS
{
	class DataObjectRepository;
	class AbstractObject;
}
namespace resqml2_0_1
{
	class AbstractIjkGridRepresentation;
}

class VtkIjkGridRepresentation : public VtkResqml2UnstructuredGrid
{
public:
	/**
	* Constructor
	*/
	VtkIjkGridRepresentation(const std::string & fileName, const std::string & name, const std::string & uuid, const std::string & uuidParent, COMMON_NS::DataObjectRepository *pckRep, COMMON_NS::DataObjectRepository *pckSubRep, const int & idProc=0, const int & maxProc=0);

	/**
	* Destructor
	*/
	~VtkIjkGridRepresentation();
	
	/**
	* method : createOutput
	* variable : uuid (ijk Grid representation or property) 
	* description : 
	*    - if ijk uuid : create the vtk objects for represent ijk Grid 
	*    - if property uuid : add property to ijk Grid 
	*/
	void createOutput(const std::string & uuid);
	
	/**
	* method : addProperty
	* variable : uuid,  dataProperty
	* description :
	* add property to ijk Grid
	*/
	void addProperty(const std::string & uuidProperty, vtkDataArray* dataProperty);

	/**
	* method : getAttachmentPropertyCount
	* variable : uuid,  support property (CELLS or POINTS)
	* return : count 
	*/
	long getAttachmentPropertyCount(const std::string & uuid, const VtkEpcCommon::FesppAttachmentProperty propertyUnit) ;

	int getICellCount(const std::string & uuid) const;

	int getJCellCount(const std::string & uuid) const;

	int getKCellCount(const std::string & uuid) const;

	int getInitKIndex(const std::string & uuid) const;

	void setICellCount(const int & value) {	iCellCount = value; }

	void setJCellCount(const int & value) { jCellCount = value; }

	void setKCellCount(const int & value) { kCellCount = value; }

	void setInitKIndex(const int & value) { initKIndex = value; }

	void setMaxKIndex(const int & value) { maxKIndex = value; }
	/**
	* method : createWithPoints
	* variable : points de l'ijkGrid,  ijkGrid or sub-rep
	* description :
	* create the vtk objects with points
	*/	
	void createWithPoints(const vtkSmartPointer<vtkPoints> & pointsRepresentation, COMMON_NS::AbstractObject* obj2);

	/**
	* method : createpoint
	* variable : 
	* return : vtk points of ijkGrid Representation
	* 	int iCellCount;
	*   int jCellCount;
	*   int kCellCount;
	*   int initKIndex;
	*   int maxKIndex;
	*/	
	vtkSmartPointer<vtkPoints> createpoint();

private:

	/**
	* method : checkHyperslabingCapacity
	* variable : ijkGridRepresentation
	* calc if ijkgrid isHyperslabed
	*/	
	void checkHyperslabingCapacity(resqml2_0_1::AbstractIjkGridRepresentation* ijkGridRepresentation);

	std::string lastProperty;

	int iCellCount;
	int jCellCount;
	int kCellCount;
	int initKIndex;
	int maxKIndex;

	bool isHyperslabed;

};
#endif
