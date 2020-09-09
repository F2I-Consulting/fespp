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
#include <vector>

#include <vtkMultiBlockDataSetAlgorithm.h>
#include <vtkMultiProcessController.h>

#include "EPCReaderModule.h"

#ifdef WITH_ETP
	#include "etp/VtkEtpDocument.h"
#endif

class VtkEpcDocumentSet;
class vtkDataArraySelection;

/**
 * A VTK reader for EPC document.
 */
class EPCREADER_EXPORT vtkEPCReader : public vtkMultiBlockDataSetAlgorithm
{
public:
	vtkTypeMacro(vtkEPCReader, vtkMultiBlockDataSetAlgorithm)
	static vtkEPCReader *New();
	void PrintSelf(ostream& os, vtkIndent indent) final;
	
	// Description:
	// Specify file name of the .epc file.
	vtkSetStringMacro(FileName)
	vtkGetStringMacro(FileName)

	void SetSubFileName(const char* name);
	vtkGetStringMacro(EpcFileName)

	// Description:
	// Get/set the multi process controller to use for coordinated reads.
	// By default, set to the global controller.
	vtkGetObjectMacro(Controller, vtkMultiProcessController)
	vtkSetObjectMacro(Controller, vtkMultiProcessController)

	vtkGetObjectMacro(UuidList, vtkDataArraySelection)
	void SetUuidList(const char* name, int status);
	
	// Used by Paraview, derived from the attribute UuidList
	int GetUuidListArrayStatus(const char* name);
	int GetNumberOfUuidListArrays();
	const char* GetUuidListArrayName(int index);

	vtkSetMacro(MarkerOrientation, bool)
	void setMarkerOrientation(const bool orientation);

	vtkSetMacro(MarkerSize, int)
	void setMarkerSize(const int size);

	void displayError(const std::string&);
	void displayWarning(const std::string&);

private:
	vtkEPCReader();
	vtkEPCReader(const vtkEPCReader&);
	~vtkEPCReader() final;

	char* FileName;
	char* EpcFileName;

	vtkDataArraySelection* UuidList;
	bool MarkerOrientation;
	int MarkerSize;

	vtkMultiProcessController* Controller;
	
	bool loadedFile;

	// id of process
	int idProc;
	// number of process
	int nbProc;

	std::vector<std::string> fileNameSet;

	VtkEpcDocumentSet* epcDocumentSet;

#ifdef WITH_ETP
	VtkEtpDocument* etpDocument;
	bool isEtpDocument;
	std::string port;
	std::string ip;
#endif

	int RequestInformation(vtkInformation *, vtkInformationVector **, vtkInformationVector *) final;
	int RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *) final;

	void RequestDataEpcDocument(vtkInformationVector *);
#ifdef WITH_ETP
	void RequestDataEtpDocument(vtkInformationVector *);
#endif

	void OpenEpcDocument(const std::string &);
};
#endif
