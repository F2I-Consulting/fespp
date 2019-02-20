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
#ifdef WITH_ETP
	this->existEtpPipe = false;
#endif
	this->panelSelectionVisible = false;
	this->panelMetadataVisible = false;

	QObject::connect(this->actionDataLoadManager(), SIGNAL(triggered(bool)), this, SLOT(showDataLoadManager()));
	//QObject::connect(this->actionPanelSelection(), SIGNAL(triggered(bool)), this, SLOT(showPanelSelection()));
	QObject::connect(this->actionPanelMetadata(), SIGNAL(triggered(bool)), this, SLOT(showPanelMetadata()));
#ifdef WITH_ETP
	QObject::connect(this->actionEtpCommand(), SIGNAL(triggered(bool)), this, SLOT(showEtpConnectionManager()));
#endif

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
#ifdef WITH_ETP
QAction* PQToolsManager::actionEtpCommand()
{
	return this->Internal->Actions.actionEtpCommand;
}
#endif

//-----------------------------------------------------------------------------
void PQToolsManager::showDataLoadManager()
{
	PQDataLoadManager* dialog = new PQDataLoadManager(this->getMainWindow());
	dialog->setAttribute(Qt::WA_DeleteOnClose, true);

	dialog->show();
}

//-----------------------------------------------------------------------------
#ifdef WITH_ETP
void PQToolsManager::showEtpConnectionManager()
{
	PQEtpConnectionManager* dialog = new PQEtpConnectionManager(this->getMainWindow());
	dialog->setAttribute(Qt::WA_DeleteOnClose, true);

	dialog->show();
}
#endif

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
#ifdef WITH_ETP
	return this->existEpcPipe || this->existEtpPipe;
#else
	return this->existEpcPipe;
#endif
}

//-----------------------------------------------------------------------------
void PQToolsManager::existPipe(bool value)
{
	this->existEpcPipe = value;
}

//-----------------------------------------------------------------------------
#ifdef WITH_ETP
bool PQToolsManager::etp_existPipe()
{
	return this->existEtpPipe;
}
#endif

//-----------------------------------------------------------------------------
#ifdef WITH_ETP
void PQToolsManager::etp_existPipe(bool value)
{
	this->existEtpPipe = value;
}
#endif

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
#ifdef WITH_ETP
		source = findPipelineSource("EtpDocument");
		if (!source)
		{
			getPQSelectionPanel()->deleteTreeView();
			this->existEtpPipe = false;
		}
#endif
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

