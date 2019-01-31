#include "etp/VtkEtpDocument.h"

#include <thread>

// include Vtk
#include <vtkInformation.h>
#include <QApplication>
#include <vtkSmartPointer.h>
#include <vtkMultiBlockDataSet.h>

// include Fespp
#include "EtpClientSession.h"
#include "PQEtpPanel.h"
#include "etp/EtpFesppStoreProtocolHandlers.h"
#include "etp/EtpFesppDiscoveryProtocolHandlers.h"
#include "VTK/VtkIjkGridRepresentation.h"

#include <etp/EtpHdfProxy.h>
#include "resqml2/AbstractRepresentation.h"
#include "common/AbstractObject.h"
#include "resqml2/AbstractFeatureInterpretation.h"

#include <resqml2_0_1/AbstractIjkGridRepresentation.h>
#include <resqml2_0_1/ContinuousPropertySeries.h>

PQEtpPanel* getPQEtpPanel()
{
	PQEtpPanel *panel = 0;
	foreach(QWidget *widget, qApp->topLevelWidgets()) {
		panel = widget->findChild<PQEtpPanel *>();
		if (panel) {
			break;
		}
	}
	return panel;
}

void setSessionToEtpHdfProxy(EtpClientSession* myOwnEtpSession)
{
	COMMON_NS::EpcDocument& epcDoc = myOwnEtpSession->epcDoc;
	for (const auto & hdfProxy : epcDoc.getHdfProxySet()) {
		ETP_NS::EtpHdfProxy* etpHdfProxy = dynamic_cast<ETP_NS::EtpHdfProxy*>(hdfProxy);
		if (etpHdfProxy != nullptr && etpHdfProxy->getSession() == nullptr) {
			etpHdfProxy->setSession(myOwnEtpSession->getIoContext(), myOwnEtpSession->getHost(), myOwnEtpSession->getPort(), myOwnEtpSession->getTarget());
		}
	}
}

void startio(VtkEtpDocument *etp_document, std::string ipAddress, std::string port, const VtkEpcCommon::modeVtkEpc & mode)
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


	protocol.m_protocol = Energistics::Etp::v12::Datatypes::Protocol::DirectedDiscovery;
	protocol.m_protocolVersion = protocolVersion;
	protocol.m_role = "store";
	requestedProtocols.push_back(protocol);

	auto work = boost::asio::make_work_guard(ioc);
	std::thread thread([&ioc]() {
		ioc.run();
	});

	auto client_session_sharedPtr = std::make_shared<EtpClientSession>(ioc, ipAddress, port, requestedProtocols, supportedObjects, etp_document, mode);
	client_session_sharedPtr->run();

	etp_document->setClientSession(client_session_sharedPtr.get());

	work.reset();
	thread.join();
}

//----------------------------------------------------------------------------
VtkEtpDocument::VtkEtpDocument(const std::string & ipAddress, const std::string & port, const VtkEpcCommon::modeVtkEpc & mode) :
		VtkResqml2MultiBlockDataSet("EtpDocument", "EtpDocument", "EtpDocument", "", 0, 0)
{
	treeViewMode = (mode==VtkEpcCommon::Both || mode==VtkEpcCommon::TreeView);
	representationMode = (mode==VtkEpcCommon::Both || mode==VtkEpcCommon::Representation);

	vtkOutput = vtkSmartPointer<vtkMultiBlockDataSet>::New();

	std::thread askUserThread(startio, this, ipAddress, port, mode);
	askUserThread.detach(); // Detach the thread since we don't want it to be a blocking one.

	last_id = 0;
}

//----------------------------------------------------------------------------
VtkEtpDocument::~VtkEtpDocument()
{
	if (client_session)	{
		client_session->close();
		client_session->epcDoc.close();
	}
}

