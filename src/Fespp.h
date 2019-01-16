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

#ifndef __FESPP_h
#define __FESPP_h

#include "vtkMultiBlockDataSetAlgorithm.h"

// include system
#include <string>
#include <unordered_map>
#include <vector>

#include <common/EpcDocument.h>

#include "etp/VtkEtpDocument.h"

class VtkEpcDocumentSet;
class VtkEpcDocument;
class vtkStdString;
class vtkCallbackCommand;
class vtkMultiProcessController;
class vtkDataArraySelection;

#ifdef PARAVIEW_USE_MPI
#include <mpi.h>
#endif // PARAVIEW_USE_MPI

class Fespp : public vtkMultiBlockDataSetAlgorithm
{
public:

	static Fespp *New();
	vtkTypeMacro(Fespp, vtkMultiBlockDataSetAlgorithm);
	void PrintSelf(ostream& os, vtkIndent indent);
	
	// Description:
	// Specify file name of the .epc file.
	vtkSetStringMacro(FileName);
	vtkGetStringMacro(FileName);

	void SetSubFileName(const char* name);
	vtkGetStringMacro(SubFileName);

	// Description:
	// Get/set the multi process controller to use for coordinated reads.  By
	// default, set to the global controller.
	vtkGetObjectMacro(Controller, vtkMultiProcessController);
	virtual void SetController(vtkMultiProcessController *);

	vtkGetObjectMacro(uuidList, vtkDataArraySelection);
	int GetuuidListArrayStatus(const char* name);
	int GetNumberOfuuidListArrays();
	const char* GetuuidListArrayName(int index);
	void SetUuidList(const char* name, int status);

	void displayError(std::string);
	void displayWarning(std::string);

protected:
	Fespp();
	~Fespp();

	int RequestInformation(vtkInformation *, vtkInformationVector **, vtkInformationVector *);
	int RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *);

	void RequestDataEpcDocument(vtkInformation *, vtkInformationVector **, vtkInformationVector *);
	void RequestDataEtpDocument(vtkInformation *, vtkInformationVector **, vtkInformationVector *);

	void OpenEpcDocument(const std::string &);

	char*						FileName;
	char*						SubFileName;

	vtkDataArraySelection* 				uuidList;

	vtkMultiProcessController*	Controller;

private:
	Fespp(const Fespp&);
	void operator=(const Fespp&);
	
	bool 						loadedFile;

	// id of process
	int idProc;
	// number of process
	int nbProc;

#ifdef PARAVIEW_USE_MPI
	MPI_Comm GetMPICommunicator();
#endif // PARAVIEW_USE_MPI

	VtkEpcDocumentSet* epcDocumentSet;
	VtkEtpDocument* etpDocument;

	int countTest;

	std::vector<std::string> fileNameSet;
	std::string port;
	std::string ip;

	bool isEtpDocument;
	bool isEpcDocument;
};
#endif

