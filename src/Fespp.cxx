/*-----------------------------------------------------------------------
Copyright F2I-CONSULTING, (2014)

cedric.robert@f2i-consulting.com

This software is a computer program whose purpose is to display data formatted using Energistics standards.

This software is governed by the CeCILL license under French law and
abiding by the rules of distribution of free software.  You can  use,
modify and/ or redistribute the software under the terms of the CeCILL
license as circulated by CEA, CNRS and INRIA at the following URL
"http://www.cecill.info".

As a counterpart to the access to the source code and  rights to copy,
modify and redistribute granted by the license, users are provided only
with a limited warranty  and the software's author,  the holder of the
economic rights,  and the successive licensors  have only  limited
liability.

In this respect, the user's attention is drawn to the risks associated
with loading,  using,  modifying and/or developing or reproducing the
software by the user in light of its specific status of free software,
that may mean  that it is complicated to manipulate,  and  that  also
therefore means  that it is reserved for developers  and  experienced
professionals having in-depth computer knowledge. Users are therefore
encouraged to load and test the software's suitability as regards their
requirements in conditions enabling the security of their systems and/or
data to be ensured and,  more generally, to use and operate it in the
same conditions as regards security.

The fact that you are presently reading this means that you have had
knowledge of the CeCILL license and that you accept its terms.
-----------------------------------------------------------------------*/
#include <Fespp.h>
#include <vtkDataArraySelection.h>
#include <vtkIndent.h>
#include <vtkInformation.h>
#include <vtkInformationVector.h>
#include <vtkMultiBlockDataSet.h>
#include <vtkMultiProcessController.h>
#include <vtkObjectFactory.h>
#include <vtkOStreamWrapper.h>
#include <vtkSetGet.h>
#include <vtkSmartPointer.h>
#include <exception>
#include <iostream>
#include <utility>
#include <algorithm>

// Fespp include
#include "VTK/VtkEpcDocument.h"
#include "VTK/VtkEpcDocumentSet.h"

#ifdef PARAVIEW_USE_MPI
#include <vtkMPI.h>
#include <vtkMPICommunicator.h>
#include <vtkMPIController.h>
#endif // PARAVIEW_USE_MPI

#ifndef MPICH_IGNORE_CXX_SEEK
#define MPICH_IGNORE_CXX_SEEK
#endif

vtkStandardNewMacro(Fespp);
vtkCxxSetObjectMacro(Fespp, Controller, vtkMultiProcessController);

//----------------------------------------------------------------------------
Fespp::Fespp() :
FileName(nullptr), Controller(nullptr), loadedFile(false), idProc(0), nbProc(0), countTest(0)
{
	SetNumberOfInputPorts(0);
	SetNumberOfOutputPorts(1);

	loadedFile = false;

	this->uuidList = vtkDataArraySelection::New();

	this->SetController(vtkMultiProcessController::GetGlobalController());


#ifdef PARAVIEW_USE_MPI
	auto comm = GetMPICommunicator();
	if (comm != MPI_COMM_NULL)
	{
		MPI_Comm_rank(comm, &this->idProc);
		MPI_Comm_size(comm, &this->nbProc);
	}
#else
	idProc = 0;
	nbProc = 0;
#endif

	countTest = 0;
	epcDocumentSet = nullptr;
#ifdef WITH_ETP
	etpDocument = nullptr;
	isEtpDocument=false;
#endif
	isEpcDocument=false;
}

//----------------------------------------------------------------------------
Fespp::~Fespp()
{
	SetFileName(nullptr);
	SetController(nullptr);
	fileNameSet.clear();
	uuidList->Delete();

	if (epcDocumentSet != nullptr) {
		delete epcDocumentSet;
		epcDocumentSet = nullptr;
	}

#ifdef WITH_ETP
	if (etpDocument != nullptr) {
		delete etpDocument;
		etpDocument = nullptr;
	}
#endif
}

//----------------------------------------------------------------------------
void Fespp::SetSubFileName(const char* name)
{
#ifdef WITH_ETP
	if(isEtpDocument) {
		auto it = std::string(name).find(":");
		if(it != std::string::npos) {
			port = std::string(name).substr(it+1);
			ip = std::string(name).substr(0,it);
		}
	}
#endif
	if(isEpcDocument) {
		if (std::find(fileNameSet.begin(), fileNameSet.end(),std::string(name))==fileNameSet.end())	{
			fileNameSet.push_back(std::string(name));
			this->OpenEpcDocument(name);
		}
	}
}

//----------------------------------------------------------------------------
int Fespp::GetuuidListArrayStatus(const char* uuid)
{
	return (this->uuidList->ArrayIsEnabled(uuid));
}
//----------------------------------------------------------------------------
void Fespp::SetUuidList(const char* uuid, int status)
{
	if (std::string(uuid)=="connect") {
#ifdef WITH_ETP
		if(etpDocument == nullptr) {
			loadedFile = true;
			etpDocument = new VtkEtpDocument(ip, port, VtkEpcCommon::Representation);
		}
#endif
	} else if (status != 0) {
		if(isEpcDocument) {
			auto msg = epcDocumentSet->visualize(std::string(uuid));
			if  (!msg.empty()){
				displayError(msg);
			}
		}
#ifdef WITH_ETP
		if(isEtpDocument && etpDocument!=nullptr) {
				etpDocument->visualize(std::string(uuid));
		}
#endif
	}
	else {
		if(isEpcDocument) {
			epcDocumentSet->unvisualize(std::string(uuid));
		}
#ifdef WITH_ETP
		if(isEtpDocument && etpDocument!=nullptr) {
			etpDocument->unvisualize(std::string(uuid));
		}
#endif
	}

	this->Modified();
	this->Update();
	this->UpdateDataObject();
	this->UpdateInformation();
	this->UpdateWholeExtent();
}

