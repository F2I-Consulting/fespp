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
#include "PQControler.h"
#include <typeinfo>

#include <QMessageBox>
#include <QDockWidget>
#include <QApplication>
#include <QDateTime>

#include "PQSelectionPanel.h"
#ifdef WITH_ETP
#include "PQEtpPanel.h"
#include "PQEtpConnectionManager.h"
#endif

#include <vtkSMProxyLocator.h>
#include <vtkPVXMLElement.h>
#include <pqActiveObjects.h>
#include <pqApplicationCore.h>
#include <pqServerManagerModel.h>
#include <pqServer.h>
#include <pqPipelineSource.h>
#include <pqPropertiesPanel.h>
#include <pqObjectBuilder.h>
#include <vtkSMPropertyHelper.h>
#include <vtkSMProperty.h>
#include <vtkCollection.h>

#include "PQToolsManager.h"

namespace
{
	PQSelectionPanel* getPQSelectionPanel() {
		foreach(QWidget *widget, qApp->topLevelWidgets()) {
			PQSelectionPanel* panel = widget->findChild<PQSelectionPanel *>();
			if (panel != nullptr) {
				return panel;
			}
		}
		return nullptr;
	}

	pqPropertiesPanel* getpqPropertiesPanel() {
		foreach(QWidget *widget, qApp->topLevelWidgets()) {
			pqPropertiesPanel* panel = widget->findChild<pqPropertiesPanel *>();
			if (panel != nullptr) {
				return panel;
			}
		}
		return nullptr;
	}
}

//=============================================================================
QPointer<PQControler> PQControlerInstance = nullptr;

PQControler* PQControler::instance()
{
	if (PQControlerInstance == nullptr) {
		pqApplicationCore* core = pqApplicationCore::instance();
		if (core == nullptr) {
			qFatal("Cannot use the Tools without an application core instance.");
			return nullptr;
		}
		PQControlerInstance = new PQControler(core);
	}

	return PQControlerInstance;
}

//-----------------------------------------------------------------------------
PQControler::PQControler(QObject* p)
: QObject(p)
{
	existEpcPipe = false;
#ifdef WITH_ETP
	existEtpPipe = false;
#endif

	// connection with pipeline
	connect(pqApplicationCore::instance()->getServerManagerModel(), &pqServerManagerModel::sourceRemoved,
			this, &PQControler::deletePipelineSource);
	connect(pqApplicationCore::instance()->getObjectBuilder(), QOverload<pqPipelineSource*, const QStringList &>::of(&pqObjectBuilder::readerCreated),
			this, &PQControler::newPipelineSource);

	// connection with pvsm (load/save state)
	connect(pqApplicationCore::instance(), SIGNAL(stateSaved(vtkPVXMLElement*)), 
			this, SLOT(saveEpcState(vtkPVXMLElement*)));
	connect(pqApplicationCore::instance(), SIGNAL(stateLoaded(vtkPVXMLElement*, vtkSMProxyLocator*)),
			this, SLOT(loadEpcState(vtkPVXMLElement*, vtkSMProxyLocator*)));

}

//----------------------------------------------------------------------------
void PQControler::newFile(std::string file_name) {
	getOrCreatePipelineSource();
	getPQSelectionPanel()->addFileName(file_name);
}

//----------------------------------------------------------------------------
void PQControler::setFileStatus(std::string file_name, int status) {
	vtkSMProxy* fesppReaderProxy = getOrCreatePipelineSource()->getProxy();
	vtkSMPropertyHelper(fesppReaderProxy, "FilesList").SetStatus(file_name.c_str(), status);
	if (status == 1) {
		file_name_set.insert(file_name);
	} else {
		file_name_set.erase(file_name);
	}

	fesppReaderProxy->UpdatePropertyInformation(fesppReaderProxy->GetProperty("FilesList"));
	fesppReaderProxy->UpdateVTKObjects();
	pqPropertiesPanel* propertiesPanel = getpqPropertiesPanel();
	propertiesPanel->update();
	propertiesPanel->apply();
}

//----------------------------------------------------------------------------
void PQControler::setUuidStatus(std::string uuid, int status) {
	vtkSMProxy* fesppReaderProxy = getOrCreatePipelineSource()->getProxy();
	vtkSMPropertyHelper(fesppReaderProxy, "UuidList").SetStatus(uuid.c_str(), status);
	if (status == 1) {
		uuid_checked_set.insert(uuid);
	} else {
		uuid_checked_set.erase(uuid);
	} 

	fesppReaderProxy->UpdatePropertyInformation(fesppReaderProxy->GetProperty("UuidList"));
	fesppReaderProxy->UpdateVTKObjects();
	pqPropertiesPanel* propertiesPanel = getpqPropertiesPanel();
	propertiesPanel->update();
	propertiesPanel->apply();
}

//----------------------------------------------------------------------------
void PQControler::deletePipelineSource(pqPipelineSource* pipe)
{
	if(existEpcPipe) {
		if (pipe->getSMName() == "EpcDocument") {
			getPQSelectionPanel()->deleteTreeView();
			uuid_checked_set.clear();
			file_name_set.clear();
			// delete EpcReader is automatic
			existEpcPipe = false;
		}
	}
#ifdef WITH_ETP
	if(existEtpPipe) {
		if (pipe->getSMName() == "EtpDocument") {
			getPQSelectionPanel()->deleteTreeView();
			existEtpPipe = false;
		}
	}
#endif
}

