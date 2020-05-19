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
#include "PQToolsManager.h"

#include <QMainWindow>
#include <QPointer>

#include "PQDataLoadManager.h"
#include "PQSelectionPanel.h"
#ifdef WITH_ETP
#include "PQEtpPanel.h"
#include "PQEtpConnectionManager.h"
#endif

#include "pqActiveObjects.h"
#include "pqApplicationCore.h"
#include "pqServerManagerModel.h"
#include "pqServer.h"
#include "pqRenderView.h"
#include "pqPipelineSource.h"
#include "pqPropertiesPanel.h"
#include "pqPipelineBrowserWidget.h"
#include "pqObjectBuilder.h"
#include "vtkSMPropertyHelper.h"

#include "VTK/VtkEpcCommon.h"

#include "ui_PQActionHolder.h"

namespace
{
pqPropertiesPanel* getpqPropertiesPanel()
{
	pqPropertiesPanel *panel = nullptr;
	foreach(QWidget *widget, qApp->topLevelWidgets())
	{
		panel = widget->findChild<pqPropertiesPanel *>();

		if (panel != nullptr) {
			break;
		}
	}
	return panel;
}

PQSelectionPanel* getPQSelectionPanel()
{
	PQSelectionPanel *panel = nullptr;
	foreach(QWidget *widget, qApp->topLevelWidgets())
	{
		panel = widget->findChild<PQSelectionPanel *>();

		if (panel != nullptr) {
			break;
		}
	}
	return panel;
}
}

//=============================================================================
class PQToolsManager::pqInternal
{
public:
	Ui::PQActionHolder Actions;
	QWidget* ActionPlaceholder;
};

//=============================================================================
QPointer<PQToolsManager> PQToolsManagerInstance = nullptr;

PQToolsManager* PQToolsManager::instance()
{
	if (PQToolsManagerInstance == nullptr) {
		pqApplicationCore* core = pqApplicationCore::instance();
		if (!core) {
			qFatal("Cannot use the Tools without an application core instance.");
			return nullptr;
		}
		PQToolsManagerInstance = new PQToolsManager(core);
	}

	return PQToolsManagerInstance;
}


//-----------------------------------------------------------------------------
PQToolsManager::PQToolsManager(QObject* p)
: QObject(p)
{
	Internal = new PQToolsManager::pqInternal;

	// This widget serves no real purpose other than initializing the Actions
	// structure created with designer that holds the actions.
	Internal->ActionPlaceholder = new QWidget(nullptr);
	Internal->Actions.setupUi(Internal->ActionPlaceholder);

	existEpcPipe = false;
#ifdef WITH_ETP
	existEtpPipe = false;
#endif
	panelSelectionVisible = false;

	QObject::connect(actionDataLoadManager(), SIGNAL(triggered(bool)), this, SLOT(showDataLoadManager()));

#ifdef WITH_ETP
	QObject::connect(actionEtpCommand(), SIGNAL(triggered(bool)), this, SLOT(showEtpConnectionManager()));
#endif

	pqServerManagerModel* smModel = pqApplicationCore::instance()->getServerManagerModel();
	connect(smModel, SIGNAL(sourceRemoved(pqPipelineSource*)), this,
			SLOT(deletePipelineSource(pqPipelineSource*)));

	pqApplicationCore* core = pqApplicationCore::instance();
	pqObjectBuilder* builder = core->getObjectBuilder();
	connect(builder, SIGNAL(readerCreated(pqPipelineSource*, const QStringList &)), this,
			SLOT(newPipelineSource(pqPipelineSource*, const QStringList &)));
}

PQToolsManager::~PQToolsManager()
{
	delete Internal->ActionPlaceholder;
	delete Internal;
}

//-----------------------------------------------------------------------------
QAction* PQToolsManager::actionDataLoadManager()
{
	return Internal->Actions.actionDataLoadManager;
}

//-----------------------------------------------------------------------------
#ifdef WITH_ETP
QAction* PQToolsManager::actionEtpCommand()
{
	return Internal->Actions.actionEtpCommand;
}
#endif

//-----------------------------------------------------------------------------
void PQToolsManager::showDataLoadManager()
{
	PQDataLoadManager* dialog = new PQDataLoadManager(getMainWindow());
	dialog->setAttribute(Qt::WA_DeleteOnClose, true);

	dialog->show();
}

//-----------------------------------------------------------------------------
#ifdef WITH_ETP
void PQToolsManager::showEtpConnectionManager()
{
	PQEtpConnectionManager* dialog = new PQEtpConnectionManager(getMainWindow());
	dialog->setAttribute(Qt::WA_DeleteOnClose, true);

	dialog->show();
}
#endif