//----------------------------------------------------------------------------
int64_t VtkEtpDocument::push_command(const std::string & command)
{
	cout << endl << endl << "command : " << command << endl << endl;
	auto commandTokens = tokenize(command, ' ');

	if (commandTokens[0] == "GetTreeResources") {
		Energistics::Etp::v12::Protocol::Discovery::GetTreeResources mb;
		mb.m_context.m_uri = commandTokens[1];
		mb.m_context.m_depth = 1;

		if (commandTokens.size() > 2) {
			mb.m_context.m_depth = std::stoi(commandTokens[2]);

			if (commandTokens.size() > 4) {
				mb.m_context.m_contentTypes = tokenize(commandTokens[4], ',');
			}
		}
		if (commandTokens.size() > 3 && (commandTokens[3] == "true" || commandTokens[3] == "True" || commandTokens[3] == "TRUE")) {
			std::static_pointer_cast<EtpFesppDiscoveryProtocolHandlers>(client_session->getDiscoveryProtocolHandlers())->getObjectWhenDiscovered.push_back(client_session->send(mb));
		}
		else {
			client_session->send(mb);
		}
	}
	else if (commandTokens[0] == "GetGraphResources") {
		Energistics::Etp::v12::Protocol::Discovery::GetGraphResources mb;
		mb.m_context.m_uri = commandTokens[1];
		mb.m_scope = Energistics::Etp::v12::Datatypes::Object::ContextScopeKind::self;
		mb.m_context.m_depth = 1;
		mb.m_groupByType = false;

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

				if (commandTokens.size() > 5) {
					mb.m_context.m_contentTypes = tokenize(commandTokens[5], ',');
				}
			}
		}

		if (commandTokens.size() > 4 && (commandTokens[4] == "true" || commandTokens[4] == "True" || commandTokens[4] == "TRUE")) {
			std::static_pointer_cast<EtpFesppDiscoveryProtocolHandlers>(client_session->getDiscoveryProtocolHandlers())->getObjectWhenDiscovered.push_back(client_session->send(mb));
		}
		else {
			client_session->send(mb);
		}
	}
	else if (commandTokens[0] == "GetDataObject") {
		Energistics::Etp::v12::Protocol::Store::GetDataObjects getO;
		getO.m_uris = tokenize(commandTokens[1], ',');
		auto id = client_session->send(getO);
		std::static_pointer_cast<EtpFesppDiscoveryProtocolHandlers>(client_session->getDiscoveryProtocolHandlers())->getObjectWhenDiscovered.push_back(id);
		client_session->newAnsweredMessages(id);
		return id;
	}
	else if (commandTokens.size() == 2) {
		if (commandTokens[0] == "GetSourceObjects") {
			Energistics::Etp::v12::Protocol::Discovery::GetGraphResources mb;
			mb.m_context.m_uri = commandTokens[1];
			mb.m_scope = Energistics::Etp::v12::Datatypes::Object::ContextScopeKind::sources;
			mb.m_context.m_depth = 1;
			mb.m_groupByType = false;

			auto id = client_session->send(mb);
			std::static_pointer_cast<EtpFesppDiscoveryProtocolHandlers>(client_session->getDiscoveryProtocolHandlers())->getObjectWhenDiscovered.push_back(id);
			client_session->newAnsweredMessages(id);
			return id;
		}
		 if (commandTokens[0] == "GetTargetObjects") {
			Energistics::Etp::v12::Protocol::Discovery::GetGraphResources mb;
			mb.m_context.m_uri = commandTokens[1];
			mb.m_scope = Energistics::Etp::v12::Datatypes::Object::ContextScopeKind::targets;
			mb.m_context.m_depth = 1;
			mb.m_groupByType = false;

			auto id = client_session->send(mb);
			std::static_pointer_cast<EtpFesppDiscoveryProtocolHandlers>(client_session->getDiscoveryProtocolHandlers())->getObjectWhenDiscovered.push_back(id);
			client_session->newAnsweredMessages(id);
			return id;
		}
	}
	return 0;
}


//----------------------------------------------------------------------------
void VtkEtpDocument::createTree()
{
	// filter: ijkGrid
	push_command("GetTreeResources eml:// 3 false application/x-resqml+xml;version=2.0;type=obj_IjkGridRepresentation");
}

