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
	Server = manager->getActiveServer();

	this->ui = new PQDataLoadManager::pqUI;
	this->ui->setupUi(this);

	this->ui->epcFile->setServer(Server);
	this->ui->epcFile->setForceSingleFile(false);
	this->ui->epcFile->setExtension("epc Files (*.epc)");

	QObject::connect(ui->epcFile, SIGNAL(filenamesChanged(const QStringList&)), this,
			SLOT(checkInputValid()));

	QObject::connect(this, SIGNAL(accepted()), this, SLOT(setupPipeline()));

	checkInputValid();
}

PQDataLoadManager::~PQDataLoadManager()
{
	delete ui;
}

//-----------------------------------------------------------------------------
void PQDataLoadManager::checkInputValid()
{
	this->ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(!ui->epcFile->filenames().isEmpty());
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
		fesppReader = builder->createReader("sources", "Fespp", QStringList("EpcDocument"), Server);
		manager->existPipe(true);
	}
	QStringList epcFiles =	ui->epcFile->filenames();

	if (!epcFiles.isEmpty() && fesppReader != nullptr) {
		for (int i = 0; i < epcFiles.length(); ++i){
			pqActiveObjects *activeObjects = &pqActiveObjects::instance();
			activeObjects->setActiveSource(fesppReader);

			vtkSMProxy* fesppReaderProxy = fesppReader->getProxy();
			vtkSMPropertyHelper(fesppReaderProxy, "SubFileName").Set(epcFiles[i].toStdString().c_str());

			fesppReaderProxy->UpdateSelfAndAllInputs();

			manager->newFile(epcFiles[i].toStdString().c_str());
		}
	}
	END_UNDO_SET();
}
