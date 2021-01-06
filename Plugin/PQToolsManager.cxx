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
#include "PQControler.h"
#ifdef WITH_ETP
#include "PQEtpPanel.h"
#include "PQEtpConnectionManager.h"
#endif

//#include "pqActiveObjects.h"
#include "pqApplicationCore.h"
#include "pqFileDialog.h"
#include "pqServerManagerModel.h"
#include "pqServer.h"
//#include "pqRenderView.h"
//#include "pqPipelineSource.h"
//#include "pqPropertiesPanel.h"
//#include "pqPipelineBrowserWidget.h"
//#include "pqObjectBuilder.h"
//#include "vtkSMPropertyHelper.h"
//#include "vtkDataArraySelection.h"
//#include "vtkSMArraySelectionDomain.h"
//#include "VTK/VtkEpcCommon.h"
//#include "vtkSMProperty.h"

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

	panelSelectionVisible = false;

	connect(actionEtpFileSelection(), &QAction::triggered, this, &PQToolsManager::showEpcImportFileDialog);

	PQControler::instance();
#ifdef WITH_ETP
	connect(actionEtpCommand(), &QAction::triggered, this, &PQToolsManager::showEtpConnectionManager);
#endif
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
void PQToolsManager::showEpcImportFileDialog()
{
	pqFileDialog dialog(PQToolsManager::instance()->getActiveServer(), getMainWindow(),
			tr("Open EPC document"), "", tr("EPC Documents (*.epc)"));
	dialog.setFileMode(pqFileDialog::ExistingFile);
	if (QDialog::Accepted == dialog.exec()) {
		PQControler::instance()->newFile(dialog.getSelectedFiles()[0].toStdString());
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
