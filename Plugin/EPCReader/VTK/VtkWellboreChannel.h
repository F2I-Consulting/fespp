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
#ifndef SRC_VTK_VTKWELLBORECHANNEL_H_
#define SRC_VTK_VTKWELLBORECHANNEL__H_

#include <string>

// include VTK
#include <vtkSmartPointer.h>

// include Fespp
#include "VtkResqml2PolyData.h"

class VtkWellboreChannel : public VtkResqml2PolyData
{

public:
	/**
	* Constructor
	*/
	VtkWellboreChannel(const std::string & fileName, const std::string & name, const std::string & uuid, const std::string & uuidParent, const COMMON_NS::DataObjectRepository *epcPackageRepresentation, const COMMON_NS::DataObjectRepository *epcPackageSubRepresentation);

	/**
	* Destructor
	*/
	~VtkWellboreChannel() = default;

	/**
	* method : visualize
	* variable : std::string uuid
	* Create the VTK object for representing the RESQML Marker.
	*/
	void visualize(const std::string & uuid) final;

	long getAttachmentPropertyCount(const std::string &, VtkEpcCommon::FesppAttachmentProperty) final { return 0; }

	/**
	* method : visualize
	* variable : std::string uuid
	* delete the vtkPolyData.
	*/
	void remove(const std::string & uuid) final;
};
#endif /* SRC_VTK_VTKWELLBORECHANNEL__H_ */
