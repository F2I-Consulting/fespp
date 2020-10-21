/*-----------------------------------------------------------------------
Licensed to the Apache Software Foundation (ASF) under one
or more contributor license agreements.  See the NOTICE file
distributed with this work for additional information
regarding copyright ownership.  The ASF licenses this file
to you under the Apache License, Version 2.0 (the
"License"; you may not use this file except in compliance
with the License.  You may obtain a copy of the License at

  http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing,
software distributed under the License is distributed on an
"AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
KIND, either express or implied.  See the License for the
specific language governing permissions and limitations
under the License.
-----------------------------------------------------------------------*/
#ifndef __VtkEpcDocument_h
#define __VtkEpcDocument_h

#include <vtkDataArray.h>

// Fesapi namespaces
#include <fesapi/nsDefinitions.h>
#include <fesapi/common/DataObjectRepository.h>

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

/** @brief	The epc document representation.
 */

class VtkEpcDocument : public VtkResqml2MultiBlockDataSet
{

public:
	/**
	* Constructor
	*/
	VtkEpcDocument (const std::string & fileName, int idProc=0, int maxProc=0, VtkEpcDocumentSet * epcDocSet =nullptr);

	/**
	* Destructor
	*/
	~VtkEpcDocument();

	/**
	 * load a data object in VTK object
	 *
	 * @param 	uuid	a uuid of epc document to load
	 */
	void visualize(const std::string & uuid);

	/**
	 * load all Wellbores in VTK object
	 */
	void visualizeFullWell();

	/**
	 * unload all Wellbores in VTK object
	 */
	void unvisualizeFullWell();
	
	/**
	 * Change orientation state for all Welbore markers
	 *
	 * @param 	orientation	false - no orientation for all welbore markers
	 * 						true - dips/azimuth orientation for all wellbore markers
	 */
	void toggleMarkerOrientation(const bool orientation);

	/**
	 * Change for all markers size
	 *
	 * @param 	size	size in pixel for all markers
	 */
	void setMarkerSize(int size);

	/**
	 * unload a data object in VTK object
	 *
	 * @param 	uuid	a uuid of epc document to unload
	 */
	void remove(const std::string & uuid);
	
	/**
	* return all VtkEpcCommons for calculate TreeView representation
	* notes: when no time series link => timeIndex = -1 
	*/
	std::vector<VtkEpcCommon const *> getAllVtkEpcCommons() const;

	/**
	* add all Vtk representation to the epc VtkMultiblockDataset
	*/
	void attach();

	/**
	* return all uuids in epc document
	*/
	std::vector<std::string> getListUuid();

	/**
	* apply data for a property
	*
	* @param 	uuidProperty	the property uuid
	* @param 	uuid	the data
	*/
	void addProperty(const std::string & uuidProperty, vtkDataArray* dataProperty);

	/**
	* get type for a uuid
	*
	* @param 	uuid	
	*/
	VtkEpcCommon::Resqml2Type getType(std::string uuid);

	/**
	* get all info (VtkEpcCommon) for a uuid
	*
	* @param 	uuid	
	*/
	VtkEpcCommon getInfoUuid(std::string uuid);

	/**
	* get the element count for a unit (points/cells) on which property values attached 
	*
	* @param 	uuid	property or representation uuid
	* @param 	propertyUnit	unit	
	*/
	long getAttachmentPropertyCount(const std::string & uuid, VtkEpcCommon::FesppAttachmentProperty propertyUnit);

	/**
	* get the ICell count for a representation or property uuid
	*
	* @param 	uuid	property or representation uuid
	*/
	int getICellCount(const std::string & uuid);

	/**
	* get the JCell count for a representation or property uuid
	*
	* @param 	uuid	property or representation uuid
	*/
	int getJCellCount(const std::string & uuid);

	/**
	* get the KCell count for a representation or property uuid
	*
	* @param 	uuid	property or representation uuid
	*/
	int getKCellCount(const std::string & uuid);

	/**
	* for multiprocess get the init K layer index for a process
	*
	* @param 	uuid	property or representation uuid
	*/
	int getInitKIndex(const std::string & uuid);

	/**
	* return error messages
	*/
	std::string getError() ;

	const COMMON_NS::DataObjectRepository& getDataObjectRepository() const;

private:
	void addGrid2DTreeVtk(const std::string & uuid, const std::string & parent, const std::string & name);
	void addPolylineSetTreeVtk(const std::string & uuid, const std::string & parent, const std::string & name);
	void addTriangulatedSetTreeVtk(const std::string & uuid, const std::string & parent, const std::string & name);
	void addWellTrajTreeVtk(const std::string & uuid, const std::string & parent, const std::string & name);
	void addWellFrameTreeVtk(const std::string & uuid, const std::string & parent, const std::string & name);
	void addWellMarkerTreeVtk(const std::string & uuid, const std::string & parent, const std::string & name);
	void addIjkGridTreeVtk(const std::string & uuid, const std::string & parent, const std::string & name);
	void addUnstrucGridTreeVtk(const std::string & uuid, const std::string & parent, const std::string & name);
	int addSubRepTreeVtk(const std::string & uuid, const std::string & parent, const std::string & name);
	int addPropertyTreeVtk(const std::string & uuid, const std::string & parent, const std::string & name);
	
	void searchFaultPolylines(const std::string & fileName);
	void searchHorizonPolylines(const std::string & fileName);
	void searchUnstructuredGrid(const std::string & fileName);
	void searchTriangulated(const std::string & fileName);
	void searchGrid2d(const std::string & fileName);
	void searchIjkGrid(const std::string & fileName);
	void searchWellboreTrajectory(const std::string & fileName);
	void searchSubRepresentation(const std::string & fileName);
	void searchTimeSeries(const std::string & fileName);


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
	void createTreeVtk(const std::string & uuid, const std::string & parent, const std::string & name, VtkEpcCommon::Resqml2Type resqmlType);

	COMMON_NS::DataObjectRepository repository;

	std::unordered_map<std::string, VtkGrid2DRepresentation*> uuidToVtkGrid2DRepresentation;
	std::unordered_map<std::string, VtkPolylineRepresentation*> uuidToVtkPolylineRepresentation;
	std::unordered_map<std::string, VtkTriangulatedRepresentation*> uuidToVtkTriangulatedRepresentation;
	std::unordered_map<std::string, VtkSetPatch*> uuidToVtkSetPatch;
	std::unordered_map<std::string, VtkWellboreTrajectoryRepresentation*> uuidToVtkWellboreTrajectoryRepresentation;
	std::unordered_map<std::string, VtkIjkGridRepresentation*> uuidToVtkIjkGridRepresentation;
	std::unordered_map<std::string, VtkUnstructuredGridRepresentation*> uuidToVtkUnstructuredGridRepresentation;
	std::unordered_map<std::string, VtkPartialRepresentation*> uuidToVtkPartialRepresentation;

	std::vector<std::string> uuidPartialRep;
	std::vector<std::string> uuidRep;

	VtkEpcDocumentSet * epcSet;

	std::string epc_error;
};
#endif