//----------------------------------------------------------------------------
void PQControler::newPipelineSource(pqPipelineSource* pipe, const QStringList &filenames)
{
	// opening epc file without going through the button.
	std::vector<std::string> epcfiles;
	for (int i = 0; i < filenames.length(); ++i) {
		if (filenames[i].endsWith(".epc")) {
			epcfiles.push_back(filenames[i].toStdString());
		}
	}

	if (!epcfiles.empty()) {
		pipe->rename(QString("To delete (Artefact create by File-Open EPC)"));

		// creation of a pipe if it does not exist
		getOrCreatePipelineSource();

		// add files to TreeView
		for (size_t i = 0; i < epcfiles.size(); ++i) {
			if (file_name_set.find(epcfiles[i]) != file_name_set.end()) {
				// add to treeview
				getPQSelectionPanel()->addFileName(epcfiles[i]);
			}
		}
	}
}

//----------------------------------------------------------------------------
void PQControler::loadEpcState(vtkPVXMLElement *root, vtkSMProxyLocator *)
{
	// verify tag "EpcDocument" exist 
	vtkPVXMLElement* e = root->FindNestedElementByName("EpcDocument");
  	if (e) {
		// files management
		vtkPVXMLElement* f = e->FindNestedElementByName("files");
		if (f) {
			// creation of a pipe if it does not exist
//			getOrCreatePipelineSource();
			for (unsigned cc = 0; cc < f->GetNumberOfNestedElements(); cc++) {
				vtkPVXMLElement* child = f->GetNestedElement(cc);
				if (child && child->GetName()) {
        			if (strcmp(child->GetName(), "name") == 0) {
						std::string name = child->GetAttributeOrEmpty("value");
						if (file_name_set.find(name) == file_name_set.end()) {
							// add to treeview
							getPQSelectionPanel()->addFileName(name);
						}
					}
				}
			}
			// uuids management
			vtkPVXMLElement* u = e->FindNestedElementByName("uuids");
			if (u) {
				for (unsigned cc = 0; cc < u->GetNumberOfNestedElements(); cc++) {
					vtkPVXMLElement* child = u->GetNestedElement(cc);
					if (child && child->GetName()) {
        				if (strcmp(child->GetName(), "uuid") == 0) {
							std::string uuid = child->GetAttributeOrEmpty("value");
							try {
								if (uuid_checked_set.find(uuid) == uuid_checked_set.end()) {
									// add to treeview
									getPQSelectionPanel()->checkUuid(uuid);
								}
							}
							catch (const std::out_of_range&) {
								vtkOutputWindowDisplayErrorText(std::string("Unknown UUID " + uuid).c_str());
							}
						}
					}
				}
			}
		}
  	}
}

//----------------------------------------------------------------------------
void PQControler::saveEpcState(vtkPVXMLElement* root)
{
	std::string sDate = QDateTime::currentDateTime().toString("dd.MM.yyyy-hh:mm:ss").toStdString();
	bool epcDocumentPVSMExist = false;
	vtkPVXMLElement* ServerManagerState = root->FindNestedElementByName("ServerManagerState");
  	if (ServerManagerState) {
		vtkNew<vtkCollection> elements;
		ServerManagerState->GetElementsByName("ProxyCollection", elements.GetPointer());
		int nbItems = elements->GetNumberOfItems();
	    for (int i = 0; i < nbItems; ++i) {
			vtkPVXMLElement* proxyElement = vtkPVXMLElement::SafeDownCast(elements->GetItemAsObject(i));
			if (strcmp(proxyElement->GetAttributeOrEmpty("name"), "sources") == 0) {
				for (unsigned cc = 0; cc < proxyElement->GetNumberOfNestedElements(); cc++) {
					vtkPVXMLElement* child = proxyElement->GetNestedElement(cc);
					if (strcmp(child->GetAttributeOrEmpty("name"), "EpcDocument") == 0) {
						child->SetAttribute("name", ("EpcDocument-"+sDate).c_str());
						epcDocumentPVSMExist = true;
					}
				}
			}
		}
	}
	if  (epcDocumentPVSMExist) {
	  	vtkNew<vtkPVXMLElement> files;
  		files->SetName("files");
  		for (const auto& file : file_name_set) {
			vtkNew<vtkPVXMLElement> value;
			value->SetName("name");
	  		value->AddAttribute("value", file.c_str());
			files->AddNestedElement(value);
  		}
 		vtkNew<vtkPVXMLElement> uuids;
		uuids->SetName("uuids");
  		for (const auto& uuid : uuid_checked_set) {
			vtkNew<vtkPVXMLElement> value;
			value->SetName("uuid");
	  		value->AddAttribute("value", uuid.c_str());
			uuids->AddNestedElement(value);
	  	}
	  	vtkNew<vtkPVXMLElement> e;
	  	e->SetName("EpcDocument");
	  	e->AddNestedElement(files);
  		e->AddNestedElement(uuids);
  		root->AddNestedElement(e.Get(), 1);
	}
}

//----------------------------------------------------------------------------
pqPipelineSource* PQControler::getOrCreatePipelineSource()
{
	if (existEpcPipe) {
		QList<pqPipelineSource*> sources = pqApplicationCore::instance()->getServerManagerModel()->findItems<pqPipelineSource*>(PQToolsManager::instance()->getActiveServer());
		foreach(pqPipelineSource* s, sources) {
			if (strcmp(s->getSMName().toStdString().c_str(), "EpcDocument") == 0) {
				return s;
			}
		}
		return nullptr;
	} else {
		pqApplicationCore* core = pqApplicationCore::instance();
	
		pqObjectBuilder* builder = core->getObjectBuilder();
		pqPipelineSource* epcReader = builder->createReader("sources", "EPCReader", QStringList("EpcDocument"), PQToolsManager::instance()->getActiveServer());

		existEpcPipe = true;
		return epcReader;
	}
}
