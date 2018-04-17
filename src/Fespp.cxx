#include <Fespp.h>
#include <vtkDataArraySelection.h>
#include <vtkIndent.h>
#include <vtkInformation.h>
#include <vtkInformationVector.h>
#include <vtkMPI.h>
#include <vtkMPICommunicator.h>
#include <vtkMPIController.h>
#include <vtkMultiBlockDataSet.h>
#include <vtkMultiProcessController.h>
#include <vtkObjectFactory.h>
#include <vtkOStreamWrapper.h>
#include <vtkSetGet.h>
#include <vtkSmartPointer.h>
#include <VTK/VtkEpcDocument.h>
#include <VTK/VtkEpcDocumentSet.h>
#include <exception>
#include <iostream>
#include <utility>
#include <vector>

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

	this->subFileList = vtkDataArraySelection::New();
	this->uuidList = vtkDataArraySelection::New();

	this->SetController(vtkMultiProcessController::GetGlobalController());

	auto comm = GetMPICommunicator();
	if (comm != MPI_COMM_NULL)
	{
		MPI_Comm_rank(comm, &this->idProc);
		MPI_Comm_size(comm, &this->nbProc);
	}
}

//----------------------------------------------------------------------------
Fespp::~Fespp()
{
	SetFileName(nullptr); // Also delete FileName if not nullptr.
	this->SetController(nullptr);
	for (std::pair<std::string, VtkEpcDocument*> element : vtkEpcDocuments)
		delete element.second;
}

//----------------------------------------------------------------------------
int Fespp::GetsubFileListArrayStatus(const char* name)
{
	return (this->subFileList->ArrayIsEnabled(name));
}
//----------------------------------------------------------------------------
void Fespp::SetSubFileList(const char* name, int status)
{
	if (status != 0)
	{
		this->subFileList->EnableArray(name);
	}
	else
	{
		if (this->subFileList != nullptr)
		{
			this->subFileList->EnableArray(name);
		}
		else
			this->subFileList = vtkDataArraySelection::New();
	}
	this->subFileList->Modified();

	openEpcDocument(name);

	this->Modified();
}

//----------------------------------------------------------------------------
int Fespp::GetNumberOfsubFileListArrays()
{
	return this->subFileList->GetNumberOfArrays();
}

//----------------------------------------------------------------------------
const char* Fespp::GetsubFileListArrayName(int index)
{
	return this->subFileList->GetArrayName(index);
}

//----------------------------------------------------------------------------
int Fespp::GetuuidListArrayStatus(const char* uuid)
{
	return (this->uuidList->ArrayIsEnabled(uuid));
}
//----------------------------------------------------------------------------
void Fespp::SetUuidList(const char* uuid, int status)
{
	if (status != 0)
	{
		vtkEpcDocumentSet->visualize(std::string(uuid));
	}
	else
	{
		vtkEpcDocumentSet->unvisualize(std::string(uuid));
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

		vtkEpcDocumentSet = new VtkEpcDocumentSet(idProc, nbProc, false, true);
		openEpcDocument(FileName);
		if (extension=="epc")
		{
			vtkEpcDocumentSet->visualizeFull();
		}
	}
	return 1;
}

//----------------------------------------------------------------------------
void Fespp::openEpcDocument(const std::string & name)
{
	vtkEpcDocumentSet->addEpcDocument(name);

	loadedFile = true;
}

//----------------------------------------------------------------------------
int Fespp::RequestData(vtkInformation *request,
		vtkInformationVector **inputVector,
		vtkInformationVector *outputVector)
{
	auto comm = GetMPICommunicator();
	double t1, t2;
	if (comm != MPI_COMM_NULL)
		t1 = MPI_Wtime();

	if (this->idProc == 0)
		cout << "traitement fait avec " <<  this->nbProc << " processeur(s) \n";

	vtkInformation *outInfo = outputVector->GetInformationObject(0);
	vtkMultiBlockDataSet *output = vtkMultiBlockDataSet::SafeDownCast(outInfo->Get(vtkMultiBlockDataSet::DATA_OBJECT()));

	if (comm != MPI_COMM_NULL)
		MPI_Barrier(comm);

	if (loadedFile)
	{
		try{
			auto outputSet = vtkEpcDocumentSet->getVisualization();

			output->DeepCopy(vtkEpcDocumentSet->getVisualization());
		}
		catch (const std::exception & e)
		{
			vtkErrorMacro("EXCEPTION fesapi.dll:" << e.what());
		}
	}

	if (comm != MPI_COMM_NULL)
	{
		MPI_Barrier(comm);
		t2 = MPI_Wtime();
		if (this->idProc == 0)
			cout << "Elapsed time is " << (t2-t1) << "\n";
	}

	return 1;
}

//----------------------------------------------------------------------------
void Fespp::PrintSelf(ostream& os, vtkIndent indent)
{
	cout<<"Fespp::PrintSelf\n";
	Superclass::PrintSelf(os, indent);
	os << indent << "fileName: " << (FileName ? FileName : "(none)") << "\n";
}
