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
#include "vtkEPCReader.h"

#include <algorithm>

#include <vtkDataArraySelection.h>
#include <vtkIndent.h>
#include <vtkInformation.h>
#include <vtkInformationVector.h>
#include <vtkMultiBlockDataSet.h>
#include <vtkObjectFactory.h>

// vtkEPCReader includes
#include "VTK/VtkEpcDocumentSet.h"

vtkStandardNewMacro(vtkEPCReader)

	//----------------------------------------------------------------------------
	vtkEPCReader::vtkEPCReader() : FileName(nullptr), EpcFileName(nullptr),
								   UuidList(vtkDataArraySelection::New()), Controller(nullptr),
								   loadedFile(false), fileNameSet(std::vector<std::string>()),
								   epcDocumentSet(nullptr)
#ifdef WITH_ETP
								   ,
								   etpDocument(nullptr), isEtpDocument(false),
								   port(""), ip("")
#endif
{
	SetNumberOfInputPorts(0);
	SetNumberOfOutputPorts(1);

	vtkMultiProcessController *controller = vtkMultiProcessController::GetGlobalController();
	if (controller != nullptr)
	{
		SetController(controller);
		idProc = controller->GetLocalProcessId();
		nbProc = controller->GetNumberOfProcesses();
	}
	else
	{
		idProc = 0;
		nbProc = 1;
	}
}

//----------------------------------------------------------------------------
vtkEPCReader::~vtkEPCReader()
{
	SetController(nullptr);
	UuidList->Delete();

	if (epcDocumentSet != nullptr)
	{
		delete epcDocumentSet;
	}

#ifdef WITH_ETP
	if (etpDocument != nullptr)
	{
		delete etpDocument;
	}
#endif
}

//----------------------------------------------------------------------------
void vtkEPCReader::SetSubFileName(const char *name)
{
	const std::string nameStr(name);
	const std::string extension = nameStr.length() > 3
									  ? nameStr.substr(nameStr.length() - 3, 3)
									  : "";

#ifdef WITH_ETP
	if (isEtpDocument)
	{
		const auto it = nameStr.find(":");
		if (it != std::string::npos)
		{
			port = nameStr.substr(it + 1);
			ip = nameStr.substr(0, it);
		}
	}
#endif

	if (extension == "epc")
	{
		SetFileName("EpcDocument");
		if (std::find(fileNameSet.begin(), fileNameSet.end(), nameStr) == fileNameSet.end())
		{
			fileNameSet.push_back(nameStr);
			OpenEpcDocument(nameStr);
		}
	}
}

//----------------------------------------------------------------------------
int vtkEPCReader::GetUuidListArrayStatus(const char *uuid)
{
	return UuidList->ArrayIsEnabled(uuid);
}

//----------------------------------------------------------------------------
void vtkEPCReader::SetUuidList(const char *uuid, int status)
{
	const std::string uuidStr(uuid);
	UuidList->AddArray(uuid, status);
	loadedFile = true;
	if (uuidStr == "connect")
	{
#ifdef WITH_ETP
		if (etpDocument == nullptr)
		{
			loadedFile = true;
			etpDocument = new VtkEtpDocument(ip, port, VtkEpcCommon::modeVtkEpc::Representation);
		}
#endif
	}
	else if (uuidStr.find("allWell-") != std::string::npos)
	{
		if (status == 0)
		{
			epcDocumentSet->unvisualizeFullWell(uuidStr.substr(8));
		}
		else
		{
			epcDocumentSet->visualizeFullWell(uuidStr.substr(8));
		}
	}
	else if (status == 0)
	{
		if (FileName != nullptr && strcmp(FileName, "EpcDocument") == 0)
		{
			epcDocumentSet->unvisualize(uuidStr);
		}
#ifdef WITH_ETP
		if (isEtpDocument && etpDocument != nullptr)
		{
			etpDocument->unvisualize(uuidStr);
		}
#endif
	}
	else
	{
		if (FileName != nullptr && strcmp(FileName, "EpcDocument") == 0)
		{
			std::string msg = epcDocumentSet->visualize(uuidStr);
			if (!msg.empty())
			{
				displayError(msg);
			}
		}
#ifdef WITH_ETP
		if (isEtpDocument && etpDocument != nullptr)
		{
			etpDocument->visualize(uuidStr);
		}
#endif
	}

	Modified();
	Update();
	UpdateDataObject();
	UpdateInformation();
	UpdateWholeExtent();
}

