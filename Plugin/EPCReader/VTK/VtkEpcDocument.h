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

#include "VtkResqml2MultiBlockDataSet.h"
#include "VtkEpcDocumentSet.h"

namespace common {
	class DataObjectRepository;
}

class VtkIjkGridRepresentation;
class VtkUnstructuredGridRepresentation;
class VtkPartialRepresentation;
class VtkGrid2DRepresentation;
class VtkPolylineRepresentation;
class VtkTriangulatedRepresentation;
class VtkSetPatch;
class VtkWellboreTrajectoryRepresentation;

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
	virtual ~VtkEpcDocument();

	/**
	* method : visualize
	* variable : std::string uuid 
	* create uuid representation.
	*/
	void visualize(const std::string & uuid);

	/**
	* method : visualizeFullWell
	* create uuid representation with all Welbore_trajectory
	*/
	void visualizeFullWell();
	void unvisualizeFullWell();
	
	/**
	* method : toggleMarkerOrientation
	* variable : const bool orientation
	* enable/disable marker orientation option
	*/
	void toggleMarkerOrientation(const bool orientation);

	/**
	* method : setMarkerSize
	* variable : int size 
	* set the new marker size
	*/
	void setMarkerSize(int size);

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
	std::vector<VtkEpcCommon const *> getAllVtkEpcCommons() const;

	/**
	* method : attach
	* variable : --
	*/
	void attach();

	std::vector<std::string> getListUuid();

	void addProperty(const std::string & uuidProperty, vtkDataArray* dataProperty);

	VtkEpcCommon::Resqml2Type getType(std::string);
	VtkEpcCommon getInfoUuid(std::string);

	long getAttachmentPropertyCount(const std::string & uuid, VtkEpcCommon::FesppAttachmentProperty propertyUnit);
	int getICellCount(const std::string & uuid);
	int getJCellCount(const std::string & uuid);
	int getKCellCount(const std::string & uuid);
	int getInitKIndex(const std::string & uuid);

	std::string getError() ;

	const common::DataObjectRepository* getDataObjectRepository() const;

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

	common::DataObjectRepository* repository;

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
