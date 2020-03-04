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
#include "PQToolsActionGroup.h"

#include "PQToolsManager.h"

PQToolsActionGroup::PQToolsActionGroup(QObject* p)
: QActionGroup(p)
{
	PQToolsManager* manager = PQToolsManager::instance();
	if (!manager) {
		qFatal("Cannot get Fespp Tools Manager.");
		return;
	}

	addAction(manager->actionDataLoadManager());
//	addAction(manager->actionPanelMetadata());
#ifdef WITH_ETP
	addAction(manager->actionEtpCommand());
#endif // ETP support

	setExclusive(false);
}
