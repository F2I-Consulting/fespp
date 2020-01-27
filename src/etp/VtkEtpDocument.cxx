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

#include <fesapi/resqml2/AbstractFeatureInterpretation.h>
#include <fesapi/resqml2_0_1/AbstractIjkGridRepresentation.h>

#include <fesapi/etp/EtpHdfProxy.h>

// include Fespp
#include "EtpClientSession.h"
#include "../PQEtpPanel.h"
#include "EtpFesppStoreProtocolHandlers.h"
#include "IjkGridRepDiscoveryHandler.h"
#include "GetWhenDiscoverHandler.h"
#include "../VTK/VtkIjkGridRepresentation.h"

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

void setSessionToEtpHdfProxy(std::shared_ptr<EtpClientSession> myOwnEtpSession)
{
	for (const auto & hdfProxy : myOwnEtpSession->repo.getHdfProxySet()) {
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

	auto clientSession = std::make_shared<EtpClientSession>(ipAddress, port, "/", "", requestedProtocols, supportedObjects, mode);
	clientSession->setCoreProtocolHandlers(std::make_shared<ETP_NS::CoreHandlers>(clientSession));
	clientSession->setDiscoveryProtocolHandlers(std::make_shared<ETP_NS::DiscoveryHandlers>(clientSession));
	clientSession->setStoreProtocolHandlers(std::make_shared<EtpFesppStoreProtocolHandlers>(clientSession));
	clientSession->setDataArrayProtocolHandlers(std::make_shared<ETP_NS::DataArrayHandlers>(clientSession));
	clientSession->setStoreNotificationProtocolHandlers(std::make_shared<ETP_NS::StoreNotificationHandlers>(clientSession));
	etp_document->setClientSession(clientSession);
	clientSession->setConnectionError(!clientSession->run());
}

//----------------------------------------------------------------------------
VtkEtpDocument::VtkEtpDocument(const std::string & ipAddress, const std::string & port, const VtkEpcCommon::modeVtkEpc & mode) :
	VtkResqml2MultiBlockDataSet("EtpDocument", "EtpDocument", "EtpDocument", "", 0, 0), client_session(nullptr),
	treeViewMode(mode == VtkEpcCommon::Both || mode == VtkEpcCommon::TreeView), representationMode(mode == VtkEpcCommon::Both || mode == VtkEpcCommon::Representation)
{
	vtkOutput = vtkSmartPointer<vtkMultiBlockDataSet>::New();

	std::thread runSessionThread(startio, this, ipAddress, port, mode);
	runSessionThread.detach(); // Detach the thread since we don't want it to be a blocking one.
}

//----------------------------------------------------------------------------
VtkEtpDocument::~VtkEtpDocument()
{
	if (client_session != nullptr)	{
		client_session->close();
	}
	for(auto i : uuidToVtkIjkGridRepresentation) {
		delete i.second;
	}
}

//----------------------------------------------------------------------------
void VtkEtpDocument::createTree()
{
	// GetResources uri:eml:/// - depth:1 - contentTypes:resqml20.obj_IjkGridRepresentation;
	Energistics::Etp::v12::Protocol::Discovery::GetResources mb;
	mb.m_context.m_uri = "eml:///";
	mb.m_context.m_depth = 1;
	mb.m_context.m_dataObjectTypes.push_back("resqml20.obj_IjkGridRepresentation");
	mb.m_scope = Energistics::Etp::v12::Datatypes::Object::ContextScopeKind::self; // Could be whatever enumerated values
	const int64_t id = client_session->sendWithSpecificHandler(mb, std::make_shared<IjkGridRepDiscoveryHandler>(this));
	client_session->insertMessageIdTobeAnswered(id);
}

//----------------------------------------------------------------------------
void VtkEtpDocument::visualize(const std::string & rec_uri)
{
	size_t pos1 = rec_uri.find_last_of("/")+1;
	const std::string type = rec_uri.substr(pos1,rec_uri.find_last_of("(") - pos1);

	pos1 = rec_uri.find_last_of("(")+1;
	const std::string uuid = rec_uri.substr(pos1, rec_uri.find_last_of(")") - pos1);

	// Get the dataobject and its targets
	Energistics::Etp::v12::Protocol::Discovery::GetResources mb;
	mb.m_context.m_uri = rec_uri;
	mb.m_context.m_depth = 1;
	mb.m_scope = Energistics::Etp::v12::Datatypes::Object::ContextScopeKind::targetsOrSelf;
	const int64_t id = client_session->sendWithSpecificHandler(mb, std::make_shared<GetWhenDiscoverHandler>(client_session));
	client_session->insertMessageIdTobeAnswered(id);
	while (client_session->isWaitingForAnswer()) {} // wait answers
	setSessionToEtpHdfProxy(client_session);

	if(type == "resqml20.obj_IjkGridRepresentation") {
		RESQML2_0_1_NS::AbstractIjkGridRepresentation const * ijkGrid = client_session->repo.getDataObjectByUuid<RESQML2_0_1_NS::AbstractIjkGridRepresentation>(uuid);

		// If the grid is not partial, create a VTK object for the ijk grid and visualize it.
		if (ijkGrid->isPartial()) {
			createTreeVtk(ijkGrid->getUuid(), "etpDocument", ijkGrid->getTitle().c_str(), VtkEpcCommon::PARTIAL);
		}
		else {
			if (uuidToVtkIjkGridRepresentation.find(uuid) == uuidToVtkIjkGridRepresentation.end()) {
				auto interpretation = ijkGrid->getInterpretation();
				createTreeVtk(ijkGrid->getUuid(), interpretation != nullptr ? interpretation->getUuid() : "etpDocument", ijkGrid->getTitle().c_str(), VtkEpcCommon::IJK_GRID);
			}
			if (uuidToVtkIjkGridRepresentation[uuid]->getOutput() == nullptr) {
				uuidToVtkIjkGridRepresentation[uuid]->visualize(uuid);
			}
		}

		// attach representation to EtpDocument VtkMultiBlockDataSet
		if (std::find(attachUuids.begin(), attachUuids.end(), uuid) == attachUuids.end()) {
			detach();
			attachUuids.push_back(uuid);
			attach();
		}
	}
	else if(type=="resqml20.obj_ContinuousProperty" || type=="resqml20.obj_DiscreteProperty" ) {
		RESQML2_NS::AbstractValuesProperty const * prop = client_session->repo.getDataObjectByUuid<RESQML2_NS::AbstractValuesProperty>(uuid);

		// Build a VTK property in the property node in the treeview if necessary
		const std::string ijkGridUuid = prop->getRepresentationUuid();
		createTreeVtk(prop->getUuid(), ijkGridUuid, prop->getTitle().c_str(), VtkEpcCommon::PROPERTY);
		uuidToVtkIjkGridRepresentation[ijkGridUuid]->visualize(uuid);
	}
	else {
		std::cout << "Not implemented yet." << std::endl;
	}
}

//----------------------------------------------------------------------------
void VtkEtpDocument::createTreeVtk(const std::string & uuid, const std::string & parent, const std::string & name, VtkEpcCommon::Resqml2Type type)
{
	VtkEpcCommon& tmp = uuidIsChildOf[uuid];
	tmp.setType(type);
	tmp.setUuid(uuid);
	tmp.setParent(parent);
	tmp.setName(name);
	tmp.setTimeIndex(-1);
	tmp.setTimestamp(0);
	tmp.setParentType(uuidIsChildOf[parent].getUuid().empty() ? VtkEpcCommon::INTERPRETATION : uuidIsChildOf[parent].getType());

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
	std::string uuidtoAttach = uuidIsChildOf[uuid].getType() == VtkEpcCommon::PROPERTY ? uuidIsChildOf[uuid].getParent() : uuid;

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
long VtkEtpDocument::getAttachmentPropertyCount(const std::string & uuid, VtkEpcCommon::FesppAttachmentProperty propertyUnit)
{
	return 0;
}

//----------------------------------------------------------------------------
void VtkEtpDocument::attach()
{
	if (attachUuids.size() > (std::numeric_limits<unsigned int>::max)()) {
		throw std::range_error("Too much attached uuids");
	}

	for (unsigned int newBlockIndex = 0; newBlockIndex < attachUuids.size(); ++newBlockIndex) {
		const std::string& uuid = attachUuids[newBlockIndex];
		vtkOutput->SetBlock(newBlockIndex, uuidToVtkIjkGridRepresentation[uuid]->getOutput());
		vtkOutput->GetMetaData(newBlockIndex)->Set(vtkCompositeDataSet::NAME(), uuidToVtkIjkGridRepresentation[uuid]->getUuid().c_str());
	}
}

//----------------------------------------------------------------------------
vtkSmartPointer<vtkMultiBlockDataSet> VtkEtpDocument::getVisualization() const
{
	return vtkOutput;
}
