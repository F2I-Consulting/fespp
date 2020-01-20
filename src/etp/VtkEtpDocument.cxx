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
#include "etp/VtkEtpDocument.h"

#include <thread>

// include Vtk
#include <vtkInformation.h>
#include <QApplication>
#include <vtkSmartPointer.h>
#include <vtkMultiBlockDataSet.h>

#include <fesapi/etp/EtpHdfProxy.h>

#include <fesapi/resqml2/AbstractRepresentation.h>
#include <fesapi/common/AbstractObject.h>
#include <fesapi/resqml2/AbstractFeatureInterpretation.h>

#include <fesapi/resqml2_0_1/AbstractIjkGridRepresentation.h>
#include <fesapi/resqml2_0_1/ContinuousPropertySeries.h>

// include Fespp
#include "EtpClientSession.h"
#include "PQEtpPanel.h"
#include "etp/EtpFesppStoreProtocolHandlers.h"
#include "etp/EtpFesppDiscoveryProtocolHandlers.h"
#include "VTK/VtkIjkGridRepresentation.h"

PQEtpPanel* getPQEtpPanel()
{
	PQEtpPanel *panel = nullptr;
	foreach(QWidget *widget, qApp->topLevelWidgets()) {
		panel = widget->findChild<PQEtpPanel *>();
		if (panel != nullptr) {
			break;
		}
	}
	return panel;
}

void setSessionToEtpHdfProxy(EtpClientSession* myOwnEtpSession)
{
	COMMON_NS::DataObjectRepository& repo = myOwnEtpSession->repo;
	for (const auto & hdfProxy : repo.getHdfProxySet()) {
		ETP_NS::EtpHdfProxy* etpHdfProxy = dynamic_cast<ETP_NS::EtpHdfProxy*>(hdfProxy);
		if (etpHdfProxy != nullptr && etpHdfProxy->getSession() == nullptr) {
			etpHdfProxy->setSession(myOwnEtpSession->getIoContext(), myOwnEtpSession->getHost(), myOwnEtpSession->getPort(), myOwnEtpSession->getTarget());
		}
	}
}

void startio(VtkEtpDocument *etp_document, std::string ipAddress, std::string port, VtkEpcCommon::modeVtkEpc mode)
{
	boost::asio::io_context ioc;

	Energistics::Etp::v12::Datatypes::Version protocolVersion;
	protocolVersion.m_major = 1;
	protocolVersion.m_minor = 2;
	protocolVersion.m_patch = 0;
	protocolVersion.m_revision = 0;

	std::vector<std::string> supportedObjects;

	std::vector<Energistics::Etp::v12::Datatypes::SupportedProtocol> requestedProtocols;
	Energistics::Etp::v12::Datatypes::SupportedProtocol protocol;

	protocol.m_protocol = Energistics::Etp::v12::Datatypes::Protocol::Core;
	protocol.m_protocolVersion = protocolVersion;
	protocol.m_role = "server";
	requestedProtocols.push_back(protocol);

	protocol.m_protocol = Energistics::Etp::v12::Datatypes::Protocol::Discovery;
	protocol.m_protocolVersion = protocolVersion;
	protocol.m_role = "store";
	requestedProtocols.push_back(protocol);

	protocol.m_protocol = Energistics::Etp::v12::Datatypes::Protocol::Store;
	protocol.m_protocolVersion = protocolVersion;
	protocol.m_role = "store";
	requestedProtocols.push_back(protocol);

	protocol.m_protocol = Energistics::Etp::v12::Datatypes::Protocol::DataArray;
	protocol.m_protocolVersion = protocolVersion;
	protocol.m_role = "store";
	requestedProtocols.push_back(protocol);

	protocol.m_protocol = Energistics::Etp::v12::Datatypes::Protocol::StoreNotification;
	protocol.m_protocolVersion = protocolVersion;
	protocol.m_role = "store";
	requestedProtocols.push_back(protocol);

	auto client_session_sharedPtr = std::make_shared<EtpClientSession>(ipAddress, port, "/", "", requestedProtocols, supportedObjects, mode);
	client_session_sharedPtr->setCoreProtocolHandlers(std::make_shared<ETP_NS::CoreHandlers>(client_session_sharedPtr));
	client_session_sharedPtr->setDiscoveryProtocolHandlers(std::make_shared<EtpFesppDiscoveryProtocolHandlers>(client_session_sharedPtr, etp_document));
	client_session_sharedPtr->setStoreProtocolHandlers(std::make_shared<EtpFesppStoreProtocolHandlers>(client_session_sharedPtr, &client_session_sharedPtr->repo));
	client_session_sharedPtr->setDataArrayProtocolHandlers(std::make_shared<ETP_NS::DataArrayHandlers>(client_session_sharedPtr));
	etp_document->setClientSession(client_session_sharedPtr.get());
	client_session_sharedPtr->run();
}

