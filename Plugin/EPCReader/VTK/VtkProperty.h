﻿/*-----------------------------------------------------------------------
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
#ifndef __VtkProperty_h
#define __VtkProperty_h

#include <vtkDataArray.h>
#include <vtkSmartPointer.h>

#include <fesapi/nsDefinitions.h>

#include "VtkAbstractObject.h"

namespace COMMON_NS
{
	class DataObjectRepository;
}

namespace RESQML2_NS
{
	class AbstractValuesProperty;
	class PolylineSetRepresentation;
	class TriangulatedSetRepresentation;
	class Grid2dRepresentation;
	class AbstractIjkGridRepresentation;
	class UnstructuredGridRepresentation;
	class WellboreTrajectoryRepresentation;
}

/** @brief	the data table of a property
 */

class VtkProperty  : public VtkAbstractObject
{
public:
	enum typeSupport { POINTS = 0, CELLS = 1 };

	/**
	* Constructor
	*/
	VtkProperty(const std::string & fileName, const std::string & name, const std::string & uuid, const std::string & uuidParent, COMMON_NS::DataObjectRepository const * repo, int idProc=0, int maxProc=0);
	~VtkProperty();

	/**
	* Don't call
	*/
	void visualize(const std::string &);

	/**
	* Don't call
	*/
	void createTreeVtk(const std::string & uuid, const std::string & parent, const std::string & name, VtkEpcCommon::Resqml2Type resqmlTypeParent);
	
	/**
	* delete the vtkDataArray.
	*/
	void remove(const std::string & uuid);

	/**
	* create the vtkDataArray.
	*/
	vtkDataArray* visualize(const std::string & uuid, RESQML2_NS::PolylineSetRepresentation const * polylineSetRepresentation);
	vtkDataArray* visualize(const std::string & uuid, RESQML2_NS::TriangulatedSetRepresentation const * triangulatedSetRepresentation);
	vtkDataArray* visualize(const std::string & uuid, RESQML2_NS::Grid2dRepresentation const * grid2dRepresentation);
	vtkDataArray* visualize(const std::string & uuid, RESQML2_NS::AbstractIjkGridRepresentation const * ijkGridRepresentation);
	vtkDataArray* visualize(const std::string & uuid, RESQML2_NS::UnstructuredGridRepresentation const * unstructuredGridRepresentation);
	vtkDataArray* visualize(const std::string & uuid, RESQML2_NS::WellboreTrajectoryRepresentation const * wellboreTrajectoryRepresentation);

	unsigned int getSupport();
	vtkSmartPointer<vtkDataArray> loadValuesPropertySet(const std::vector<RESQML2_NS::AbstractValuesProperty*>& valuesPropertySet, long cellCount, long pointCount);
	vtkSmartPointer<vtkDataArray> loadValuesPropertySet(const std::vector<RESQML2_NS::AbstractValuesProperty*>& valuesPropertySet, long cellCount, long pointCount, int iCellCount, int jCellCount, int kCellCount, int initKIndex);

	long getAttachmentPropertyCount(const std::string & uuid, VtkEpcCommon::FesppAttachmentProperty propertyUnit) ;

protected:

private:
	vtkSmartPointer<vtkDataArray> cellData;
	typeSupport support;

	// EPC DOCUMENT
	COMMON_NS::DataObjectRepository const * repository;
};
#endif
