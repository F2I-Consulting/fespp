#include "PQDataLoadManager.h"

#include "PQToolsManager.h"

#include "pqApplicationCore.h"
#include "pqObjectBuilder.h"
#include "pqPipelineSource.h"
#include "vtkSMProperty.h"
#include "vtkSMSourceProxy.h"
#include "pqDataRepresentation.h"
#include "pqSMAdaptor.h"
#include "pqUndoStack.h"

#include <pqPropertiesPanel.h>
#include <pqActiveObjects.h>

#include <pqPipelineRepresentation.h>

#include "vtkSMPropertyHelper.h"
#include <vtkSMStringVectorProperty.h>

#include <QPushButton>

#include "ui_PQDataLoadManager.h"
class PQDataLoadManager::pqUI : public Ui::PQDataLoadManager
{
};

namespace
{
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

//=============================================================================
PQDataLoadManager::PQDataLoadManager(QWidget* p, Qt::WindowFlags f /*=0*/)
: QDialog(p, f)
{
	PQToolsManager* manager = PQToolsManager::instance();
	this->Server = manager->getActiveServer();

	this->ui = new PQDataLoadManager::pqUI;
	this->ui->setupUi(this);

	this->ui->epcFile->setServer(this->Server);
	this->ui->epcFile->setForceSingleFile(true);
	this->ui->epcFile->setExtension("epc Files (*.epc)");

	QObject::connect(this->ui->epcFile, SIGNAL(filenamesChanged(const QStringList&)), this,
			SLOT(checkInputValid()));

	QObject::connect(this, SIGNAL(accepted()), this, SLOT(setupPipeline()));

	this->checkInputValid();
}

PQDataLoadManager::~PQDataLoadManager()
{
	delete this->ui;
}

//-----------------------------------------------------------------------------
void PQDataLoadManager::checkInputValid()
{
	bool valid = true;

	if (this->ui->epcFile->filenames().isEmpty())
		valid = false;

	this->ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(valid);
}

//-----------------------------------------------------------------------------
void PQDataLoadManager::setupPipeline()
{
	pqApplicationCore* core = pqApplicationCore::instance();
	pqObjectBuilder* builder = core->getObjectBuilder();

	PQToolsManager* manager = PQToolsManager::instance();

	BEGIN_UNDO_SET("EPC Data Load");

	pqPipelineSource* fesppReader;

	if (!manager->existPipe()){
		fesppReader = builder->createReader("sources", "Fespp", QStringList("EpcDocument"), this->Server);
		manager->existPipe(true);
	}
	else{
		fesppReader = manager->getFesppReader();
	}

	QString epcFiles = this->ui->epcFile->filenames().join("*");

	if (!epcFiles.isEmpty())
	{
		// active Pipeline
		if (fesppReader)
		{
			pqActiveObjects *activeObjects = &pqActiveObjects::instance();
			activeObjects->setActiveSource(fesppReader);

			manager->newFile(epcFiles.toStdString().c_str());

			// add file to property
			vtkSMProxy* fesppReaderProxy = fesppReader->getProxy();

	        vtkSMPropertyHelper( fesppReaderProxy, "subFileList" ).SetStatus(epcFiles.toStdString().c_str(),1);

	        fesppReaderProxy->UpdateSelfAndAllInputs();
		}
	}
	END_UNDO_SET();
}
