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
#include "Fespp.h"

#include <vtkDataArraySelection.h>
#include <vtkIndent.h>
#include <vtkInformation.h>
#include <vtkInformationVector.h>
#include <vtkMultiBlockDataSet.h>
#include <vtkObjectFactory.h>

// Fespp includes
#include "VTK/VtkEpcDocumentSet.h"

vtkStandardNewMacro(Fespp);

//----------------------------------------------------------------------------
Fespp::Fespp() :
				FileName(nullptr), SubFileName(nullptr),
				UuidList(vtkDataArraySelection::New()), Controller(nullptr),
				loadedFile(false), fileNameSet(std::vector<std::string>()),
				epcDocumentSet(nullptr), isEpcDocument(false)
#ifdef WITH_ETP
				, etpDocument(nullptr), isEtpDocument(false),
				port(""), ip("")
#endif
{
	SetNumberOfInputPorts(0);
	SetNumberOfOutputPorts(1);

	vtkMultiProcessController* controller = vtkMultiProcessController::GetGlobalController();
	if (controller != nullptr) {
		SetController(controller);
		idProc = controller->GetLocalProcessId();
		nbProc = controller->GetNumberOfProcesses();
	}
	else {
		idProc = 0;
		nbProc = 1;
	}
}

//----------------------------------------------------------------------------
Fespp::~Fespp()
{
	SetController(nullptr);
	UuidList->Delete();

	if (epcDocumentSet != nullptr) {
		delete epcDocumentSet;
	}

#ifdef WITH_ETP
	if (etpDocument != nullptr) {
		delete etpDocument;
	}
#endif
}

//----------------------------------------------------------------------------
void Fespp::SetSubFileName(const char* name)
{
	const std::string nameStr = std::string(name);
#ifdef WITH_ETP
	if (isEtpDocument) {
		const auto it = nameStr.find(":");
		if (it != std::string::npos) {
			port = nameStr.substr(it+1);
			ip = nameStr.substr(0,it);
		}
	}
#endif
	if (isEpcDocument) {
		if (std::find(fileNameSet.begin(), fileNameSet.end(), nameStr) == fileNameSet.end())	{
			fileNameSet.push_back(nameStr);
			OpenEpcDocument(nameStr);
		}
	}
}

//----------------------------------------------------------------------------
int Fespp::GetUuidListArrayStatus(const char* uuid)
{
	return UuidList->ArrayIsEnabled(uuid);
}
//----------------------------------------------------------------------------
void Fespp::SetUuidList(const char* uuid, int status)
{
	const std::string uuidStr = std::string(uuid);
	if (uuidStr == "connect") {
#ifdef WITH_ETP
		if (etpDocument == nullptr) {
			loadedFile = true;
			etpDocument = new VtkEtpDocument(ip, port, VtkEpcCommon::modeVtkEpc::Representation);
		}
#endif
	}
	else if (uuidStr.find("allWell-") != std::string::npos) {
		if (status == 0) {
			epcDocumentSet->unvisualizeFullWell(uuidStr.substr(8));
		} else {
			epcDocumentSet->visualizeFullWell(uuidStr.substr(8));
		}
	}
	else if (status == 0) {
		if (isEpcDocument) {
			epcDocumentSet->unvisualize(uuidStr);
		}
#ifdef WITH_ETP
		if(isEtpDocument && etpDocument!=nullptr) {
			etpDocument->unvisualize(uuidStr);
		}
#endif
	}
	else {
		if(isEpcDocument) {
			auto msg = epcDocumentSet->visualize(uuidStr);
			if  (!msg.empty()){
				displayError(msg);
			}
		}
#ifdef WITH_ETP
		if(isEtpDocument && etpDocument!=nullptr) {
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
int Fespp::GetNumberOfUuidListArrays()
{
	return UuidList->GetNumberOfArrays();
}

//----------------------------------------------------------------------------
const char* Fespp::GetUuidListArrayName(int index)
{
	return UuidList->GetArrayName(index);
}

//----------------------------------------------------------------------------
void Fespp::displayError(const std::string & msg)
{
	if (!msg.empty()) {
		vtkErrorMacro(<< msg.c_str());
	}
}

//----------------------------------------------------------------------------
void Fespp::displayWarning(const std::string & msg)
{
	if (!msg.empty()) {
		vtkWarningMacro(<< msg.c_str());
	}
}

//----------------------------------------------------------------------------
int Fespp::RequestInformation(
		vtkInformation *,
		vtkInformationVector **,
		vtkInformationVector *)
{
	if ( !loadedFile ) {
		const std::string stringFileName = std::string(FileName);
		if (stringFileName == "EpcDocument") {
			isEpcDocument = true;
			loadedFile = true;
			epcDocumentSet = new VtkEpcDocumentSet(idProc, nbProc, VtkEpcCommon::modeVtkEpc::Both);
		}
#ifdef WITH_ETP
		else if(stringFileName == "EtpDocument") {
			isEtpDocument = true;
		}
#endif
	}
	return 1;
}

//----------------------------------------------------------------------------
void Fespp::OpenEpcDocument(const std::string & name)
{
	displayWarning(epcDocumentSet->addEpcDocument(name));
}

//----------------------------------------------------------------------------
int Fespp::RequestData(vtkInformation *,
		vtkInformationVector **,
		vtkInformationVector *outputVector)
{
#ifdef WITH_ETP
	if (isEtpDocument)	{
		RequestDataEtpDocument(outputVector);
	}
#endif
	if (isEpcDocument)	{
		RequestDataEpcDocument(outputVector);
	}
	return 1;
}
//----------------------------------------------------------------------------
void Fespp::RequestDataEpcDocument(vtkInformationVector *outputVector)
{
	vtkInformation *outInfo = outputVector->GetInformationObject(0);
	vtkMultiBlockDataSet *output = vtkMultiBlockDataSet::SafeDownCast(outInfo->Get(vtkMultiBlockDataSet::DATA_OBJECT()));

	if (loadedFile) {
		try {
			output->DeepCopy(epcDocumentSet->getVisualization());
		}
		catch (const std::exception & e)
		{
			displayError("EXCEPTION in FESAPI library: " + std::string(e.what()));
		}
	}
}

//----------------------------------------------------------------------------
#ifdef WITH_ETP
void Fespp::RequestDataEtpDocument(vtkInformationVector * outputVector)
{
	vtkInformation *outInfo = outputVector->GetInformationObject(0);
	vtkMultiBlockDataSet *output = vtkMultiBlockDataSet::SafeDownCast(outInfo->Get(vtkMultiBlockDataSet::DATA_OBJECT()));
	if (loadedFile)	{
		output->DeepCopy(etpDocument->getVisualization());
	}
}
#endif
//----------------------------------------------------------------------------
void Fespp::PrintSelf(ostream& os, vtkIndent indent)
{
	Superclass::PrintSelf(os, indent);
	os << indent << "fileName: " << (FileName ? FileName : "(none)") << endl;
}
