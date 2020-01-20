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

#include "PQDataLoadManager.h"
#include "PQSelectionPanel.h"
#ifdef WITH_ETP
#include "PQEtpPanel.h"
#include "PQEtpConnectionManager.h"
#endif
#include "PQMetaDataPanel.h"

#include "pqActiveObjects.h"
#include "pqApplicationCore.h"
#include "pqServerManagerModel.h"
#include "pqServer.h"
#include "pqRenderView.h"
#include "pqPipelineSource.h"
#include "pqPropertiesPanel.h"

#include "VTK/VtkEpcCommon.h"

#include <QMainWindow>
#include <QPointer>

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

PQMetaDataPanel* getPQMetadataPanel()
{
	PQMetaDataPanel *panel = nullptr;
	foreach(QWidget *widget, qApp->topLevelWidgets()) {
		panel = widget->findChild<PQMetaDataPanel *>();

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
	panelMetadataVisible = false;

	QObject::connect(actionDataLoadManager(), SIGNAL(triggered(bool)), this, SLOT(showDataLoadManager()));
	//QObject::connect(actionPanelSelection(), SIGNAL(triggered(bool)), this, SLOT(showPanelSelection()));
	//	QObject::connect(actionPanelMetadata(), SIGNAL(triggered(bool)), this, SLOT(showPanelMetadata()));
#ifdef WITH_ETP
	QObject::connect(actionEtpCommand(), SIGNAL(triggered(bool)), this, SLOT(showEtpConnectionManager()));
#endif

	//	actionPanelMetadata()->setEnabled(false);
	//	actionPanelSelection()->setEnabled(false);
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
/*
QAction* PQToolsManager::actionPanelSelection()
{
	return Internal->Actions.actionPanelSelection;
}
 */

//-----------------------------------------------------------------------------
//QAction* PQToolsManager::actionPanelMetadata()
//{
//	return Internal->Actions.actionPanelMetadata;
//}

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
void PQToolsManager::showPanelMetadata()
{
	if(panelMetadataVisible) {
		setVisibilityPanelMetadata(false);
		panelMetadataVisible = false;
	}
	else {
		setVisibilityPanelMetadata(true);
		panelMetadataVisible = true;
	}
}

//-----------------------------------------------------------------------------
void PQToolsManager::setVisibilityPanelMetadata(bool visible)
{
	getPQMetadataPanel()->setVisible(visible);
	panelSelectionVisible=visible;
}

//-----------------------------------------------------------------------------
pqPipelineSource* PQToolsManager::getFesppReader()
{
	return findPipelineSource("Fespp");
}

//-----------------------------------------------------------------------------
pqView* PQToolsManager::getFesppView()
{
	return findView(getFesppReader(), 0, pqRenderView::renderViewType());
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
		if (strcmp(s->getProxy()->GetXMLName(), SMName) == 0)
			return s;
	}

	return nullptr;
}

//-----------------------------------------------------------------------------
pqView* PQToolsManager::findView(pqPipelineSource* source, int port, const QString& viewType)
{
	if (source) {
		foreach (pqView* view, source->getViews())
    										{
			pqDataRepresentation* repr = source->getRepresentation(port, view);
			if (repr && repr->isVisible())
				return view;
    										}
	}
	pqView* view = pqActiveObjects::instance().activeView();
	if (view->getViewType() == viewType)
		return view;

	pqApplicationCore* core = pqApplicationCore::instance();
	pqServerManagerModel* smModel = core->getServerManagerModel();
	foreach (view, smModel->findItems<pqView*>()) {
		if (view && (view->getViewType() == viewType) && (view->getNumberOfVisibleRepresentations() < 1)) {
			return view;
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
		pqPipelineSource * source = findPipelineSource("EpcDocument");
		if (!source) {
			getPQSelectionPanel()->deleteTreeView();
			existEpcPipe = false;
		}
#ifdef WITH_ETP
		source = findPipelineSource("EtpDocument");
		if (!source) {
			getPQSelectionPanel()->deleteTreeView();
			existEtpPipe = false;
		}
#endif
	}
	//	actionPanelMetadata()->setEnabled(false);
	//	setVisibilityPanelMetadata(false);
	//	panelMetadataVisible = false;
}

//----------------------------------------------------------------------------
void PQToolsManager::newFile(const std::string & fileName)
{
	//	actionPanelMetadata()->setEnabled(true);
	getPQSelectionPanel()->addFileName(fileName);
	//	actionPanelSelection()->setEnabled(true);

	connect(getpqPropertiesPanel(), SIGNAL(deleteRequested(pqPipelineSource*)), this, SLOT(deletePipelineSource(pqPipelineSource*)));

}

