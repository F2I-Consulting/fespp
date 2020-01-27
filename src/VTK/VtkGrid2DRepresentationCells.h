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
#ifndef __VtkGrid2DRepresentationCells_h
#define __VtkGrid2DRepresentationCells_h

#include "VtkResqml2StructuredGrid.h"

namespace COMMON_NS
{
	class DataObjectRepository;
}

class VtkGrid2DRepresentationCells : public VtkResqml2StructuredGrid
{
public:
	/**
	* Constructor
	*/
	VtkGrid2DRepresentationCells(const std::string & fileName, const std::string & name, const std::string & uuid, const std::string & uuidParent, COMMON_NS::DataObjectRepository *pckRep, COMMON_NS::DataObjectRepository *pckSubRep);
	
	/**
	* Destructor
	*/
	~VtkGrid2DRepresentationCells();

	/**
	* method : createOutput
	* variable : std::string uuid (Triangulated representation UUID)
	* create the vtk objects for represent triangulated.
	*/
	void createOutput(const std::string & uuid);
	
private:
	std::string lastProperty;
};
#endif