//----------------------------------------------------------------------------
int Fespp::GetNumberOfuuidListArrays()
{
	return this->uuidList->GetNumberOfArrays();
}

//----------------------------------------------------------------------------
const char* Fespp::GetuuidListArrayName(int index)
{
	return this->uuidList->GetArrayName(index);
}



#ifdef PARAVIEW_USE_MPI
//----------------------------------------------------------------------------
MPI_Comm Fespp::GetMPICommunicator()
{
	MPI_Comm comm = MPI_COMM_NULL;

	vtkMPIController *MPIController = vtkMPIController::SafeDownCast(this->Controller);

	if (MPIController != nullptr)
	{
		vtkMPICommunicator *mpiComm = vtkMPICommunicator::SafeDownCast(MPIController->GetCommunicator());

		if (mpiComm != nullptr)
		{
			comm = *mpiComm->GetMPIComm()->GetHandle();
		}
	}
	return comm;
}
#endif


//----------------------------------------------------------------------------
void Fespp::displayError(std::string msg)
{
	vtkErrorMacro(<< msg.c_str());
}

//----------------------------------------------------------------------------
void Fespp::displayWarning(std::string msg)
{
	vtkWarningMacro(<< msg.c_str());
}

//----------------------------------------------------------------------------
int Fespp::RequestInformation(
		vtkInformation *request,
		vtkInformationVector **inputVector,
		vtkInformationVector *outputVector)
{
	if ( !loadedFile ) {
		auto stringFileName = std::string(FileName);
		auto lengthFileName = stringFileName.length();
		auto extension = stringFileName.substr(lengthFileName -3, lengthFileName);
		if (stringFileName == "EpcDocument") {
			isEpcDocument=true;
			loadedFile = true;
			epcDocumentSet = new VtkEpcDocumentSet(idProc, nbProc, VtkEpcCommon::Both);
		}
#ifdef WITH_ETP
		if(stringFileName == "EtpDocument")	{
			isEtpDocument=true;
		}
#endif
		if(extension=="epc") {
			isEpcDocument=true;
			loadedFile = true;
			this->OpenEpcDocument(stringFileName);
			epcDocumentSet->visualizeFull();
		}
	}
	return 1;
}

//----------------------------------------------------------------------------
void Fespp::OpenEpcDocument(const std::string & name)
{
	auto msg = epcDocumentSet->addEpcDocument(name);
	if  (!msg.empty()){
		displayWarning(msg);
	}

}

//----------------------------------------------------------------------------
int Fespp::RequestData(vtkInformation *request,
		vtkInformationVector **inputVector,
		vtkInformationVector *outputVector)
{
#ifdef WITH_ETP
	if (isEtpDocument)	{
		RequestDataEtpDocument(request, inputVector, outputVector);
	}
#endif
	if (isEpcDocument)	{
		RequestDataEpcDocument(request, inputVector, outputVector);
	}
	return 1;
}
//----------------------------------------------------------------------------
void Fespp::RequestDataEpcDocument(vtkInformation *request,
		vtkInformationVector **inputVector,
		vtkInformationVector *outputVector)
{
#ifdef PARAVIEW_USE_MPI
	auto comm = GetMPICommunicator();
	double t1;
	if (comm != MPI_COMM_NULL)
		t1 = MPI_Wtime();
#endif

	if (this->idProc == 0)
		cout << "traitement fait avec " <<  this->nbProc << " processeur(s) \n";

	vtkInformation *outInfo = outputVector->GetInformationObject(0);
	vtkMultiBlockDataSet *output = vtkMultiBlockDataSet::SafeDownCast(outInfo->Get(vtkMultiBlockDataSet::DATA_OBJECT()));

#ifdef PARAVIEW_USE_MPI
	if (comm != MPI_COMM_NULL)
		MPI_Barrier(comm);
#endif

	if (loadedFile)
	{
		try{
			output->DeepCopy(epcDocumentSet->getVisualization());
		}
		catch (const std::exception & e)
		{
			vtkErrorMacro("EXCEPTION fesapi.dll:" << e.what());
		}
	}

#ifdef PARAVIEW_USE_MPI
	if (comm != MPI_COMM_NULL)
	{
		double t2;
		MPI_Barrier(comm);
		t2 = MPI_Wtime();
		if (this->idProc == 0)
			cout << "Elapsed time is " << (t2-t1) << "\n";
	}
#endif
}

//----------------------------------------------------------------------------
#ifdef WITH_ETP
void Fespp::RequestDataEtpDocument(vtkInformation *request,
		vtkInformationVector **inputVector,
		vtkInformationVector *outputVector)
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
	cout<<"Fespp::PrintSelf" << endl;
	Superclass::PrintSelf(os, indent);
	os << indent << "fileName: " << (FileName ? FileName : "(none)") << endl;
}
