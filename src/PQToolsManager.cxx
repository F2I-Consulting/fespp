/*-----------------------------------------------------------------------
Copyright F2I-CONSULTING, (2014)

cedric.robert@f2i-consulting.com

This software is a computer program whose purpose is to display data formatted using Energistics standards.

This software is governed by the CeCILL license under French law and
abiding by the rules of distribution of free software.  You can  use,
modify and/ or redistribute the software under the terms of the CeCILL
license as circulated by CEA, CNRS and INRIA at the following URL
"http://www.cecill.info".

As a counterpart to the access to the source code and  rights to copy,
modify and redistribute granted by the license, users are provided only
with a limited warranty  and the software's author,  the holder of the
economic rights,  and the successive licensors  have only  limited
liability.

In this respect, the user's attention is drawn to the risks associated
with loading,  using,  modifying and/or developing or reproducing the
software by the user in light of its specific status of free software,
that may mean  that it is complicated to manipulate,  and  that  also
therefore means  that it is reserved for developers  and  experienced
professionals having in-depth computer knowledge. Users are therefore
encouraged to load and test the software's suitability as regards their
requirements in conditions enabling the security of their systems and/or
data to be ensured and,  more generally, to use and operate it in the
same conditions as regards security.

The fact that you are presently reading this means that you have had
knowledge of the CeCILL license and that you accept its terms.
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

