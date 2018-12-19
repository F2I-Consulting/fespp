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
PQEtpPanel* getPQEtpPanel()
{
	// get multi-block inspector panel
	PQEtpPanel *panel = 0;
	foreach(QWidget *widget, qApp->topLevelWidgets())
	{
		panel = widget->findChild<PQEtpPanel *>();

		if(panel)
		{
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
	this->Server = manager->getActiveServer();

	this->ui = new PQEtpConnectionManager::pqUI;
	this->ui->setupUi(this);

	QObject::connect(this, SIGNAL(accepted()), this, SLOT(setupPipeline()));

	this->checkInputValid();
}

PQEtpConnectionManager::~PQEtpConnectionManager()
{
	delete this->ui;
}

//-----------------------------------------------------------------------------
void PQEtpConnectionManager::checkInputValid()
{
	bool valid = true;

	this->ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(valid);
}

//-----------------------------------------------------------------------------
void PQEtpConnectionManager::setupPipeline()
{
	pqApplicationCore* core = pqApplicationCore::instance();
	pqObjectBuilder* builder = core->getObjectBuilder();

	PQToolsManager* manager = PQToolsManager::instance();

	BEGIN_UNDO_SET("ETP Data Load");

	pqPipelineSource* fesppReader;

	if (manager->etp_existPipe())
	{
		fesppReader = manager->getFesppReader();
	}
	else
	{
		fesppReader = builder->createReader("sources", "Fespp", QStringList("EtpDocument"), this->Server);

		manager->etp_existPipe(true);
	}

	// active Pipeline
	if (fesppReader)
	{
		pqActiveObjects *activeObjects = &pqActiveObjects::instance();
		activeObjects->setActiveSource(fesppReader);

		// add connection parameter to property
		auto connection_parameter = this->ui->etp_ip->text()+":"+this->ui->etp_port->text();
		vtkSMProxy* fesppReaderProxy = fesppReader->getProxy();
		vtkSMPropertyHelper( fesppReaderProxy, "SubFileName" ).Set(connection_parameter.toStdString().c_str());
		fesppReaderProxy->UpdateSelfAndAllInputs();
	}
	getPQEtpPanel()->etpClientConnect(	this->ui->etp_ip->text().toStdString(), this->ui->etp_port->text().toStdString());
END_UNDO_SET();
}