//----------------------------------------------------------------------------
void vtkEPCReader::setMarkerOrientation(bool orientation)
{
#ifdef WITH_ETP
	if (etpDocument == nullptr)
	{
		/******* TODO
			* enable/disable marker orientation for ETP document
			*/
	}
#endif
	if (FileName != nullptr && strcmp(FileName, "EpcDocument") == 0)
	{
		epcDocumentSet->toggleMarkerOrientation(orientation);
	}

	Modified();
	Update();
	UpdateDataObject();
	UpdateInformation();
	UpdateWholeExtent();
}

//----------------------------------------------------------------------------
void vtkEPCReader::setMarkerSize(int size)
{
#ifdef WITH_ETP
	if (etpDocument == nullptr)
	{
		/******* TODO
		* modify marker size for ETP document
		*/
	}
#endif
	if (FileName != nullptr && strcmp(FileName, "EpcDocument") == 0)
	{
		epcDocumentSet->setMarkerSize(size);
	}
	Modified();
	Update();
	UpdateDataObject();
	UpdateInformation();
	UpdateWholeExtent();
}

//----------------------------------------------------------------------------
int vtkEPCReader::GetNumberOfUuidListArrays()
{
	return UuidList->GetNumberOfArrays();
}

//----------------------------------------------------------------------------
const char *vtkEPCReader::GetUuidListArrayName(int index)
{
	return UuidList->GetArrayName(index);
}

//----------------------------------------------------------------------------
void vtkEPCReader::displayError(const std::string &msg)
{
	if (!msg.empty())
	{
		vtkErrorMacro(<< msg.c_str());
	}
}

//----------------------------------------------------------------------------
void vtkEPCReader::displayWarning(const std::string &msg)
{
	if (!msg.empty())
	{
		vtkWarningMacro(<< msg.c_str());
	}
}

//----------------------------------------------------------------------------
int vtkEPCReader::RequestInformation(
	vtkInformation *,
	vtkInformationVector **,
	vtkInformationVector *)
{
	/*
	if ( !loadedFile ) {
		const std::string stringFileName = std::string(FileName);
		if (stringFileName == "EpcDocument") {
			loadedFile = true;
			epcDocumentSet = new VtkEpcDocumentSet(idProc, nbProc, VtkEpcCommon::modeVtkEpc::Both);
		}
#ifdef WITH_ETP
		else if(stringFileName == "EtpDocument") {
			isEtpDocument = true;
		}
#endif
	}
	*/
	return 1;
}

//----------------------------------------------------------------------------
void vtkEPCReader::OpenEpcDocument(const std::string &name)
{
	if (epcDocumentSet == nullptr) {
		epcDocumentSet = new VtkEpcDocumentSet(idProc, nbProc, VtkEpcCommon::modeVtkEpc::Both);
	}
	try {
		displayWarning(epcDocumentSet->addEpcDocument(name));
	}
	catch (const std::exception &e) {
		displayError("EXCEPTION in FESAPI library: " + std::string(e.what()));
	}
}

//----------------------------------------------------------------------------
int vtkEPCReader::RequestData(vtkInformation *,
							  vtkInformationVector **,
							  vtkInformationVector *outputVector)
{
#ifdef WITH_ETP
	if (isEtpDocument)
	{
		RequestDataEtpDocument(outputVector);
	}
#endif
	if (FileName != nullptr && strcmp(FileName, "EpcDocument") == 0)
	{
		RequestDataEpcDocument(outputVector);
	}
	return 1;
}
//----------------------------------------------------------------------------
void vtkEPCReader::RequestDataEpcDocument(vtkInformationVector *outputVector)
{
	vtkInformation *outInfo = outputVector->GetInformationObject(0);
	vtkMultiBlockDataSet *output = vtkMultiBlockDataSet::SafeDownCast(outInfo->Get(vtkMultiBlockDataSet::DATA_OBJECT()));

	if (loadedFile)
	{
		try
		{
			output->DeepCopy(epcDocumentSet->getVisualization());
		}
		catch (const std::exception &e)
		{
			displayError("EXCEPTION in FESAPI library: " + std::string(e.what()));
		}
	}
}

//----------------------------------------------------------------------------
#ifdef WITH_ETP
void vtkEPCReader::RequestDataEtpDocument(vtkInformationVector *outputVector)
{
	vtkInformation *outInfo = outputVector->GetInformationObject(0);
	vtkMultiBlockDataSet *output = vtkMultiBlockDataSet::SafeDownCast(outInfo->Get(vtkMultiBlockDataSet::DATA_OBJECT()));
	if (loadedFile)
	{
		output->DeepCopy(etpDocument->getVisualization());
	}
}
#endif
//----------------------------------------------------------------------------
void vtkEPCReader::PrintSelf(ostream &os, vtkIndent indent)
{
	Superclass::PrintSelf(os, indent);
	os << indent << "fileName: " << (FileName ? FileName : "(none)") << endl;
}