//----------------------------------------------------------------------------
void VtkEtpDocument::visualize(const std::string & rec_uri)
{
	auto pos1 = rec_uri.find_last_of("/")+1;
	std::string type = rec_uri.substr(pos1,rec_uri.find_last_of("(") - pos1);

	pos1 = rec_uri.find_last_of("(")+1;
	std::string uuid = rec_uri.substr(pos1, rec_uri.find_last_of(")") - pos1);

	std::string uuidParent = "etpDocument";
	if(type=="obj_IjkGridRepresentation") {
		auto it = uuidToVtkIjkGridRepresentation.find (uuid);
		if ( it == uuidToVtkIjkGridRepresentation.end()){
			push_command("GetDataObject "+rec_uri);
			push_command("GetSourceObjects "+rec_uri);
			push_command("GetTargetObjects "+rec_uri);

			while(client_session->allReceived() == false){
			} // wait answers

			setSessionToEtpHdfProxy(client_session);
			COMMON_NS::EpcDocument& epcDoc = client_session->epcDoc;

			auto ijkGridSet = epcDoc.getIjkGridRepresentationSet();

			for (const auto & ijkGrid : ijkGridSet) {

				auto interpretation = ijkGrid->getInterpretation();
				if (interpretation!=nullptr) {
					uuidParent = interpretation->getUuid();
				}

				uuidIsChildOf[ijkGrid->getUuid()] = new VtkEpcCommon();

				if (ijkGrid->isPartial()) {
					std::cout << "Partial Ijk Grid " << ijkGrid->getTitle() << " : " << ijkGrid->getUuid() << std::endl;
					createTreeVtk(ijkGrid->getUuid(), uuidParent, ijkGrid->getTitle().c_str(), VtkEpcCommon::PARTIAL);
				}
				else {
					createTreeVtk(ijkGrid->getUuid(), uuidParent, ijkGrid->getTitle().c_str(), VtkEpcCommon::IJK_GRID);
				}
				//property
				auto valuesPropertySet = ijkGrid->getValuesPropertySet();
				for (size_t i = 0; i < valuesPropertySet.size(); ++i)	{
					uuidIsChildOf[valuesPropertySet[i]->getUuid()] = new VtkEpcCommon();
					createTreeVtk(valuesPropertySet[i]->getUuid(), ijkGrid->getUuid(), valuesPropertySet[i]->getTitle().c_str(), VtkEpcCommon::PROPERTY);
				}

				uuidToVtkIjkGridRepresentation[uuid]->visualize(uuid);

				// attach representation to EtpDocument VtkMultiBlockDataSet
				if (std::find(attachUuids.begin(), attachUuids.end(), uuid) == attachUuids.end())	{
					this->detach();
					attachUuids.push_back(uuid);
					this->attach();
				}
			}
		}
		else if (uuidToVtkIjkGridRepresentation[uuid]->getOutput()==nullptr) {
			push_command("GetObject "+rec_uri);
			push_command("GetSourceObjects "+rec_uri);
			push_command("GetTargetObjects "+rec_uri);

			while(client_session->allReceived() == false){
			} // wait answers

			setSessionToEtpHdfProxy(client_session);
			COMMON_NS::EpcDocument& epcDoc = client_session->epcDoc;

			auto ijkGridSet = epcDoc.getIjkGridRepresentationSet();

			for (const auto & ijkGrid : ijkGridSet) {
				auto interpretation = ijkGrid->getInterpretation();
				if (interpretation!=nullptr) {
					uuidParent = interpretation->getUuid();
				}

				uuidIsChildOf[ijkGrid->getUuid()] = new VtkEpcCommon();
				createTreeVtk(ijkGrid->getUuid(), uuidParent, ijkGrid->getTitle().c_str(), ijkGrid->isPartial() ? VtkEpcCommon::PARTIAL : VtkEpcCommon::IJK_GRID);

				//property
				auto valuesPropertySet = ijkGrid->getValuesPropertySet();
				for (const auto & valuesPropery : valuesPropertySet)	{
					uuidIsChildOf[valuesPropery->getUuid()] = new VtkEpcCommon();
					createTreeVtk(valuesPropery->getUuid(), ijkGrid->getUuid(), valuesPropery->getTitle().c_str(), VtkEpcCommon::PROPERTY);
				}

				uuidToVtkIjkGridRepresentation[uuid]->visualize(uuid);

				// attach representation to EtpDocument VtkMultiBlockDataSet
				if (std::find(attachUuids.begin(), attachUuids.end(), uuid) == attachUuids.end())	{
					this->detach();
					attachUuids.push_back(uuid);
					this->attach();
				}
			}
		}
	}
	else if(type=="obj_ContinuousProperty" || type=="obj_DiscreteProperty" ) {
		if (uuidIsChildOf[uuid]->getParentType() == VtkEpcCommon::IJK_GRID)	{
			uuidToVtkIjkGridRepresentation[uuidIsChildOf[uuid]->getParent()]->visualize(uuid);
		}
		else if (uuidIsChildOf[uuid]->getParentType() == VtkEpcCommon::PARTIAL)	{
			std::cout << "properties: " << uuid << " is on partial Grid and is not yet implemented " << std::endl;
		}
	}
}

//----------------------------------------------------------------------------
void VtkEtpDocument::createTreeVtk(const std::string & uuid, const std::string & parent, const std::string & name, const VtkEpcCommon::Resqml2Type & type)
{
	uuidIsChildOf[uuid]->setType( type);
	uuidIsChildOf[uuid]->setUuid(uuid);
	uuidIsChildOf[uuid]->setParent(parent);
	uuidIsChildOf[uuid]->setName(name);
	uuidIsChildOf[uuid]->setTimeIndex(-1);
	uuidIsChildOf[uuid]->setTimestamp(0);

	uuidIsChildOf[uuid]->setParentType(uuidIsChildOf[parent] ? uuidIsChildOf[parent]->getType() : VtkEpcCommon::INTERPRETATION);

	if (type == VtkEpcCommon::Resqml2Type::IJK_GRID) {
		uuidToVtkIjkGridRepresentation[uuid] = new VtkIjkGridRepresentation("etpDocument", name, uuid, parent, &client_session->epcDoc, nullptr);
	}
	else if (type == VtkEpcCommon::Resqml2Type::PROPERTY) {
		addPropertyTreeVtk(uuid, parent, name);
	}
	else if (type == VtkEpcCommon::Resqml2Type::PARTIAL) {
		cout << " PARTIAL : " << uuid << endl;
	}
}

