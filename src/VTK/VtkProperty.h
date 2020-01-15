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

#ifndef __VtkProperty_h
#define __VtkProperty_h

#include <vtkDataArray.h>
#include <vtkSmartPointer.h>

#include "VtkAbstractObject.h"

#include <fesapi/common/DataObjectRepository.h>

namespace resqml2
{
	class AbstractValuesProperty;
}

//namespace COMMON_NS
//{
//	class DataObjectRepository;
//}

namespace resqml2_0_1
{
	class AbstractValuesProperty;
	class PolylineSetRepresentation;
	class TriangulatedSetRepresentation;
	class Grid2dRepresentation;
	class AbstractIjkGridRepresentation;
	class UnstructuredGridRepresentation;
	class WellboreTrajectoryRepresentation;
}

class VtkProperty  : public VtkAbstractObject
{
public:
	enum typeSupport { POINTS = 0, CELLS = 1 };

	/**
	* Constructor
	*/
	VtkProperty(const std::string & fileName, const std::string & name, const std::string & uuid, const std::string & uuidParent, COMMON_NS::DataObjectRepository *repo, const int & idProc=0, const int & maxProc=0);

	/**
	* Destructor
	*/
	~VtkProperty();

	/**
	* method : getCellData
	* variable : --
	* return the vtkDataArray.
	*/
//	vtkDataArray* getCellData() const;

	/**
	* method : visualize
	* variable : std::string uuid 
	*/
	void visualize(const std::string & uuid);
	
	/**
	* method : createTreeVtk
	* variable : std::string uuid, std::string parent, std::string name, Resqml2Type resqmlTypeParent
	*/
	void createTreeVtk(const std::string & uuid, const std::string & parent, const std::string & name, const VtkEpcCommon::Resqml2Type & resqmlTypeParent);
	
	/**
	* method : remove
	* variable : std::string uuid
	* delete the vtkDataArray.
	*/
	void remove(const std::string & uuid);

	/**
	* method : visualize (surcharge)
	* variable : std::string uuid , FESAPI representation
	* create the vtkDataArray.
	*/
	vtkDataArray* visualize(const std::string & uuid,  resqml2_0_1::PolylineSetRepresentation* polylineSetRepresentation);
	vtkDataArray* visualize(const std::string & uuid,  resqml2_0_1::TriangulatedSetRepresentation* triangulatedSetRepresentation);
	vtkDataArray* visualize(const std::string & uuid,  resqml2_0_1::Grid2dRepresentation* grid2dRepresentation);
	vtkDataArray* visualize(const std::string & uuid,  resqml2_0_1::AbstractIjkGridRepresentation* ijkGridRepresentation);
	vtkDataArray* visualize(const std::string & uuid,  resqml2_0_1::UnstructuredGridRepresentation* unstructuredGridRepresentation);
	vtkDataArray* visualize(const std::string & uuid,  resqml2_0_1::WellboreTrajectoryRepresentation* wellboreTrajectoryRepresentation);

	unsigned int getSupport();
	vtkDataArray *loadValuesPropertySet(std::vector<resqml2::AbstractValuesProperty*> valuesPropertySet, long cellCount, long pointCount);
	vtkDataArray *loadValuesPropertySet(std::vector<resqml2::AbstractValuesProperty*> valuesPropertySet, long cellCount, long pointCount, int iCellCount, int jCellCount, int kCellCount, int initKIndex);

	long getAttachmentPropertyCount(const std::string & uuid, const VtkEpcCommon::FesppAttachmentProperty propertyUnit) ;

protected:

private:
	vtkSmartPointer<vtkDataArray> cellData;
	typeSupport support;

	// EPC DOCUMENT
	COMMON_NS::DataObjectRepository* repository;
};
#endif