//-----------------------------------------------------------------------------
void PQToolsManager::setVisibilityPanelSelection(bool visible)
{
	getPQSelectionPanel()->setVisible(visible);
	panelSelectionVisible=visible;
}

//-----------------------------------------------------------------------------
pqPipelineSource* PQToolsManager::getFesppReader(const std::string & pipe_name)
{
	return findPipelineSource(pipe_name.c_str());
}

//-----------------------------------------------------------------------------
QWidget* PQToolsManager::getMainWindow()
{
	foreach (QWidget* topWidget, QApplication::topLevelWidgets()) {
		if (qobject_cast<QMainWindow*>(topWidget))
			return topWidget;
	}

	return nullptr;
}

//-----------------------------------------------------------------------------
pqServer* PQToolsManager::getActiveServer()
{
	pqApplicationCore* app = pqApplicationCore::instance();
	pqServerManagerModel* smModel = app->getServerManagerModel();
	pqServer* server = smModel->getItemAtIndex<pqServer*>(0);
	return server;
}

//-----------------------------------------------------------------------------
pqPipelineSource* PQToolsManager::findPipelineSource(const char* SMName)
{
	pqApplicationCore* core = pqApplicationCore::instance();
	pqServerManagerModel* smModel = core->getServerManagerModel();

	QList<pqPipelineSource*> sources = smModel->findItems<pqPipelineSource*>(getActiveServer());
	foreach (pqPipelineSource* s, sources) {
		if (strcmp(s->getSMName().toStdString().c_str(), SMName) == 0) {
			return s;
		}
	}
	return nullptr;
}

//-----------------------------------------------------------------------------
bool PQToolsManager::existPipe()
{
#ifdef WITH_ETP
	return existEpcPipe || existEtpPipe;
#else
	return existEpcPipe;
#endif
}

//-----------------------------------------------------------------------------
void PQToolsManager::existPipe(bool value)
{
	existEpcPipe = value;
}

//-----------------------------------------------------------------------------
#ifdef WITH_ETP
bool PQToolsManager::etp_existPipe()
{
	return existEtpPipe;
}
#endif

//-----------------------------------------------------------------------------
#ifdef WITH_ETP
void PQToolsManager::etp_existPipe(bool value)
{
	existEtpPipe = value;
}
#endif

//----------------------------------------------------------------------------
void PQToolsManager::deletePipelineSource(pqPipelineSource* pipe)
{
	if(existPipe())
	{
		if (pipe->getSMName() == "EpcDocument") {
			getPQSelectionPanel()->deleteTreeView();
			existEpcPipe = false;
		}
#ifdef WITH_ETP
		if (pipe->getSMName() == "EtpDocument") {
			getPQSelectionPanel()->deleteTreeView();
			existEtpPipe = false;
		}
#endif
	}
}

//----------------------------------------------------------------------------
void PQToolsManager::newPipelineSource(pqPipelineSource* pipe, const QStringList &filenames)
{
	std::vector<std::string> epcfiles;
	if (!filenames.isEmpty()) {
		for (int i = 0; i < filenames.length(); ++i){
			const size_t lengthFileName = filenames[i].toStdString().length();
			const std::string extension = filenames[i].toStdString().substr(lengthFileName - 3, lengthFileName);
			if(extension == "epc") {
				epcfiles.push_back(filenames[i].toStdString());
			}
		}
	}
	if (epcfiles.size() > 0){
		pqApplicationCore* core = pqApplicationCore::instance();
		pqObjectBuilder* builder = core->getObjectBuilder();

		// get or create reader pipe
		pqPipelineSource* fesppReader;
		if (existPipe()) {
			fesppReader = getFesppReader("EpcDocument");
		}
		else {
			fesppReader = builder->createReader("sources", "Fespp", QStringList("EpcDocument"), getActiveServer());
			existPipe(true);
		}

		// rename file pipe & add file to EpcDocument pipe
		vtkSMProxy* fesppReaderProxy = fesppReader->getProxy();
		std::string newName = "To delete (Artefact create by File-Open EPC)";
		for (int i = 0; i < epcfiles.size(); ++i){
			pipe->rename(QString(newName.c_str()));
			// add file to EpcDocument pipe
			vtkSMPropertyHelper(fesppReaderProxy, "SubFileName").Set(epcfiles[i].c_str());
			fesppReaderProxy->UpdateSelfAndAllInputs();
			// add file to Selection Panel
			newFile(epcfiles[i].c_str());
		}
	}
}

//----------------------------------------------------------------------------
void PQToolsManager::newFile(const std::string & fileName)
{
	getPQSelectionPanel()->addFileName(fileName);
}