//----------------------------------------------------------------------------
void VtkEtpDocument::addPropertyTreeVtk(const std::string & uuid, const std::string & parent, const std::string & name)
{
	if (uuidIsChildOf[parent]->getType() == VtkEpcCommon::IJK_GRID) {
		uuidToVtkIjkGridRepresentation[parent]->createTreeVtk(uuid, parent, name, uuidIsChildOf[uuid]->getType());
	}
}



//----------------------------------------------------------------------------
void VtkEtpDocument::unvisualize(const std::string & rec_uri)
{
	if(representationMode) {
		auto pos1 = rec_uri.find_last_of("(")+1;
		auto lenght = rec_uri.find_last_of(")") - pos1;
		std::string uuid = rec_uri.substr(pos1,lenght);

		this->remove(uuid);
	}
}

//----------------------------------------------------------------------------
void VtkEtpDocument::remove(const std::string & uuid)
{
	auto uuidtoAttach = uuid;
	if (uuidIsChildOf[uuid]->getType() == VtkEpcCommon::PROPERTY) {
		uuidtoAttach = uuidIsChildOf[uuid]->getParent();
	}

	if (std::find(attachUuids.begin(), attachUuids.end(), uuidtoAttach) != attachUuids.end()) {
		if (uuidIsChildOf[uuidtoAttach]->getType() == VtkEpcCommon::IJK_GRID) {
			uuidToVtkIjkGridRepresentation[uuidtoAttach]->remove(uuid);
			if (uuid == uuidtoAttach) {
				this->detach();
				attachUuids.erase(std::find(attachUuids.begin(), attachUuids.end(), uuid));
				this->attach();
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
void VtkEtpDocument::receive_resources_tree(const std::string & rec_uri, const std::string & rec_name, const std::string & contentType, const int32_t & sourceCount)
{
	cout << "TreeView : " << rec_uri << " " << contentType << " " << sourceCount << endl;

	if (contentType == "application/x-resqml+xml;version=2.0;type=obj_IjkGridRepresentation"){
		auto leaf = new VtkEpcCommon();
		leaf->setName(rec_name);
		leaf->setUuid(rec_uri);
		leaf->setType(VtkEpcCommon::IJK_GRID);
		leaf->setParent("EtpDoc");
		leaf->setParentType(VtkEpcCommon::INTERPRETATION);
		leaf->setTimeIndex(-1);
		leaf->setTimestamp(0);

		push_command("GetGraphResources "+rec_uri+" sources 1 false application/x-resqml+xml;version=2.0;type=obj_ContinuousProperty");
		response_queue.push_back(leaf);
		treeView.push_back(leaf);
	}
	else if (contentType == "application/x-resqml+xml;version=2.0;type=obj_ContinuousProperty"){
		auto leaf = new VtkEpcCommon();
		leaf->setName(rec_name);
		leaf->setUuid(rec_uri);
		leaf->setType(VtkEpcCommon::PROPERTY);
		leaf->setParent(response_queue.front()->getUuid());
		leaf->setParentType(response_queue.front()->getType());
		leaf->setTimeIndex(-1);
		leaf->setTimestamp(0);

		treeView.push_back(leaf);
		response_queue.pop_front();
	}
	else {
		response_queue.pop_front();
	}

	if(response_queue.empty()) {
		client_session->close();
		client_session->epcDoc.close();
		getPQEtpPanel()->setEtpTreeView(this->getTreeView());
	}
}

void VtkEtpDocument::receive_nbresources_tree(size_t nb_resources)
{
	if (!response_queue.empty()){
		if (nb_resources > 0) {
			auto copy = response_queue.front();
			for (size_t nb = 1; nb < nb_resources; ++nb){
				response_queue.push_front(copy);
			}
		}
		else {
			response_queue.pop_front();
			if(response_queue.empty()) {
				client_session->close();
				client_session->epcDoc.close();
				getPQEtpPanel()->setEtpTreeView(this->getTreeView());
			}
		}
	}
}

//----------------------------------------------------------------------------
std::vector<VtkEpcCommon*> VtkEtpDocument::getTreeView() const
{
	return treeView;
}

//----------------------------------------------------------------------------
vtkSmartPointer<vtkMultiBlockDataSet> VtkEtpDocument::getVisualization() const
{
	return vtkOutput;
}
