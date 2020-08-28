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

class vtkSMProxyLocator;
class vtkPVXMLElement;
class pqPipelineSource;
class pqServer;
class pqView;

/// This singleton class manages the state associated with the packaged
/// visualizations provided by the Fespp tools.
class PQToolsManager : public QObject
{
	Q_OBJECT

public:
	static PQToolsManager* instance();

	~PQToolsManager();

	/// Get the action for the respective operation.
	QAction* actionEtpFileSelection();
#ifdef WITH_ETP
	QAction* actionEtpCommand();
#endif

	/// Convenience function for getting the current server.
	pqServer* getActiveServer();

	/// Convenience function for getting the main window.
	QWidget* getMainWindow();

	/// Get the reader objects.  Returns NULL if that reader was never created.
	pqPipelineSource* getFesppReader(const std::string & pipe_name);

	bool existPipe();
	void existPipe(bool value);

#ifdef WITH_ETP
	bool etp_existPipe();
	void etp_existPipe(bool value);
#endif

	void newFile(const std::string & fileName);

public slots:
	void showEpcImportFileDialog();
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
	void loadEpcState(vtkPVXMLElement *root, vtkSMProxyLocator *locator);
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
