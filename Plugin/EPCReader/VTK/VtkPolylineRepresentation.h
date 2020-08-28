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
#ifndef __VtkPolylineRepresentation_h
#define __VtkPolylineRepresentation_h

#include "VtkResqml2PolyData.h"

class VtkPolylineRepresentation : public VtkResqml2PolyData 
{
public:
	/**
	* Constructor
	*/
	VtkPolylineRepresentation(const std::string & fileName, const std::string & name, const std::string & uuid, const std::string & uuidParent, unsigned int patchNo, COMMON_NS::DataObjectRepository *repoRepresentation, COMMON_NS::DataObjectRepository *repoSubRepresentation);
	
	/**
	* Destructor
	*/
	~VtkPolylineRepresentation();

	/**
	* method : visualize
	* variable : std::string uuid (Polyline representation UUID)
	* create the vtk objects for represent Polyline.
	*/
	void visualize(const std::string & uuid);

	void addProperty(const std::string & uuidProperty, vtkDataArray* dataProperty);
	
	long getAttachmentPropertyCount(const std::string & uuid, VtkEpcCommon::FesppAttachmentProperty propertyUnit);

private:
	unsigned int patchIndex;
	
	std::string lastProperty;
};
#endif