//----------------------------------------------------------------------------
VtkEtpDocument::VtkEtpDocument(const std::string & ipAddress, const std::string & port, const VtkEpcCommon::modeVtkEpc & mode) :
	VtkResqml2MultiBlockDataSet("EtpDocument", "EtpDocument", "EtpDocument", "", 0, 0), client_session(nullptr),
	treeViewMode(mode == VtkEpcCommon::Both || mode == VtkEpcCommon::TreeView), representationMode(mode == VtkEpcCommon::Both || mode == VtkEpcCommon::Representation),
	last_id(0), loading(false)
{
	vtkOutput = vtkSmartPointer<vtkMultiBlockDataSet>::New();

	std::thread runSessionThread(startio, this, ipAddress, port, mode);
	runSessionThread.detach(); // Detach the thread since we don't want it to be a blocking one.
}

//----------------------------------------------------------------------------
VtkEtpDocument::~VtkEtpDocument()
{
	loading = false;
	if (client_session != nullptr)	{
		client_session->close();
	}
	for(auto i : uuidToVtkIjkGridRepresentation) {
		delete i.second;
	}
}

//----------------------------------------------------------------------------
EtpClientSession* VtkEtpDocument::getClientSession()
{
	return client_session;
}

//----------------------------------------------------------------------------
int64_t VtkEtpDocument::push_command(const std::string & command)
{
	cout << endl << endl << "command : " << command << endl << endl;
	auto commandTokens = tokenize(command, ' ');

	if (commandTokens[0] == "GetResources") {
		Energistics::Etp::v12::Protocol::Discovery::GetResources mb;
		mb.m_context.m_uri = commandTokens[1];
		mb.m_scope = Energistics::Etp::v12::Datatypes::Object::ContextScopeKind::self;
		mb.m_context.m_depth = 1;
		mb.m_countObjects = true;

		if (commandTokens.size() > 2) {
			if (commandTokens[2] == "self")
				mb.m_scope = Energistics::Etp::v12::Datatypes::Object::ContextScopeKind::self;
			else if (commandTokens[2] == "sources")
				mb.m_scope = Energistics::Etp::v12::Datatypes::Object::ContextScopeKind::sources;
			else if (commandTokens[2] == "sourcesOrSelf")
				mb.m_scope = Energistics::Etp::v12::Datatypes::Object::ContextScopeKind::sourcesOrSelf;
			else if (commandTokens[2] == "targets")
				mb.m_scope = Energistics::Etp::v12::Datatypes::Object::ContextScopeKind::targets;
			else if (commandTokens[2] == "targetsOrSelf")
				mb.m_scope = Energistics::Etp::v12::Datatypes::Object::ContextScopeKind::targetsOrSelf;

			if (commandTokens.size() > 3) {
				mb.m_context.m_depth = std::stoi(commandTokens[3]);

				if (commandTokens.size() > 4) {
					if (commandTokens[4] == "false" || commandTokens[4] == "False" || commandTokens[4] == "FALSE") {
						mb.m_countObjects = false;
					}

					if (commandTokens.size() > 6) {
						mb.m_context.m_dataObjectTypes = tokenize(commandTokens[6], ',');
					}
				}
			}
		}

		if (commandTokens.size() > 5 && (commandTokens[5] == "true" || commandTokens[5] == "True" || commandTokens[5] == "TRUE")) {
			std::static_pointer_cast<EtpFesppDiscoveryProtocolHandlers>(client_session->getDiscoveryProtocolHandlers())->getObjectWhenDiscovered.push_back(client_session->send(mb));
		}
		else {
			client_session->send(mb);
		}
	}
	else if (commandTokens[0] == "GetDataObject") {
		Energistics::Etp::v12::Protocol::Store::GetDataObjects getO;
		std::vector<std::string> tokens = tokenize(commandTokens[1], ',');
		std::map<std::string, std::string> tokenMaps;
		for (size_t i = 0; i < tokens.size(); ++i) {
			tokenMaps[std::to_string(i)] = tokens[i];
		}
		getO.m_uris = tokenMaps;
		auto id = client_session->send(getO);
		client_session->insertMessageIdTobeAnswered(id);
		return id;
	}
	else if (commandTokens.size() == 2) {
		if (commandTokens[0] == "GetSourceObjects") {
			Energistics::Etp::v12::Protocol::Discovery::GetResources mb;
			mb.m_context.m_uri = commandTokens[1];
			mb.m_scope = Energistics::Etp::v12::Datatypes::Object::ContextScopeKind::sources;
			mb.m_context.m_depth = 1;

			auto id = client_session->send(mb);
			std::static_pointer_cast<EtpFesppDiscoveryProtocolHandlers>(client_session->getDiscoveryProtocolHandlers())->getObjectWhenDiscovered.push_back(id);
			client_session->insertMessageIdTobeAnswered(id);
			return id;
		}
		if (commandTokens[0] == "GetTargetObjects") {
			Energistics::Etp::v12::Protocol::Discovery::GetResources mb;
			mb.m_context.m_uri = commandTokens[1];
			mb.m_scope = Energistics::Etp::v12::Datatypes::Object::ContextScopeKind::targets;
			mb.m_context.m_depth = 1;

			auto id = client_session->send(mb);
			std::static_pointer_cast<EtpFesppDiscoveryProtocolHandlers>(client_session->getDiscoveryProtocolHandlers())->getObjectWhenDiscovered.push_back(id);
			client_session->insertMessageIdTobeAnswered(id);
			return id;
		}
	}
	return 0;
}


