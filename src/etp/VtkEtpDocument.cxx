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
#include "etp/EtpFesppDirectedDiscoveryProtocolHandlers.h"
#include "VTK/VtkIjkGridRepresentation.h"

#include <etp/EtpHdfProxy.h>
#include <resqml2_0_1/AbstractIjkGridRepresentation.h>
#include <resqml2_0_1/ContinuousPropertySeries.h>

namespace
{
PQEtpPanel* getPQEtpPanel()
{
	PQEtpPanel *panel = 0;
	foreach(QWidget *widget, qApp->topLevelWidgets())
	{
		panel = widget->findChild<PQEtpPanel *>();

		if (panel)
		{
			break;
		}
	}
	return panel;
}
}

void setSessionToEtpHdfProxy(EtpClientSession* myOwnEtpSession) {
	COMMON_NS::EpcDocument& epcDoc = myOwnEtpSession->epcDoc;
	cerr << "setSessionToEtpHdfProxy " << endl;
	for (const auto & hdfProxy : epcDoc.getHdfProxySet())
	{
		ETP_NS::EtpHdfProxy* etpHdfProxy = dynamic_cast<ETP_NS::EtpHdfProxy*>(hdfProxy);
		if (etpHdfProxy != nullptr && etpHdfProxy->getSession() == nullptr) {
			cerr << "setSessionToEtpHdfProxy status "<< myOwnEtpSession->getIoContext().stopped() << endl;
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

	// Requested protocol
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

	// We run the io_service off in its own thread so that it operates completely asynchronously with respect to the rest of the program.
	// This is particularly important regarding "std::future" usage in DataArrayBlockingSession
	auto work = boost::asio::make_work_guard(ioc);
	std::thread thread([&ioc]() {
		std::cerr << "Start IOC" << std::endl;
		ioc.run(); // Run the I/O service. The call will never return since we have used boost::asio::make_work_guard. We need to reset the worker if we want it to return.
		std::cerr << "End IOC" << std::endl;
	});

	auto client_session_sharedPtr = std::make_shared<EtpClientSession>(ioc, ipAddress, port, requestedProtocols, supportedObjects, etp_document, mode);
	client_session_sharedPtr->run();

	etp_document->setClientSession(client_session_sharedPtr.get());

	// Resetting the work make ioc (in a separate thread) return when there will no more be any uncomplete operations (such as a reading operation for example)
	// ioc does no more honor boost::asio::make_work_guard.
	work.reset();
	// Wait for ioc.run() (in the other thread) to return
	thread.join();
}

//----------------------------------------------------------------------------
VtkEtpDocument::VtkEtpDocument(const std::string & ipAddress, const std::string & port, const VtkEpcCommon::modeVtkEpc & mode) :
														VtkResqml2MultiBlockDataSet("EtpDocument", "EtpDocument", "EtpDocument", "", 0, 0)
{
	treeViewMode=false;
	representationMode=false;
	vtkOutput = vtkSmartPointer<vtkMultiBlockDataSet>::New();

	if (mode==VtkEpcCommon::Both || mode==VtkEpcCommon::TreeView)
	{
		treeViewMode=true;
	}
	if (mode==VtkEpcCommon::Both || mode==VtkEpcCommon::Representation)
	{
		representationMode=true;
	}

	idMessageCurrent = 0;

	cout << "start thread mode "<<treeViewMode << " - "<<representationMode<< endl;
	//	startio(this, ipAddress, port, mode);
	std::thread askUserThread(startio, this, ipAddress, port, mode);
	askUserThread.detach(); // Detach the thread since we don't want it to be a blocking one.

#ifdef _WIN32
	_CrtDumpMemoryLeaks();
#endif

}

//----------------------------------------------------------------------------
VtkEtpDocument::~VtkEtpDocument()
{
	if (client_session)
	{
		client_session->close();
		client_session->epcDoc.close();
	}
	//	this->push_command("quit");
}

//----------------------------------------------------------------------------
int64_t VtkEtpDocument::push_command(const std::string & command)
{

	++idMessageCurrent;
	cout << "void VtkEtpDocument::push_command(const std::string & " << command<< ")" <<endl;
	auto commandTokens = tokenize(command, ' ');


	if (!commandTokens.empty() && commandTokens[0] == "GetResources") {
		Energistics::Etp::v12::Protocol::Discovery::GetResources2 mb;
		mb.m_context.m_uri = commandTokens[1];
		mb.m_context.m_scope = Energistics::Etp::v12::Datatypes::Object::ContextScopeKind::self;
		mb.m_context.m_depth = 0;

		if (commandTokens.size() > 2) {
			if (commandTokens[2] == "self")
				mb.m_context.m_scope = Energistics::Etp::v12::Datatypes::Object::ContextScopeKind::self;
			else if (commandTokens[2] == "sources")
				mb.m_context.m_scope = Energistics::Etp::v12::Datatypes::Object::ContextScopeKind::sources;
			else if (commandTokens[2] == "sourcesOrSelf")
				mb.m_context.m_scope = Energistics::Etp::v12::Datatypes::Object::ContextScopeKind::sourcesOrSelf;
			else if (commandTokens[2] == "targets")
				mb.m_context.m_scope = Energistics::Etp::v12::Datatypes::Object::ContextScopeKind::targets;
			else if (commandTokens[2] == "targetsOrSelf")
				mb.m_context.m_scope = Energistics::Etp::v12::Datatypes::Object::ContextScopeKind::targetsOrSelf;

			if (commandTokens.size() > 3) {
				mb.m_context.m_depth = std::stoi(commandTokens[3]);

				if (commandTokens.size() > 4) {
					mb.m_context.m_contentTypes = tokenize(commandTokens[4], ',');
				}
			}
		}
		client_session->answeredMessages[idMessageCurrent] = false;
		client_session->send(mb, idMessageCurrent);

		return idMessageCurrent;
	}

	if (commandTokens.size() == 1) {
		if (commandTokens[0] == "GetXyzOfIjkGrids") {
			setSessionToEtpHdfProxy(client_session);
		}
		else if (commandTokens[0] == "List") {
			std::cout << "*** START LISTING ***" << std::endl;
			COMMON_NS::EpcDocument& epcDoc = client_session->epcDoc;
			for (const auto& entryPair : epcDoc.getResqmlAbstractObjectSet()) {
				if (!entryPair.second->isPartial()) {
					std::cout << entryPair.first << " : " << entryPair.second->getTitle() << std::endl;
					std::cout << "*** SOURCE REL ***" << std::endl;
					for (const auto& uuid : entryPair.second->getAllSourceRelationshipUuids()) {
						std::cout << uuid << " : " << epcDoc.getResqmlAbstractObjectByUuid(uuid)->getXmlTag() << std::endl;
					}
					std::cout << "*** TARGET REL ***" << std::endl;
					for (const auto& uuid : entryPair.second->getAllTargetRelationshipUuids()) {
						std::cout << uuid << " : " << epcDoc.getResqmlAbstractObjectByUuid(uuid)->getXmlTag() << std::endl;
					}
					std::cout << std::endl;
				}
				else {
					std::cout << "PARTIAL " << entryPair.first << " : " << entryPair.second->getTitle() << std::endl;
				}
			}
			std::cout << "*** END LISTING ***" << std::endl;
		}
	}
	else if (commandTokens.size() == 2) {
		if (commandTokens[0] == "GetContent") {
			Energistics::Etp::v12::Protocol::DirectedDiscovery::GetContent mb;
			mb.m_uri = commandTokens[1];
			client_session->answeredMessages[idMessageCurrent] = false;
			client_session->send(mb, idMessageCurrent);
			return idMessageCurrent;
		}
		else if (commandTokens[0] == "GetSources") {
			Energistics::Etp::v12::Protocol::DirectedDiscovery::GetSources mb;
			mb.m_uri = commandTokens[1];
			client_session->answeredMessages[idMessageCurrent] = false;
			client_session->send(mb, idMessageCurrent);
			return idMessageCurrent;
		}
		else if (commandTokens[0] == "GetTargets") {
			Energistics::Etp::v12::Protocol::DirectedDiscovery::GetTargets mb;
			mb.m_uri = commandTokens[1];
			client_session->answeredMessages[idMessageCurrent] = false;
			client_session->send(mb, idMessageCurrent);
			return idMessageCurrent;
		}
		else if (commandTokens[0] == "GetObject") {
			Energistics::Etp::v12::Protocol::Store::GetObject_ getO;
			getO.m_uri = commandTokens[1];
			client_session->answeredMessages[idMessageCurrent] = false;
			client_session->send(getO, idMessageCurrent);
			return idMessageCurrent;
		}
		else if (commandTokens[0] == "GetSourceObjects") {
			Energistics::Etp::v12::Protocol::DirectedDiscovery::GetSources mb;
			mb.m_uri = commandTokens[1];
			client_session->answeredMessages[idMessageCurrent] = false;
			std::static_pointer_cast<EtpFesppDirectedDiscoveryProtocolHandlers>(client_session->getDirectedDiscoveryProtocolHandlers())->getObjectWhenDiscovered.push_back(client_session->send(mb, idMessageCurrent));
			return idMessageCurrent;
		}
		else if (commandTokens[0] == "GetTargetObjects") {
			Energistics::Etp::v12::Protocol::DirectedDiscovery::GetTargets mb;
			mb.m_uri = commandTokens[1];
			client_session->answeredMessages[idMessageCurrent] = false;
			std::static_pointer_cast<EtpFesppDirectedDiscoveryProtocolHandlers>(client_session->getDirectedDiscoveryProtocolHandlers())->getObjectWhenDiscovered.push_back(client_session->send(mb, idMessageCurrent));
			return idMessageCurrent;
		}
		else if (commandTokens[0] == "GetResourceObjects") {
			Energistics::Etp::v12::Protocol::Discovery::GetResources2 mb;
			mb.m_context.m_uri = commandTokens[1];
			mb.m_context.m_scope = Energistics::Etp::v12::Datatypes::Object::ContextScopeKind::targetsOrSelf;
			mb.m_context.m_depth = 1;
			client_session->answeredMessages[idMessageCurrent] = false;
			std::static_pointer_cast<EtpFesppDirectedDiscoveryProtocolHandlers>(client_session->getDiscoveryProtocolHandlers())->getObjectWhenDiscovered.push_back(client_session->send(mb, idMessageCurrent));

			mb.m_context.m_scope = Energistics::Etp::v12::Datatypes::Object::ContextScopeKind::sources;
			client_session->answeredMessages[++idMessageCurrent] = false;
			std::static_pointer_cast<EtpFesppDirectedDiscoveryProtocolHandlers>(client_session->getDiscoveryProtocolHandlers())->getObjectWhenDiscovered.push_back(client_session->send(mb, idMessageCurrent));
			return idMessageCurrent;
		}
	}
	else if (commandTokens.size() == 3) {
		if (commandTokens[0] == "GetDataArray") {
			Energistics::Etp::v12::Protocol::DataArray::GetDataArray gda;
			gda.m_uri = commandTokens[1];
			gda.m_pathInResource = commandTokens[2];
			std::cout << gda.m_pathInResource << std::endl;
			client_session->answeredMessages[idMessageCurrent] = false;
			client_session->send(gda, idMessageCurrent);
			return idMessageCurrent;
		}
	}

	return idMessageCurrent;
}


//----------------------------------------------------------------------------
void VtkEtpDocument::createTree()
{
	auto last_EpcCommon = new VtkEpcCommon();
	last_EpcCommon->setUuid("eml://");
	last_EpcCommon->setType(VtkEpcCommon::Resqml2Type::INTERPRETATION);
	last_EpcCommon->setName("eml://");
	response_queue.push_back(last_EpcCommon);
	number_response_wait_queue.push_back(2);
	last_id = push_command("GetContent eml://");
}

//----------------------------------------------------------------------------
void VtkEtpDocument::visualize(const std::string & rec_uri)
{
	auto pos1 = rec_uri.find_last_of("/")+1;
	auto lenght = rec_uri.find_last_of("(") - pos1;
	std::string type = rec_uri.substr(pos1,lenght);

	std::string uuidToAttach = "";
	if(type=="obj_IjkGridRepresentation")
	{
		auto idMessage0 = push_command("GetObject "+rec_uri);

		cout << "all push" << endl;
		while(client_session->answeredMessages[idMessage0] == false) {
//			cout << "In wait loop" << endl;
//			client_session->printansweredMessages();
		} // wait answers

		idMessage0 = push_command("GetSourceObjects "+rec_uri);

		cout << "all push" << endl;
		while(client_session->answeredMessages[idMessage0] == false){
//			cout << "In wait loop" << endl;
//			client_session->printansweredMessages();
		} // wait answers

		idMessage0 = push_command("GetTargetObjects "+rec_uri);


		cout << "all push" << endl;
//		client_session->printansweredMessages();
		while(client_session->answeredMessages[idMessage0] == false){
//			cout << "In wait loop" << endl;
//			client_session->printansweredMessages();
		} // wait answers

		push_command("GetXyzOfIjkGrids");

//		while(client_session->answeredMessages[idMessage0] == false &&
//				client_session->answeredMessages[idMessage1] == false &&
//					client_session->answeredMessages[idMessage2] == false){
//		}  // wait answers

		//
		//
		// Visu
		//
		//

		COMMON_NS::EpcDocument& epcDoc = client_session->epcDoc;
		auto ijkGridSet = epcDoc.getIjkGridRepresentationSet();

		for (const auto & ijkGrid : ijkGridSet) {
			if (ijkGrid->isPartial()) {
				std::cout << "Partial Ijk Grid " << ijkGrid->getTitle() << " : " << ijkGrid->getUuid() << std::endl;
				continue;
			}
			else {
				uuidToAttach = ijkGrid->getUuid();
				uuidToVtkIjkGridRepresentation[uuidToAttach] = new VtkIjkGridRepresentation(std::string("EtpDocument"), ijkGrid->getTitle(), ijkGrid->getUuid(), ijkGrid->getParentGridUuid(), nullptr, nullptr);

				if (ijkGrid->getGeometryKind() == RESQML2_0_1_NS::AbstractIjkGridRepresentation::NO_GEOMETRY) {
					std::cout << "This IJK Grid has got no geometry." << std::endl;
					continue;
				}
			}

			//*****************
			//*** GEOMETRY ****
			//*****************
			auto xyzPointCount = ijkGrid->getXyzPointCountOfPatch(0);
			auto xyzPoints = new double[xyzPointCount * 3];
			ijkGrid->getXyzPointsOfPatchInGlobalCrs(0, xyzPoints);

			uuidToVtkIjkGridRepresentation[uuidToAttach]->createVtkPoints(xyzPointCount, xyzPoints, ijkGrid->getLocalCrs() );
			uuidToVtkIjkGridRepresentation[uuidToAttach]->setICellCount(ijkGrid->getICellCount());
			uuidToVtkIjkGridRepresentation[uuidToAttach]->setJCellCount(ijkGrid->getJCellCount());
			uuidToVtkIjkGridRepresentation[uuidToAttach]->setKCellCount(ijkGrid->getKCellCount());
			uuidToVtkIjkGridRepresentation[uuidToAttach]->setInitKIndex(0);
			uuidToVtkIjkGridRepresentation[uuidToAttach]->setMaxKIndex(ijkGrid->getKCellCount());
			uuidToVtkIjkGridRepresentation[uuidToAttach]->createWithPoints(uuidToVtkIjkGridRepresentation[uuidToAttach]->getVtkPoints(), ijkGrid);

			//			//*****************
			//			//*** PROPERTY ****
			//			//*****************
			//			auto propSet = ijkGrid->getPropertySet();
			//			for (const auto & prop : propSet) {
			//				RESQML2_0_1_NS::ContinuousProperty* continuousProp = dynamic_cast<RESQML2_0_1_NS::ContinuousProperty*>(prop);
			//				if (continuousProp != nullptr && dynamic_cast<RESQML2_0_1_NS::ContinuousPropertySeries*>(continuousProp) == nullptr &&
			//						continuousProp->getAttachmentKind() == gsoap_resqml2_0_1::resqml2__IndexableElements::resqml2__IndexableElements__cells) {
			//					std::cout << "Continuous property " << prop->getTitle() << " : " << prop->getUuid() << std::endl;
			//					auto cellCount = ijkGrid->getCellCount();
			//					std::unique_ptr<double[]> values(new double[cellCount * continuousProp->getElementCountPerValue()]);
			//					continuousProp->getDoubleValuesOfPatch(0, values.get());
			//					for (auto cellIndex = 0; cellIndex < cellCount; ++cellIndex) {
			//						for (unsigned int elementIndex = 0; elementIndex < continuousProp->getElementCountPerValue(); ++elementIndex) {
			//							std::cout << "Cell Index " << cellIndex << " : " << values[cellIndex] << std::endl;
			//						}
			//					}
			//				}
			//				else {
			//					std::cout << "Non continuous property " << prop->getTitle() << " : " << prop->getUuid() << std::endl;
			//				}
			//			}
			//			cout << "<----- AFTER PROPERTY -----> "<< endl;
			//			std::cout << std::endl;
		}
	}

	// attach representation to EtpDocument VtkMultiBlockDataSet
	if (std::find(attachUuids.begin(), attachUuids.end(), uuidToAttach) == attachUuids.end())
	{
		this->detach();
		attachUuids.push_back(uuidToAttach);
		this->attach();
	}
}
void VtkEtpDocument::createTreeVtk(const std::string & uuid, const std::string & parent, const std::string & name, const VtkEpcCommon::Resqml2Type & type)
{

}

//----------------------------------------------------------------------------
void VtkEtpDocument::unvisualize(const std::string & uuid)
{
}

//----------------------------------------------------------------------------
void VtkEtpDocument::remove(const std::string & uuid)
{

}
long VtkEtpDocument::getAttachmentPropertyCount(const std::string & uuid, const VtkEpcCommon::FesppAttachmentProperty propertyUnit)
{

}


//----------------------------------------------------------------------------
void VtkEtpDocument::attach()
{
	for (unsigned int newBlockIndex = 0; newBlockIndex < attachUuids.size(); ++newBlockIndex)
	{
		std::string uuid = attachUuids[newBlockIndex];

		vtkOutput->SetBlock(newBlockIndex, uuidToVtkIjkGridRepresentation[uuid]->getOutput());
		vtkOutput->GetMetaData(newBlockIndex)->Set(vtkCompositeDataSet::NAME(), uuidToVtkIjkGridRepresentation[uuid]->getUuid().c_str());
	}
}

//--------------- FOR TREEVIEW-------------------------------------------------
void VtkEtpDocument::receive_resources_tree(const std::string & rec_uri,
		const std::string & rec_contentType,
		const std::string & rec_name,
		Energistics::Etp::v12::Datatypes::Object::ResourceKind & rec_resourceType,
		const int32_t & rec_sourceCount,
		const int32_t & rec_targetCount,
		const int32_t & rec_contentCount,
		const int64_t & rec_lastChanged)
{
	auto leaf = new VtkEpcCommon();

	leaf->setUuid(rec_uri);
	//		leaf->setName(rec_name);
	leaf->setName(rec_uri);
	leaf->setType(VtkEpcCommon::Resqml2Type::INTERPRETATION);
	leaf->setParentType(response_queue.front()->getType());
	leaf->setParent(response_queue.front()->getUuid());

	if (rec_contentCount>0) {
		response_queue.push_back(leaf);
		number_response_wait_queue.push_back(rec_contentCount);
		command_queue.push_back("GetContent "+rec_uri);
	}

	if (rec_resourceType==Energistics::Etp::v12::Datatypes::Object::ResourceKind::DataObject) {
		std::size_t pos = rec_contentType.find(";type=");
		std::string type = rec_contentType.substr (pos+6);

		if(type=="obj_TriangulatedSetRepresentation") {
		}
		if(type=="obj_Grid2dRepresentation") {
		}
		if(type=="obj_WellboreTrajectoryRepresentation") {
		}
		if(type=="obj_PolylineSetRepresentation") {
		}
		if(type=="obj_IjkGridRepresentation") {
			leaf->setType(VtkEpcCommon::Resqml2Type::IJK_GRID);

			treeView.push_back(leaf);

			if (rec_sourceCount>0) {
				response_queue.push_back(leaf);
				number_response_wait_queue.push_back(rec_sourceCount);
				command_queue.push_back("GetSources "+rec_uri);
			}

		}
		if(type=="obj_SubRepresentation") {
			leaf->setType(VtkEpcCommon::Resqml2Type::SUB_REP);

			treeView.push_back(leaf);

			if (rec_sourceCount>0) {
				response_queue.push_back(leaf);
				number_response_wait_queue.push_back(rec_sourceCount);
				command_queue.push_back("GetSources "+rec_uri);
			}

		}
		if(type=="obj_ContinuousProperty" || type=="obj_DiscreteProperty") {
			leaf->setType(VtkEpcCommon::Resqml2Type::PROPERTY);

			treeView.push_back(leaf);
		}
	}

	--number_response_wait_queue.front();
	if(number_response_wait_queue.front() == 0) {
		number_response_wait_queue.pop_front();
		if (command_queue.size() > 0) {
			response_queue.pop_front();
			last_id = push_command(command_queue.front());
			command_queue.pop_front();
		}
		else {
			client_session->close();
			client_session->epcDoc.close();
			getPQEtpPanel()->setEtpTreeView(this->getTreeView());

		}

	}
}

//----------------------------------------------------------------------------
std::vector<VtkEpcCommon*> VtkEtpDocument::getTreeView() const
{
	return treeView;
}

//--------------- FOR REPRESENTATION------------------------------------------
void VtkEtpDocument::receive_resources_representation(const std::string & rec_uri,
		const std::string & rec_contentType,
		const std::string & rec_name,
		Energistics::Etp::v12::Datatypes::Object::ResourceKind & rec_resourceType,
		const int32_t & rec_sourceCount,
		const int32_t & rec_targetCount,
		const int32_t & rec_contentCount,
		const int64_t & rec_lastChanged)
{
//		cout << "VtkEtpDocument::receive_resources_representation" << endl;
}

//----------------------------------------------------------------------------
vtkSmartPointer<vtkMultiBlockDataSet> VtkEtpDocument::getVisualization() const
{
	cout << "return ?" << endl;
	return vtkOutput;
}
