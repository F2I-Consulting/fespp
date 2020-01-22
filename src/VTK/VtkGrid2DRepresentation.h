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
#ifndef __VtkGrid2DRepresentation_h
#define __VtkGrid2DRepresentation_h

#include <vtkDataArray.h>

#include <fesapi/common/DataObjectRepository.h>

#include "VtkResqml2MultiBlockDataSet.h"
#include "VtkGrid2DRepresentationPoints.h"

class VtkGrid2DRepresentationCells;

class VtkGrid2DRepresentation : public VtkResqml2MultiBlockDataSet
{
public:
	/**
	* Constructor
	*/
	VtkGrid2DRepresentation(const std::string & fileName, const std::string & name, const std::string & uuid, const std::string & uuidParent, COMMON_NS::DataObjectRepository *pckRep, COMMON_NS::DataObjectRepository *pckSubRep);
	
	/**
	* Destructor
	*/
	~VtkGrid2DRepresentation();

	/**
	* method : createTreeVtk
	* variable : 
	* prepare grid2D points and grid2D cells.
	*/
	void createTreeVtk(const std::string & uuid, const std::string & parent, const std::string & name, VtkEpcCommon::Resqml2Type resqmlType);

	/**
	* method : attach
	* variable : --
	* attach grid2D points and grid2D cells.
	*/
	void attach();
	
	/**
	* method : remove
	* variable : std::string uuid 
	* create grid2D points and grid2D cells.
	*/
	void visualize(const std::string & uuid);
	
	/**
	* method : remove
	* variable : std::string uuid
	* delete the vtkMultiBlockDataSet
	*/
	void remove(const std::string & uuid);

	void addProperty(const std::string & uuidProperty, vtkDataArray* dataProperty);

	long getAttachmentPropertyCount(const std::string & uuid, VtkEpcCommon::FesppAttachmentProperty propertyUnit);
	
protected:
	/**
	* method : createOutput
	* variable : std::string uuid 
	* create grid2D points and grid2D cells.
	*/
	int createOutput(const std::string & uuid);
	
private:
	// EPC DOCUMENT
	COMMON_NS::DataObjectRepository *repositoryRepresentation;
	COMMON_NS::DataObjectRepository *repositorySubRepresentation;

	VtkGrid2DRepresentationPoints grid2DPoints;
//	VtkGrid2DRepresentationCells* grid2DCells;
};
#endif
