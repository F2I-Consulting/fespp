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
#ifndef SRC_VTK_VTKWELLBOREFRAME_H_
#define SRC_VTK_VTKWELLBOREFRAME_H_

#include "VtkResqml2MultiBlockDataSet.h"

#include "VtkWellboreMarker.h"
#include "VtkWellboreChannel.h"

class VtkWellboreFrame : public VtkResqml2MultiBlockDataSet
{
public:
	/**
	* Constructor
	*/
	VtkWellboreFrame(const std::string &fileName, const std::string &name, const std::string &uuid, const std::string &uuidParent,
		const COMMON_NS::DataObjectRepository *epcPackageRepresentation, const COMMON_NS::DataObjectRepository *epcPackageSubRepresentation);

	/**
	* Destructor
	* Release the created VtkWellboreMarkers
	*/
	~VtkWellboreFrame();

	/**
	* method : createTreeVtk
	* variable : std::string uuid (Wellbore trajectory representation UUID)
	* create the vtk objects for represent Wellbore trajectory (polyline + datum + text).
	*/
	void createTreeVtk(const std::string &uuid, const std::string &parent, const std::string &name, VtkEpcCommon::Resqml2Type resqmlType);

	/**
	* method : visualize
	* variable : std::string uuid 
	* create uuid representation.
	*/
	void visualize(const std::string &uuid) final;

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
	* method : visualize
	* variable : std::string uuid
	* delete the vtkPolyData.
	*/
	void remove(const std::string &uuid) final;

	long getAttachmentPropertyCount(const std::string &, VtkEpcCommon::FesppAttachmentProperty) final { return 0; }

private:
	// EPC DOCUMENT
	COMMON_NS::DataObjectRepository const *repositoryRepresentation;
	COMMON_NS::DataObjectRepository const *repositorySubRepresentation;

	// We need to persist somehow the vtk representation of the wellbore markers in order not to have to redraw them at each selection change.
	std::unordered_map<std::string, VtkWellboreMarker *> uuid_to_VtkWellboreMarker;
	// We need to persist somehow the vtk representation of the wellbore channels in order not to have to redraw them at each selection change.
	std::unordered_map<std::string, VtkWellboreChannel *> uuid_to_VtkWellboreChannel;
};

#endif /* SRC_VTK_VTKWELLBOREFRAME_H_ */
