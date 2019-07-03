#include "PQEtpConnectionManager.h"

#include "PQToolsManager.h"
#include "PQEtpPanel.h"

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

#include "Fespp.h"
#include "ui_PQEtpConnectionManager.h"
class PQEtpConnectionManager::pqUI : public Ui::PQEtpConnectionManager
{
};

namespace
{
PQEtpPanel* getPQEtpPanel()
{
	PQEtpPanel *panel = nullptr;
	foreach(QWidget *widget, qApp->topLevelWidgets()) {
		panel = widget->findChild<PQEtpPanel *>();
		if(panel!=nullptr) {
			break;
		}
	}
	return panel;
}
}

//=============================================================================
PQEtpConnectionManager::PQEtpConnectionManager(QWidget* p, Qt::WindowFlags f /*=0*/)
: QDialog(p, f)
{
	PQToolsManager* manager = PQToolsManager::instance();
	Server = manager->getActiveServer();

	ui = new PQEtpConnectionManager::pqUI;
	ui->setupUi(this);

	QObject::connect(this, SIGNAL(accepted()), this, SLOT(setupPipeline()));

	checkInputValid();
}

PQEtpConnectionManager::~PQEtpConnectionManager()
{
	delete ui;
}

//-----------------------------------------------------------------------------
void PQEtpConnectionManager::checkInputValid()
{
	bool valid = true;
	ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(valid);
}

//-----------------------------------------------------------------------------
void PQEtpConnectionManager::setupPipeline()
{
	pqApplicationCore* core = pqApplicationCore::instance();
	pqObjectBuilder* builder = core->getObjectBuilder();

	PQToolsManager* manager = PQToolsManager::instance();

	BEGIN_UNDO_SET("ETP Data Load");

	pqPipelineSource* fesppReader;

	if (manager->etp_existPipe()) 	{
		fesppReader = manager->getFesppReader();
	}
	else {
		fesppReader = builder->createReader("sources", "Fespp", QStringList("EtpDocument"), Server);
		manager->etp_existPipe(true);
	}

	if (fesppReader != nullptr) {
		pqActiveObjects *activeObjects = &pqActiveObjects::instance();
		activeObjects->setActiveSource(fesppReader);

		auto connection_parameter = this->ui->etp_ip->text()+":"+ this->ui->etp_port->text();
		vtkSMProxy* fesppReaderProxy = fesppReader->getProxy();
		vtkSMPropertyHelper( fesppReaderProxy, "SubFileName" ).Set(connection_parameter.toStdString().c_str());
		fesppReaderProxy->UpdateSelfAndAllInputs();
	}
	getPQEtpPanel()->etpClientConnect(	this->ui->etp_ip->text().toStdString(), this->ui->etp_port->text().toStdString());
	END_UNDO_SET();
}
