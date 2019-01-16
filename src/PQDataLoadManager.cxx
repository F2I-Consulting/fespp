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
	auto valid = (this->ui->epcFile->filenames().isEmpty()) ? false : true;
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

	if (manager->existPipe()) {
		fesppReader = manager->getFesppReader();
	}
	else {
		fesppReader = builder->createReader("sources", "Fespp", QStringList("EpcDocument"), this->Server);
		manager->existPipe(true);
	}

	QString epcFiles = this->ui->epcFile->filenames().join("*");

	if (!epcFiles.isEmpty()) 	{
		if (fesppReader) {
			pqActiveObjects *activeObjects = &pqActiveObjects::instance();
			activeObjects->setActiveSource(fesppReader);

			vtkSMProxy* fesppReaderProxy = fesppReader->getProxy();
			vtkSMPropertyHelper( fesppReaderProxy, "SubFileName" ).Set(epcFiles.toStdString().c_str());

			fesppReaderProxy->UpdateSelfAndAllInputs();

			manager->newFile(epcFiles.toStdString().c_str());
		}
	}
	END_UNDO_SET();
}
