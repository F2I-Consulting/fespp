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
#ifndef _PQToolsManager_H
#define _PQToolsManager_H

#include <QObject>

class QAction;

class pqPipelineSource;
class pqServer;
class pqView;

class PQToolsManager : public QObject
{
	Q_OBJECT

public:
	static PQToolsManager* instance();

	~PQToolsManager();

	QAction* actionDataLoadManager();
#ifdef WITH_ETP
	QAction* actionEtpCommand();
#endif

	pqPipelineSource* getFesppReader(const std::string & pipe_name);

	QWidget* getMainWindow();
	pqServer* getActiveServer();

	bool existPipe();
	void existPipe(bool value);

#ifdef WITH_ETP
	bool etp_existPipe();
	void etp_existPipe(bool value);
#endif

	void newFile(const std::string & fileName);

public slots:
	void showDataLoadManager();
#ifdef WITH_ETP
	void showEtpConnectionManager();
#endif

protected:
	pqPipelineSource* findPipelineSource(const char* SMName);
	void setVisibilityPanelSelection(bool visible);

protected slots:
	/**
	 * When a pipeline source is deleted
	 */
	void deletePipelineSource(pqPipelineSource*);
	void newPipelineSource(pqPipelineSource*, const QStringList &);

private:
	PQToolsManager(QObject* p);

	pqPipelineSource* getOrCreatePipelineSource();

	class pqInternal;
	pqInternal* Internal;

	bool existEpcPipe;
#ifdef WITH_ETP
	bool existEtpPipe;
#endif
	bool panelSelectionVisible;
	Q_DISABLE_COPY(PQToolsManager)
};

#endif
