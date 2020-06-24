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

// include VTK
#include <string>

#include "VtkResqml2MultiBlockDataSet.h"

#include "VtkWellboreMarker.h"

class VtkWellboreFrame : public VtkResqml2MultiBlockDataSet
{

public:
	/**
	* Constructor
	*/
	VtkWellboreFrame(const std::string & fileName, const std::string & name, const std::string & uuid, const std::string & uuidParent, const COMMON_NS::DataObjectRepository *epcPackageRepresentation, const COMMON_NS::DataObjectRepository *epcPackageSubRepresentation);

	/**
	* Destructor
	*/
	~VtkWellboreFrame() {}

	/**
	* method : createTreeVtk
	* variable : std::string uuid (Wellbore trajectory representation UUID)
	* create the vtk objects for represent Wellbore trajectory (polyline + datum + text).
	*/
	void createTreeVtk(const std::string & uuid, const std::string & parent, const std::string & name, VtkEpcCommon::Resqml2Type resqmlType) final;

	void visualize(const std::string & uuid) final;

	void toggleMarkerOrientation(const bool & orientation);

	/**
	* method : createOutput
	* variable : std::string uuid
	* delete the vtkPolyData.
	*/
	void remove(const std::string & uuid) final;

	long getAttachmentPropertyCount(const std::string & , VtkEpcCommon::FesppAttachmentProperty ) final { return 0; }
	/**
	* method : attach
	* variable : --
	*/
	void attach();

private:
	// EPC DOCUMENT
	COMMON_NS::DataObjectRepository const *repositoryRepresentation;
	COMMON_NS::DataObjectRepository const *repositorySubRepresentation;

	std::unordered_map<std::string, VtkWellboreMarker *> uuid_to_VtkWellboreMarker;
};

#endif /* SRC_VTK_VTKWELLBOREFRAME_H_ */
