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
#ifndef __vtkEPCReader_h
#define __vtkEPCReader_h

// include system
#include <string>
#include <set>
#include <utility>

#include <vtkPartitionedDataSetCollectionAlgorithm.h>
#include <vtkSmartPointer.h>
#include <vtkStringArray.h>
#include <vtkIntArray.h>
#include <vtkCommand.h> // For UserEvent

#include "EnergisticsModule.h"
#include "Mapping/ResqmlDataRepositoryToVtkPartitionedDataSetCollection.h"

class vtkDataAssembly;
class vtkProperty;
class vtkMultiProcessController;
class vtkCallbackCommand;
class vtkRenderWindow;

/**
 * A VTK reader for EPC document.
 */
class ENERGISTICS_EXPORT  vtkEPCReader : public vtkPartitionedDataSetCollectionAlgorithm
{
public:
	static vtkEPCReader *New();
	vtkTypeMacro(vtkEPCReader, vtkPartitionedDataSetCollectionAlgorithm);

	// --------------- PART: files------ -------------


	///@{
	/**
	* API to set the filenames.
	*/
	void AddFileNameToFiles(const char* fname);
	void ClearFileName();
	///@}

protected:
	vtkEPCReader();
	~vtkEPCReader() final;

	char* FileName;
};
#endif

