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
	etpDocument = nullptr;
	isEtpDocument=false;
	isEpcDocument=false;
}

//----------------------------------------------------------------------------
Fespp::~Fespp()
{
	SetFileName(nullptr); // Also delete FileName if not nullptr.
	SetController(nullptr);
	fileNameSet.clear();
	uuidList->Delete();
	idProc = 0;
	nbProc = 0;

	if (epcDocumentSet != nullptr) {
		delete epcDocumentSet;
		epcDocumentSet = nullptr;
	}

	if (etpDocument != nullptr) {
		delete etpDocument;
		etpDocument = nullptr;
	}

	countTest = 0;
}

//----------------------------------------------------------------------------
void Fespp::SetSubFileName(const char* name)
{
	if(isEtpDocument)
	{
		auto it = std::string(name).find(":");
		if(it != std::string::npos)
		{
			auto port = std::string(name).substr(it+1);
			auto ip = std::string(name).substr(0,it);
			cout << "Fespp ip : " << ip << " : " << port << endl;
		}
	}
	if(isEpcDocument)
	{
		if (std::find(fileNameSet.begin(), fileNameSet.end(),std::string(name))==fileNameSet.end())
		{
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
	if (std::string(uuid)=="connect")
	{
		if(etpDocument == nullptr)
		{
			cout << "Fespp ETP !! open session\n";
			loadedFile = true;
			etpDocument = new VtkEtpDocument("localhost", "8080", VtkEpcCommon::Representation);
		}
	} else 	if (status != 0)
	{
		if(isEpcDocument)
			epcDocumentSet->visualize(std::string(uuid));
		if(isEtpDocument)
			etpDocument->visualize(std::string(uuid));
	}
	else
	{
		if(isEpcDocument)
			epcDocumentSet->unvisualize(std::string(uuid));
		if(isEtpDocument)
			etpDocument->unvisualize(std::string(uuid));
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
	if ( !loadedFile )
	{
		auto stringFileName = std::string(FileName);
		auto lengthFileName = stringFileName.length();
		auto extension = stringFileName.substr(lengthFileName -3, lengthFileName);

		if (stringFileName == "EpcDocument")
		{
			isEpcDocument=true;
			loadedFile = true;
			epcDocumentSet = new VtkEpcDocumentSet(idProc, nbProc, VtkEpcCommon::Both);
		}
		if(stringFileName == "EtpDocument")
		{
			isEtpDocument=true;
		}
		if(extension=="epc")
		{
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
	epcDocumentSet->addEpcDocument(name);
}

//----------------------------------------------------------------------------
int Fespp::RequestData(vtkInformation *request,
		vtkInformationVector **inputVector,
		vtkInformationVector *outputVector)
{
	if (isEpcDocument)
	{
		RequestDataEtpDocument(request, inputVector, outputVector);
	}

	if (isEpcDocument)
	{
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
//			auto outputSet = epcDocumentSet->getVisualization();
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
void Fespp::RequestDataEtpDocument(vtkInformation *request,
		vtkInformationVector **inputVector,
		vtkInformationVector *outputVector)
{
	cout<<"Fespp::RequestDataEtpDocument" << endl;
	vtkInformation *outInfo = outputVector->GetInformationObject(0);
	vtkMultiBlockDataSet *output = vtkMultiBlockDataSet::SafeDownCast(outInfo->Get(vtkMultiBlockDataSet::DATA_OBJECT()));
	if (loadedFile)
	{
//		auto outputSet = etpDocument->getVisualization();
		output->DeepCopy(etpDocument->getVisualization());
	}

}

//----------------------------------------------------------------------------
void Fespp::PrintSelf(ostream& os, vtkIndent indent)
{
	cout<<"Fespp::PrintSelf" << endl;
	Superclass::PrintSelf(os, indent);
	os << indent << "fileName: " << (FileName ? FileName : "(none)") << endl;
}
