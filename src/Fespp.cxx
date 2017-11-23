#include "Fespp.h"

#include "VTK/VtkEpcDocument.h"

#include "PQSelectionPanel.h"

#include "vtkObjectFactory.h" 
#include "vtkInformation.h"
#include "vtkInformationRequestKey.h"
#include "vtkInformationVector.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkSMProxy.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkDataArraySelection.h"
#include "vtkCallbackCommand.h"

#include <QApplication>
#include <pqPropertiesPanel.h>
#include <pqView.h>
#include <pqPVApplicationCore.h>

#include <sstream>
#include <qmessagebox.h>

namespace
{
	PQSelectionPanel* getPanelSelection()
	{
		// get Selection panel
		PQSelectionPanel *panel = 0;
		foreach(QWidget *widget, qApp->topLevelWidgets())
		{
			panel = widget->findChild<PQSelectionPanel *>();
			if (panel)
			{
				break;
			}
		}
		return panel;
	}

	pqPropertiesPanel* getpqPropertiesPanel()
	{
		// get multi-block inspector panel
		pqPropertiesPanel *panel = 0;
		foreach(QWidget *widget, qApp->topLevelWidgets())
		{
			panel = widget->findChild<pqPropertiesPanel *>();

			if (panel)
			{
				break;
			}
		}
		return panel;
	}

}

vtkStandardNewMacro(Fespp);

//----------------------------------------------------------------------------
Fespp::Fespp()
{
	FileName = NULL;

	this->uuidPropertys = vtkDataArraySelection::New();

	SetNumberOfInputPorts(0);
	SetNumberOfOutputPorts(1);

	treeWidgetSelection = getPanelSelection();
	loadedFile = false;
}

//----------------------------------------------------------------------------
Fespp::~Fespp()
{
	//deleting object variables
	if (this->uuidPropertys != NULL)
	{
		this->uuidPropertys = nullptr;
	}
	SetFileName(0);
}

char* Fespp::whatFile()
{
	return FileName;
}

//----------------------------------------------------------------------------
void Fespp::visualize(std::string file, std::string uuid)
{
	if (this->uuidPropertys != NULL){
		if (uuid != "first")
		{
			this->change("Fisrt", 0);
		}
		this->change(uuid, 1);
	}
}

void Fespp::change(std::string uuid, unsigned int status)
{
	this->SetupOutputInformation(this->GetOutputPortInformation(0));
	this->SetuuidPropertysArrayStatus(uuid.c_str(), status);
	this->SetupOutputInformation(this->GetOutputPortInformation(0));
}

void Fespp::displayError(std::string msg)
{
	vtkErrorMacro(<< msg.c_str());
}

void Fespp::displayWarning(std::string msg)
{
	vtkWarningMacro(<< msg.c_str());
}
//----------------------------------------------------------------------------
void Fespp::RemoveUuid(std::string uuid)
{
	this->change(uuid, 0);
}

//----------------------------------------------------------------------------
int Fespp::CanReadFile(const char* name)
{
	if (treeWidgetSelection->canAddFile(name))
		return 1;
	return 0;
}

//----------------------------------------------------------------------------
int Fespp::RequestInformation(
	vtkInformation *request,
	vtkInformationVector **inputVector,
	vtkInformationVector *outputVector)
{
	if (!loadedFile)
	{
		try{
			common::EpcDocument *pck = new common::EpcDocument(FileName);
			std::string result = pck->deserialize();
			if (result.empty())
			{
				auto warnings = pck->getWarnings();
				for (const auto& warning : warnings)
				{
					vtkWarningMacro("Deserialization warning : " + warning);
				}
				treeWidgetSelection->addFileName(FileName, this, pck);
				loadedFile = true;
			}
			else
			{
				vtkErrorMacro("XML validation error : " + result);
				pck->close();
			}
		}
		catch (const std::exception & e)
		{
			vtkErrorMacro("EXCEPTION in fesapi when reading file " << FileName << " : " << e.what());
		}
	}
	return 1;
}

//----------------------------------------------------------------------------
int Fespp::RequestData(vtkInformation *request,
	vtkInformationVector **inputVector,
	vtkInformationVector *outputVector)
{
	// get the info object
	vtkInformation *outInfo = outputVector->GetInformationObject(0);
	// get the output
	vtkMultiBlockDataSet *output = vtkMultiBlockDataSet::SafeDownCast(outInfo->Get(vtkMultiBlockDataSet::DATA_OBJECT()));
	if (loadedFile)
	{
		try{
			std::string StrFileName(FileName);
			output->DeepCopy(treeWidgetSelection->getOutput(StrFileName));
		}
		catch (const std::exception & e)
		{
			vtkErrorMacro("EXCEPTION fesapi.dll:" << e.what());
		}
	}
	return 1;
}

//----------------------------------------------------------------------------
void Fespp::PrintSelf(ostream& os, vtkIndent indent)
{
	Superclass::PrintSelf(os, indent);
	os << indent << "File Name: " << (FileName ? FileName : "(none)") << "\n";
}

//----------------------------------------------------------------------------
int Fespp::GetuuidPropertysArrayStatus(const char* name)
{
	// if 'name' not found, it is treated as 'disabled'
	return (this->uuidPropertys->ArrayIsEnabled(name));
}
//----------------------------------------------------------------------------
void Fespp::SetuuidPropertysArrayStatus(const char* name, int status)
{
	if (status)
	{
		this->uuidPropertys->EnableArray(name);
	}
	else
	{
		int nbProperty = uuidPropertys->GetNumberOfArrays();

		if (this->uuidPropertys != nullptr)
		{
			int test = this->uuidPropertys->GetNumberOfArrays();
			this->uuidPropertys->EnableArray(name);
			this->uuidPropertys->RemoveArrayByName(name);
		}
		else
			this->uuidPropertys = vtkDataArraySelection::New();
	}
	this->uuidPropertys->Modified();
}

//----------------------------------------------------------------------------
void Fespp::SetupOutputInformation(vtkInformation *outInfo)
{
	this->Modified();
	pqApplicationCore::instance()->render();

	getpqPropertiesPanel()->updateGeometry();
	getpqPropertiesPanel()->show();
	getpqPropertiesPanel()->view()->forceRender();
	getpqPropertiesPanel()->view()->render();
	getpqPropertiesPanel()->setUpdatesEnabled(true);
	emit getpqPropertiesPanel()->apply();
}

//----------------------------------------------------------------------------
int Fespp::GetNumberOfuuidPropertysArrays()
{
	return this->uuidPropertys->GetNumberOfArrays();
}

//----------------------------------------------------------------------------
const char* Fespp::GetuuidPropertysArrayName(int index)
{
	return this->uuidPropertys->GetArrayName(index);
}
