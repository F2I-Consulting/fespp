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
#ifndef _PQControler_H
#define _PQControler_H

#include <string>
#include <vector>
#include <set>

#include <QObject>

class vtkSMProxyLocator;
class vtkPVXMLElement;
class pqPipelineSource;
class pqServer;
class pqView;

/// This singleton class manages the state associated with the packaged
/// visualizations provided by the Fespp tools.
class PQControler : public QObject
{
	Q_OBJECT

public:
	static PQControler* instance();
	~PQControler() = default;

	void newFile(std::string);

	void setFileStatus(std::string, int);
	void setUuidStatus(std::string, int);

protected slots:
	// connection with pipeline
	void deletePipelineSource(pqPipelineSource*);
	void newPipelineSource(pqPipelineSource*, const QStringList &);

	// connection with pvsm (load/save state)
	void loadEpcState(vtkPVXMLElement *root, vtkSMProxyLocator *locator);
	void saveEpcState(vtkPVXMLElement* root);

private:
	PQControler(QObject* p);

	pqPipelineSource* getOrCreatePipelineSource();
	
	bool existEpcPipe;
#ifdef WITH_ETP
	bool existEtpPipe;
#endif
	std::set<std::string> uuid_checked_set;
	std::set<std::string> file_name_set;

	Q_DISABLE_COPY(PQControler)
};

#endif