//----------------------------------------------------------------------------
void VtkEtpDocument::createTree()
{
	Energistics::Etp::v12::Protocol::Discovery::GetResources mb;
	mb.m_context.m_uri = "eml:///";
	mb.m_context.m_depth = 1;
	mb.m_context.m_dataObjectTypes.push_back("resqml20.obj_IjkGridRepresentation");
	mb.m_scope = Energistics::Etp::v12::Datatypes::Object::ContextScopeKind::self; // Could be whatever enumerated values
	cout << "Envoie première requête uri:eml:/// - depth:1 - contentTypes:resqml20.obj_IjkGridRepresentation" << endl;
	int64_t id = client_session->send(mb);
	client_session->insertMessageIdTobeAnswered(id);
}

//----------------------------------------------------------------------------
void VtkEtpDocument::visualize(const std::string & rec_uri)
{
	size_t pos1 = rec_uri.find_last_of("/")+1;
	const std::string type = rec_uri.substr(pos1,rec_uri.find_last_of("(") - pos1);

	pos1 = rec_uri.find_last_of("(")+1;
	const std::string uuid = rec_uri.substr(pos1, rec_uri.find_last_of(")") - pos1);

	if(type == "obj_IjkGridRepresentation") {
		push_command("GetDataObject "+rec_uri);
		push_command("GetSourceObjects "+rec_uri);
		push_command("GetTargetObjects "+rec_uri);

		while(client_session->isWaitingForAnswer()){
		} // wait answers
		setSessionToEtpHdfProxy(client_session);

		RESQML2_0_1_NS::AbstractIjkGridRepresentation const * ijkGrid = client_session->repo.getDataObjectByUuid<RESQML2_0_1_NS::AbstractIjkGridRepresentation>(uuid);
		if (ijkGrid == nullptr) { // Defensive code
			std::cerr << "The requested ETP ijk grid " << uuid << " could not have been retrieved from the ETP server." << std::endl;
		}

		// If the grid is not partial, create a VTK object for the ijk grid and visualize it.
		if (ijkGrid->isPartial()) {
			createTreeVtk(ijkGrid->getUuid(), "etpDocument", ijkGrid->getTitle().c_str(), VtkEpcCommon::PARTIAL);
		}
		else {
			if (uuidToVtkIjkGridRepresentation.find(uuid) == uuidToVtkIjkGridRepresentation.end())
			{
				auto interpretation = ijkGrid->getInterpretation();
				createTreeVtk(ijkGrid->getUuid(), interpretation!=nullptr?interpretation->getUuid():"etpDocument", ijkGrid->getTitle().c_str(), VtkEpcCommon::IJK_GRID);
			}
			if (uuidToVtkIjkGridRepresentation[uuid]->getOutput() == nullptr) {
				uuidToVtkIjkGridRepresentation[uuid]->visualize(uuid);
			}
		}

		// property
		auto valuesPropertySet = ijkGrid->getValuesPropertySet();
		for (const auto & valuesPropery : valuesPropertySet) {
			createTreeVtk(valuesPropery->getUuid(), ijkGrid->getUuid(), valuesPropery->getTitle().c_str(), VtkEpcCommon::PROPERTY);
		}

		// attach representation to EtpDocument VtkMultiBlockDataSet
		if (std::find(attachUuids.begin(), attachUuids.end(), uuid) == attachUuids.end()) {
			detach();
			attachUuids.push_back(uuid);
			attach();
		}
	}
	else if(type=="obj_ContinuousProperty" || type=="obj_DiscreteProperty" ) {
		cout << "uuid = property"<< endl;
		if (uuidIsChildOf[uuid].getParentType() == VtkEpcCommon::IJK_GRID)	{
			cout << "Parent = ijkgrid"<< endl;
			if (uuidToVtkIjkGridRepresentation.find(uuidIsChildOf[uuid].getParent()) != uuidToVtkIjkGridRepresentation.end()){
				cout << "Parent Trouvé"<< endl;
				uuidToVtkIjkGridRepresentation[uuidIsChildOf[uuid].getParent()]->visualize(uuid);
			}
		}
		else {
			std::cout << "Not implemented yet." << std::endl;
		}
	} else {
		std::cout << "Not implemented yet." << std::endl;
	}
}

