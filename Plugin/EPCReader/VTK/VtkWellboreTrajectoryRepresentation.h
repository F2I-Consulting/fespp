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
#ifndef __VtkWellboreTrajectoryRepresentation_h
#define __VtkWellboreTrajectoryRepresentation_h

#include "VtkResqml2MultiBlockDataSet.h"

#include <unordered_map>

#include "VtkWellboreTrajectoryRepresentationPolyLine.h"
#include "VtkWellboreFrame.h"

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
	VtkWellboreTrajectoryRepresentation(const std::string & fileName, const std::string & name, const std::string & uuid, const std::string & uuidParent, COMMON_NS::DataObjectRepository const * repoRepresentation, COMMON_NS::DataObjectRepository const * repoSubRepresentation);
	
	/**
	* Destructor
	* Release the created VtkWellboreFrames
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
	* method : toggleMarkerOrientation
	* variable : const bool orientation
	* enable/disable marker orientation option
	*/
	void toggleMarkerOrientation(bool orientation);

	/**
	* method : setMarkerSize
	* variable : int size 
	* set the new marker size
	*/
	void setMarkerSize(int size);

	/**
	* method : remove
	* variable : std::string uuid (Wellbore trajectory representation UUID)
	* delete the vtk objects for represent Wellbore trajectory (polyline + datum + text).
	*/
	void remove(const std::string & uuid);

	void addProperty(const std::string & uuidProperty, vtkDataArray* dataProperty);

	long getAttachmentPropertyCount(const std::string & uuid, VtkEpcCommon::FesppAttachmentProperty propertyUnit);
	
private:
	// EPC DOCUMENT
	COMMON_NS::DataObjectRepository const * repositoryRepresentation;
	COMMON_NS::DataObjectRepository const * repositorySubRepresentation;

	// VTK object
	VtkWellboreTrajectoryRepresentationPolyLine polyline;

	// We need to persist somehow the vtk representation of the wellbore frames in order not to have to redraw them at each selection change.
	std::unordered_map<std::string, VtkWellboreFrame *> uuid_to_VtkWellboreFrame;
	std::unordered_map<std::string, VtkEpcCommon> uuid_Informations;
};
#endif
