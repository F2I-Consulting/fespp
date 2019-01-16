#include "PQToolsManager.h"

#include "PQDataLoadManager.h"
#include "PQEtpConnectionManager.h"
#include "PQSelectionPanel.h"
#include "PQEtpPanel.h"
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

PQSelectionPanel* getPQSelectionPanel()
{
	PQSelectionPanel *panel = 0;
	foreach(QWidget *widget, qApp->topLevelWidgets())
	{
		panel = widget->findChild<PQSelectionPanel *>();

		if (panel)
		{
			break;
		}
	}
	return panel;
}

PQMetaDataPanel* getPQMetadataPanel()
{
	PQMetaDataPanel *panel = 0;
	foreach(QWidget *widget, qApp->topLevelWidgets())
	{
		panel = widget->findChild<PQMetaDataPanel *>();

		if (panel)
		{
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
	if (PQToolsManagerInstance == nullptr)
	{
		pqApplicationCore* core = pqApplicationCore::instance();
		if (!core)
		{
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
	this->Internal = new PQToolsManager::pqInternal;

	// This widget serves no real purpose other than initializing the Actions
	// structure created with designer that holds the actions.
	this->Internal->ActionPlaceholder = new QWidget(nullptr);
	this->Internal->Actions.setupUi(this->Internal->ActionPlaceholder);

	this->existEpcPipe = false;
	this->existEtpPipe = false;
	this->panelSelectionVisible = false;
	this->panelMetadataVisible = false;

	QObject::connect(this->actionDataLoadManager(), SIGNAL(triggered(bool)), this, SLOT(showDataLoadManager()));
	//QObject::connect(this->actionPanelSelection(), SIGNAL(triggered(bool)), this, SLOT(showPanelSelection()));
	QObject::connect(this->actionPanelMetadata(), SIGNAL(triggered(bool)), this, SLOT(showPanelMetadata()));
	QObject::connect(this->actionEtpCommand(), SIGNAL(triggered(bool)), this, SLOT(showEtpConnectionManager()));

	this->actionPanelMetadata()->setEnabled(false);
	//	this->actionPanelSelection()->setEnabled(false);
}

PQToolsManager::~PQToolsManager()
{
	delete this->Internal->ActionPlaceholder;
	delete this->Internal;
}

//-----------------------------------------------------------------------------
QAction* PQToolsManager::actionDataLoadManager()
{
	return this->Internal->Actions.actionDataLoadManager;
}

//-----------------------------------------------------------------------------
/*
QAction* PQToolsManager::actionPanelSelection()
{
	return this->Internal->Actions.actionPanelSelection;
}
 */

//-----------------------------------------------------------------------------
QAction* PQToolsManager::actionPanelMetadata()
{
	return this->Internal->Actions.actionPanelMetadata;
}

//-----------------------------------------------------------------------------
QAction* PQToolsManager::actionEtpCommand()
{
	return this->Internal->Actions.actionEtpCommand;
}

//-----------------------------------------------------------------------------
void PQToolsManager::showDataLoadManager()
{
	PQDataLoadManager* dialog = new PQDataLoadManager(this->getMainWindow());
	dialog->setAttribute(Qt::WA_DeleteOnClose, true);

	dialog->show();
}

//-----------------------------------------------------------------------------
void PQToolsManager::showEtpConnectionManager()
{
	PQEtpConnectionManager* dialog = new PQEtpConnectionManager(this->getMainWindow());
	dialog->setAttribute(Qt::WA_DeleteOnClose, true);

	dialog->show();
}

//-----------------------------------------------------------------------------
void PQToolsManager::setVisibilityPanelSelection(bool visible)
{
	getPQSelectionPanel()->setVisible(visible);
	this->panelSelectionVisible=visible;
}

//-----------------------------------------------------------------------------
void PQToolsManager::showPanelMetadata()
{
	if(this->panelMetadataVisible)
	{
		this->setVisibilityPanelMetadata(false);
		panelMetadataVisible = false;
	}
	else
	{
		this->setVisibilityPanelMetadata(true);
		panelMetadataVisible = true;
	}
}

//-----------------------------------------------------------------------------
void PQToolsManager::setVisibilityPanelMetadata(bool visible)
{
	getPQMetadataPanel()->setVisible(visible);
	this->panelSelectionVisible=visible;
}

//-----------------------------------------------------------------------------
pqPipelineSource* PQToolsManager::getFesppReader()
{
	return this->findPipelineSource("Fespp");
}

//-----------------------------------------------------------------------------
pqView* PQToolsManager::getFesppView()
{
	return this->findView(this->getFesppReader(), 0, pqRenderView::renderViewType());
}

//-----------------------------------------------------------------------------
QWidget* PQToolsManager::getMainWindow()
{
	foreach (QWidget* topWidget, QApplication::topLevelWidgets())
	{
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

	QList<pqPipelineSource*> sources = smModel->findItems<pqPipelineSource*>(this->getActiveServer());
	foreach (pqPipelineSource* s, sources)
	{
		if (strcmp(s->getProxy()->GetXMLName(), SMName) == 0)
			return s;
	}

	return nullptr;
}

//-----------------------------------------------------------------------------
pqView* PQToolsManager::findView(pqPipelineSource* source, int port, const QString& viewType)
{
	if (source)
	{
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
	foreach (view, smModel->findItems<pqView*>())
	{
		if (view && (view->getViewType() == viewType) &&
				(view->getNumberOfVisibleRepresentations() < 1))
		{
			return view;
		}
	}

	return nullptr;
}

//-----------------------------------------------------------------------------
bool PQToolsManager::existPipe()
{
	return this->existEpcPipe || this->existEtpPipe;
}

//-----------------------------------------------------------------------------
void PQToolsManager::existPipe(bool value)
{
	this->existEpcPipe = value;
}

//-----------------------------------------------------------------------------
bool PQToolsManager::etp_existPipe()
{
	return this->existEtpPipe;
}

//-----------------------------------------------------------------------------
void PQToolsManager::etp_existPipe(bool value)
{
	this->existEtpPipe = value;
}

//----------------------------------------------------------------------------
void PQToolsManager::deletePipelineSource(pqPipelineSource* pipe)
{
	if(this->existPipe())
	{
		pqPipelineSource * source = findPipelineSource("EpcDocument");
		if (!source)
		{
			getPQSelectionPanel()->deleteTreeView();
			this->existEpcPipe = false;
		}
		source = findPipelineSource("EtpDocument");
		if (!source)
		{
			getPQSelectionPanel()->deleteTreeView();
			this->existEtpPipe = false;
		}
	}
	this->actionPanelMetadata()->setEnabled(false);
	this->setVisibilityPanelMetadata(false);
	panelMetadataVisible = false;
}

//----------------------------------------------------------------------------
void PQToolsManager::newFile(const std::string & fileName)
{
	this->actionPanelMetadata()->setEnabled(true);
	getPQSelectionPanel()->addFileName(fileName);
	//	this->actionPanelSelection()->setEnabled(true);

	connect(getpqPropertiesPanel(), SIGNAL(deleteRequested(pqPipelineSource*)), this, SLOT(deletePipelineSource(pqPipelineSource*)));

}