//----------------------------------------------------------------------------
void VtkEtpDocument::createTreeVtk(const std::string & uuid, const std::string & parent, const std::string & name, const VtkEpcCommon::Resqml2Type & type)
{
	auto tmp = uuidIsChildOf[uuid];
	tmp.setType(type);
	tmp.setUuid(uuid);
	tmp.setParent(parent);
	tmp.setName(name);
	tmp.setTimeIndex(-1);
	tmp.setTimestamp(0);

	tmp.setParentType(uuidIsChildOf[parent].getUuid().empty() ? VtkEpcCommon::INTERPRETATION : uuidIsChildOf[parent].getType());

	uuidIsChildOf[uuid] = tmp;

	if (type == VtkEpcCommon::Resqml2Type::IJK_GRID) {
		if (uuidToVtkIjkGridRepresentation[uuid] == nullptr) {
			uuidToVtkIjkGridRepresentation[uuid] = new VtkIjkGridRepresentation("etpDocument", name, uuid, parent, &client_session->repo, nullptr);
		}
	}
	else if (type == VtkEpcCommon::Resqml2Type::PROPERTY) {
		addPropertyTreeVtk(uuid, parent, name);
	}
	else if (type == VtkEpcCommon::Resqml2Type::PARTIAL) {
	}
}

//----------------------------------------------------------------------------
void VtkEtpDocument::addPropertyTreeVtk(const std::string & uuid, const std::string & parent, const std::string & name)
{
	if (uuidIsChildOf[parent].getType() == VtkEpcCommon::Resqml2Type::IJK_GRID) {
		uuidToVtkIjkGridRepresentation[parent]->createTreeVtk(uuid, parent, name, uuidIsChildOf[uuid].getType());
	}
}

//----------------------------------------------------------------------------
void VtkEtpDocument::unvisualize(const std::string & rec_uri)
{
	if(representationMode) {
		const size_t pos1 = rec_uri.find_last_of("(")+1;
		const size_t length = rec_uri.find_last_of(")") - pos1;
		const std::string uuid = rec_uri.substr(pos1,length);

		remove(uuid);
	}
}

//----------------------------------------------------------------------------
void VtkEtpDocument::remove(const std::string & uuid)
{
	auto uuidtoAttach = uuid;
	if (uuidIsChildOf[uuid].getType() == VtkEpcCommon::PROPERTY) {
		uuidtoAttach = uuidIsChildOf[uuid].getParent();
	}

	if (std::find(attachUuids.begin(), attachUuids.end(), uuidtoAttach) != attachUuids.end()) {
		if (uuidIsChildOf[uuidtoAttach].getType() == VtkEpcCommon::IJK_GRID) {
			uuidToVtkIjkGridRepresentation[uuidtoAttach]->remove(uuid);
			if (uuid == uuidtoAttach) {
				detach();
				attachUuids.erase(std::find(attachUuids.begin(), attachUuids.end(), uuid));
				attach();
			}
		}
	}
}

