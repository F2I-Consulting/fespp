#include "PQToolsActionGroup.h"

#include "PQToolsManager.h"


PQToolsActionGroup::PQToolsActionGroup(QObject* p)
: QActionGroup(p)
{
	PQToolsManager* manager = PQToolsManager::instance();
	if (!manager)
	{
		qFatal("Cannot get Fespp Tools Manager.");
		return;
	}

	this->addAction(manager->actionDataLoadManager());
	this->addAction(manager->actionPanelMetadata());

	this->setExclusive(false);
}
