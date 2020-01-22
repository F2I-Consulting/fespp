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

#ifndef __VtkWellboreTrajectoryRepresentation_h
#define __VtkWellboreTrajectoryRepresentation_h

#include "VtkResqml2MultiBlockDataSet.h"

#include <vtkDataArray.h>

#include "VtkWellboreTrajectoryRepresentationPolyLine.h"
#include "VtkWellboreTrajectoryRepresentationDatum.h"
#include "VtkWellboreTrajectoryRepresentationText.h"

namespace resqml2_0_1
{
	class AbstractValuesProperty;
}

namespace COMMON_NS
{
	class DataObjectRepository;
}

class VtkWellboreTrajectoryRepresentation : public VtkResqml2MultiBlockDataSet
{
public:
	/**
	* Constructor
	*/
	VtkWellboreTrajectoryRepresentation(const std::string & fileName, const std::string & name, const std::string & uuid, const std::string & uuidParent, COMMON_NS::DataObjectRepository *repoRepresentation, COMMON_NS::DataObjectRepository *repoSubRepresentation);
	
	/**
	* Destructor
	*/
	~VtkWellboreTrajectoryRepresentation();

	/**
	* method : createTreeVtk
	* variable : std::string uuid (Wellbore trajectory representation UUID)
	* create the vtk objects for represent Wellbore trajectory (polyline + datum + text).
	*/
	void createTreeVtk(const std::string & uuid, const std::string & parent, const std::string & name, VtkEpcCommon::Resqml2Type resqmlType);
	
	/**
	* method : visualize
	* variable : std::string uuid (Wellbore trajectory representation UUID)
	* Create & Attach the vtk objects for represent Wellbore trajectory (polyline + datum + text) to this object.
	*/
	void visualize(const std::string & uuid);
	
	/**
	* method : remove
	* variable : std::string uuid (Wellbore trajectory representation UUID)
	* delete the vtk objects for represent Wellbore trajectory (polyline + datum + text).
	*/
	void remove(const std::string & uuid);

	void addProperty(const std::string & uuidProperty, vtkDataArray* dataProperty);

	long getAttachmentPropertyCount(const std::string & uuid, VtkEpcCommon::FesppAttachmentProperty propertyUnit);
protected:
	/**
	* method : createOutput
	* variable : std::string uuid (Wellbore trajectory representation UUID)
	* create the vtk objects for represent Wellbore trajectory (polyline + datum + text).
	*/
	int createOutput(const std::string & uuid);
	
	/**
	* method : attach
	* variable : --
	* Attach the vtk objects for represent Wellbore trajectory (polyline + datum + text) to this object
	*/
	void attach();
	

private:
	// EPC DOCUMENT
	COMMON_NS::DataObjectRepository *repositoryRepresentation;
	COMMON_NS::DataObjectRepository *repositorySubRepresentation;

	// VTK object
	VtkWellboreTrajectoryRepresentationPolyLine polyline;
	VtkWellboreTrajectoryRepresentationDatum head;
	VtkWellboreTrajectoryRepresentationText text;
};
#endif