//----------------------------------------------------------------------------
long VtkEtpDocument::getAttachmentPropertyCount(const std::string & uuid, const VtkEpcCommon::FesppAttachmentProperty propertyUnit)
{
	return 0;
}

//----------------------------------------------------------------------------
void VtkEtpDocument::attach()
{
	for (size_t newBlockIndex = 0; newBlockIndex < attachUuids.size(); ++newBlockIndex) {
		std::string uuid = attachUuids[newBlockIndex];
		vtkOutput->SetBlock(newBlockIndex, uuidToVtkIjkGridRepresentation[uuid]->getOutput());
		vtkOutput->GetMetaData(newBlockIndex)->Set(vtkCompositeDataSet::NAME(), uuidToVtkIjkGridRepresentation[uuid]->getUuid().c_str());
	}
}

//----------------------------------------------------------------------------
void VtkEtpDocument::receive_resources_tree(const std::string & rec_uri, const std::string & rec_name, const std::string & dataobjectType, int32_t sourceCount)
{
	if (dataobjectType == "resqml20.obj_IjkGridRepresentation"){
		VtkEpcCommon leaf;
		leaf.setName(rec_name);
		leaf.setUuid(rec_uri);
		leaf.setType(VtkEpcCommon::IJK_GRID);
		leaf.setParent("EtpDoc");
		leaf.setParentType(VtkEpcCommon::INTERPRETATION);
		leaf.setTimeIndex(-1);
		leaf.setTimestamp(0);

		//push_command("GetResources " + rec_uri + " sources 1 false resqml20.obj_ContinuousProperty");
		Energistics::Etp::v12::Protocol::Discovery::GetResources mb;
		mb.m_context.m_uri = rec_uri;
		mb.m_scope = Energistics::Etp::v12::Datatypes::Object::ContextScopeKind::sources;
		mb.m_context.m_depth = 1;
		mb.m_context.m_dataObjectTypes.push_back("resqml20.obj_ContinuousProperty");
		client_session->send(mb);
		response_queue.push_back(leaf);
		treeView.push_back(leaf);

		//push_command("GetResources " + rec_uri + " sources 1 false application/x-resqml+xml;version=2.0;type=obj_DiscreteProperty");
		mb.m_scope = Energistics::Etp::v12::Datatypes::Object::ContextScopeKind::sources;
		mb.m_context.m_dataObjectTypes[0] = "resqml20.obj_DiscreteProperty";
		client_session->send(mb);
		response_queue.push_back(leaf);
		treeView.push_back(leaf);
	}
	else if (dataobjectType == "resqml20.obj_ContinuousProperty" || dataobjectType == "resqml20.obj_DiscreteProperty") {
		VtkEpcCommon leaf;
		leaf.setName(rec_name);
		leaf.setUuid(rec_uri);
		leaf.setType(VtkEpcCommon::PROPERTY);
		leaf.setParent(response_queue.front().getUuid());
		leaf.setParentType(response_queue.front().getType());
		leaf.setTimeIndex(-1);
		leaf.setTimestamp(0);

		treeView.push_back(leaf);
		response_queue.pop_front();
	}
	else {
		response_queue.pop_front();
	}


	if(response_queue.empty()) {
		/*
		client_session->close();
		client_session->epcDoc.close();
		 */
		getPQEtpPanel()->setEtpTreeView(getTreeView());
		loading = false;
	}
}

void VtkEtpDocument::receive_nbresources_tree(size_t nb_resources)
{
	if (!response_queue.empty()){
		auto copy = response_queue.front();
		if (nb_resources > 0) {
			for (size_t nb = 1; nb < nb_resources; ++nb){
				response_queue.push_front(copy);
			}
		}
		else if (response_queue.empty()) {
			/*				client_session->close();
			client_session->epcDoc.close();*/
			getPQEtpPanel()->setEtpTreeView(getTreeView());
			loading = false;
		}
	}
}

//----------------------------------------------------------------------------
std::vector<VtkEpcCommon> VtkEtpDocument::getTreeView() const
{
	return treeView;
}

//----------------------------------------------------------------------------
vtkSmartPointer<vtkMultiBlockDataSet> VtkEtpDocument::getVisualization() const
{
	return vtkOutput;
}

//----------------------------------------------------------------------------
bool VtkEtpDocument::isLoading() const
{
	return loading;
}
