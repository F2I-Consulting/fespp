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

#include <algorithm>

#include <vtkDataArraySelection.h>
#include <vtkIndent.h>
#include <vtkInformation.h>
#include <vtkInformationVector.h>
#include <vtkMultiBlockDataSet.h>
#include <vtkObjectFactory.h>

// Fespp includes
#include "VTK/VtkEpcDocumentSet.h"

vtkStandardNewMacro(Fespp)

//----------------------------------------------------------------------------
Fespp::Fespp() :
FileName(nullptr), SubFileName(nullptr),
UuidList(vtkDataArraySelection::New()), Controller(nullptr),
loadedFile(false), fileNameSet(std::vector<std::string>()),
epcDocumentSet(nullptr)
#ifdef WITH_ETP
, etpDocument(nullptr), isEtpDocument(false),
port(""), ip("")
#endif
{
	SetNumberOfInputPorts(0);
	SetNumberOfOutputPorts(1);

	MarkerOrientation = 1;
	MarkerSize = 10;

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

	std::string extension="";
	if (nameStr.length() > 3) {
		extension = nameStr.substr (nameStr.length()-3,3);
	}

#ifdef WITH_ETP
	if (isEtpDocument) {
		const auto it = nameStr.find(":");
		if (it != std::string::npos) {
			port = nameStr.substr(it+1);
			ip = nameStr.substr(0,it);
		}
	}
#endif
	if (extension == "epc" ) {
		FileName = "EpcDocument";
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
	UuidList->AddArray(uuid, status);
	loadedFile = true;
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
		if(strcmp(FileName,"EpcDocument") == 0)  {
			epcDocumentSet->unvisualize(uuidStr);
		}
#ifdef WITH_ETP
		if(isEtpDocument && etpDocument!=nullptr) {
			etpDocument->unvisualize(uuidStr);
		}
#endif
	}
	else {
		if(strcmp(FileName,"EpcDocument") == 0)  {
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
void Fespp::setMarkerOrientation(const bool orientation) {
	cout << orientation << endl;

	if (MarkerOrientation != orientation) {
		MarkerOrientation = orientation;
#ifdef WITH_ETP
		if (etpDocument == nullptr) {
			/******* TODO
			 * enable/disable marker orientation for ETP document
			 */
		}
#endif
		if(strcmp(FileName,"EpcDocument") == 0)  {
			epcDocumentSet->toggleMarkerOrientation(MarkerOrientation);
		}

		Modified();
		Update();
		UpdateDataObject();
		UpdateInformation();
		UpdateWholeExtent();
	}
}

//----------------------------------------------------------------------------
void Fespp::setMarkerSize(const int size) {
	cout << size << endl;


#ifdef WITH_ETP
	if (etpDocument == nullptr) {
		/******* TODO
		 * modify marker size for ETP document
		 */
	}
#endif
	if(strcmp(FileName,"EpcDocument") == 0)  {
		/******* TODO
		 * modifiy marker size for EPC document
		 */
	}

	if (MarkerSize != size) {
		Modified();
		Update();
		UpdateDataObject();
		UpdateInformation();
		UpdateWholeExtent();
	}
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
void Fespp::OpenEpcDocument(const std::string & name)
{
	if (epcDocumentSet == nullptr) {
		epcDocumentSet = new VtkEpcDocumentSet(idProc, nbProc, VtkEpcCommon::modeVtkEpc::Both);
	}
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
	if(strcmp(FileName,"EpcDocument") == 0) 	{
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
			cout << "output nombre de block : " << output->GetNumberOfBlocks() << endl;
			cout << "epcDocumentSet nombre de block : " << epcDocumentSet->getVisualization()->GetNumberOfBlocks() << endl;
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
