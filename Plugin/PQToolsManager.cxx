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
#include <typeinfo>

#include <QMainWindow>
#include <QPointer>
#include <QMessageBox>

#include "PQSelectionPanel.h"
#ifdef WITH_ETP
#include "PQEtpPanel.h"
#include "PQEtpConnectionManager.h"
#endif

#include "vtkSMProxyLocator.h"
#include "vtkPVXMLElement.h"
#include "pqActiveObjects.h"
#include "pqApplicationCore.h"
#include "pqFileDialog.h"
#include "pqServerManagerModel.h"
#include "pqServer.h"
#include "pqRenderView.h"
#include "pqPipelineSource.h"
#include "pqPropertiesPanel.h"
#include "pqPipelineBrowserWidget.h"
#include "pqObjectBuilder.h"
#include "vtkSMPropertyHelper.h"
#include "vtkDataArraySelection.h"
#include "vtkSMArraySelectionDomain.h"
#include "VTK/VtkEpcCommon.h"
#include "vtkSMProperty.h"

#include "ui_PQActionHolder.h"

namespace
{
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
		if (core == nullptr) {
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

	connect(actionEtpFileSelection(), &QAction::triggered, this, &PQToolsManager::showEpcImportFileDialog);

#ifdef WITH_ETP
	connect(actionEtpCommand(), &QAction::triggered, this, &PQToolsManager::showEtpConnectionManager);
#endif

	connect(pqApplicationCore::instance()->getServerManagerModel(), &pqServerManagerModel::sourceRemoved,
			this, &PQToolsManager::deletePipelineSource);
	connect(pqApplicationCore::instance(), &pqApplicationCore::stateLoaded,
			this, &PQToolsManager::loadEpcState);
	connect(pqApplicationCore::instance()->getObjectBuilder(), QOverload<pqPipelineSource*, const QStringList &>::of(&pqObjectBuilder::readerCreated),
			this, &PQToolsManager::newPipelineSource);

}

PQToolsManager::~PQToolsManager()
{
	delete Internal->ActionPlaceholder;
	delete Internal;
}

//-----------------------------------------------------------------------------
QAction* PQToolsManager::actionEtpFileSelection()
{
	return Internal->Actions.actionEtpFileSelection;
}

//-----------------------------------------------------------------------------
#ifdef WITH_ETP
QAction* PQToolsManager::actionEtpCommand()
{
	return Internal->Actions.actionEtpCommand;
}
#endif

//-----------------------------------------------------------------------------
pqServer* PQToolsManager::getActiveServer()
{
	pqApplicationCore* app = pqApplicationCore::instance();
	pqServerManagerModel* smModel = app->getServerManagerModel();
	pqServer* server = smModel->getItemAtIndex<pqServer*>(0);
	return server;
}

//-----------------------------------------------------------------------------
QWidget* PQToolsManager::getMainWindow()
{
	foreach(QWidget* topWidget, QApplication::topLevelWidgets()) {
		if (qobject_cast<QMainWindow*>(topWidget))
			return topWidget;
	}

	return nullptr;
}

//-----------------------------------------------------------------------------
pqPipelineSource* PQToolsManager::findPipelineSource(const char* SMName)
{
	QList<pqPipelineSource*> sources = pqApplicationCore::instance()->getServerManagerModel()->findItems<pqPipelineSource*>(getActiveServer());
	foreach(pqPipelineSource* s, sources) {
		if (strcmp(s->getSMName().toStdString().c_str(), SMName) == 0) {
			return s;
		}
	}
	return nullptr;
}

//-----------------------------------------------------------------------------
void PQToolsManager::showEpcImportFileDialog()
{
	pqFileDialog dialog(PQToolsManager::instance()->getActiveServer(), getMainWindow(),
			tr("Open EPC document"), "", tr("EPC Documents (*.epc)"));
	dialog.setFileMode(pqFileDialog::ExistingFile);
	if (QDialog::Accepted == dialog.exec())
	{
		pqPipelineSource* fesppReader = getOrCreatePipelineSource();

		pqActiveObjects *activeObjects = &pqActiveObjects::instance();
		activeObjects->setActiveSource(fesppReader);

		vtkSMProxy* fesppReaderProxy = fesppReader->getProxy();
		vtkSMPropertyHelper(fesppReaderProxy, "EpcFileName").Set(dialog.getSelectedFiles()[0].toStdString().c_str());

		fesppReaderProxy->UpdateSelfAndAllInputs();

		PQToolsManager::instance()->newFile(dialog.getSelectedFiles()[0].toStdString().c_str());
	}
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
void PQToolsManager::loadEpcState(vtkPVXMLElement *, vtkSMProxyLocator *)
{
	pqActiveObjects *activeObjects = &pqActiveObjects::instance();
	auto fesppReader = PQToolsManager::instance()->getFesppReader("EpcDocument");
	if (fesppReader) {
		existPipe(true);
		activeObjects->setActiveSource(fesppReader);
		vtkSMProxy* fesppReaderProxy = fesppReader->getProxy();
		std::string epcFileName = vtkSMPropertyHelper(fesppReaderProxy, "EpcFileName").GetAsString();

		getPQSelectionPanel()->addFileName(epcFileName);

		auto UuidList = fesppReaderProxy->GetProperty("UuidList");
		if (UuidList != nullptr) {
			auto ald = UuidList->FindDomain<vtkSMArraySelectionDomain>();
			for (unsigned int i=0; i < ald->GetNumberOfStrings(); ++i) {
				std::string uuid = ald->GetString(i);
				try {
					getPQSelectionPanel()->checkUuid(uuid);
				}
				catch (const std::out_of_range& oor) {
					vtkOutputWindowDisplayErrorText(std::string("Unknown UUID " + uuid).c_str());
				}
			}
		}
	}

}

//----------------------------------------------------------------------------
void PQToolsManager::newPipelineSource(pqPipelineSource* pipe, const QStringList &filenames)
{
	std::vector<std::string> epcfiles;
	for (int i = 0; i < filenames.length(); ++i) {
		if (filenames[i].endsWith(".epc")) {
			epcfiles.push_back(filenames[i].toStdString());
		}
	}

	if (!epcfiles.empty()) {
		/*		QMessageBox::information(NULL, "epc reader", "please! open this files with epc button"); */
		vtkSMProxy* fesppReaderProxy = getOrCreatePipelineSource()->getProxy();
		std::string newName = "To delete (Artefact create by File-Open EPC)";
		for (size_t i = 0; i < epcfiles.size(); ++i){
			pipe->rename(QString(newName.c_str()));
			// add file to EpcDocument pipe
			vtkSMPropertyHelper(fesppReaderProxy, "EpcFileName").Set(epcfiles[i].c_str());
			fesppReaderProxy->UpdateSelfAndAllInputs();
			// add file to Selection Panel
			newFile(epcfiles[i].c_str());
		}
	}
}

pqPipelineSource* PQToolsManager::getOrCreatePipelineSource()
{
	if (existPipe()) {
		return getFesppReader("EpcDocument");
	}
	else {
		existPipe(true);
		pqApplicationCore* core = pqApplicationCore::instance();
		pqObjectBuilder* builder = core->getObjectBuilder();
		pqServer* server = getActiveServer();
		pqPipelineSource* epcReader = builder->createReader("sources", "EPCReader", QStringList("EpcDocument"), server);
		return epcReader;
	}
}

//----------------------------------------------------------------------------
void PQToolsManager::newFile(const std::string & fileName)
{
	getPQSelectionPanel()->addFileName(fileName);
}
