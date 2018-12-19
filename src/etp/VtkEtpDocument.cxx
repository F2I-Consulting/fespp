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
#include "etp/EtpFesppCoreProtocolHandlers.h"

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
VtkEtpDocument::VtkEtpDocument(const std::string & ipAddress, const std::string & port, const VtkEpcCommon::modeVtkEpc & mode)
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

	response_waiting = 0;

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
void VtkEtpDocument::add_command(const std::string & command)
{
	wait_queue_command.push_front(command);
}


//----------------------------------------------------------------------------
void VtkEtpDocument::push_command(const std::string & command)
{

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
		response_queue_command.push_back(command);
		client_session->send(mb);
	}

	if (commandTokens.size() == 1) {
		if (commandTokens[0] == "Wait") {
			response_queue_command.push_back(command);
		}
		if (commandTokens[0] == "GetXyzOfIjkGrids") {
			setSessionToEtpHdfProxy(client_session);
			COMMON_NS::EpcDocument& epcDoc = client_session->epcDoc;
			auto ijkGridSet = epcDoc.getIjkGridRepresentationSet();
			for (const auto & ijkGrid : ijkGridSet) {
				if (ijkGrid->isPartial()) {
					std::cout << "Partial Ijk Grid " << ijkGrid->getTitle() << " : " << ijkGrid->getUuid() << std::endl;
					continue;
				}
				else {
					std::cout << "Ijk Grid " << ijkGrid->getTitle() << " : " << ijkGrid->getUuid() << std::endl;
					if (ijkGrid->getGeometryKind() == RESQML2_0_1_NS::AbstractIjkGridRepresentation::NO_GEOMETRY) {
						std::cout << "This IJK Grid has got no geometry." << std::endl;
						continue;
					}
				}

				//*****************
				//*** GEOMETRY ****
				//*****************
				auto xyzPointCount = ijkGrid->getXyzPointCountOfPatch(0);
				cout << "-----> xyzPointCount "<< xyzPointCount << endl;
				std::unique_ptr<double[]> xyzPoints(new double[xyzPointCount * 3]);
				ijkGrid->getXyzPointsOfPatchInGlobalCrs(0, xyzPoints.get());

				cout << "-----> getXyzPointsOfPatchInGlobalCrs "<< endl;
				for (auto xyzPointIndex = 0; xyzPointIndex < xyzPointCount && xyzPointIndex < 20; ++xyzPointIndex) {
					std::cout << "XYZ Point Index " << xyzPointIndex << " : " << xyzPoints[xyzPointIndex * 3] << "," << xyzPoints[xyzPointIndex * 3 + 1] << "," << xyzPoints[xyzPointIndex * 3 + 2] << std::endl;
				}

				cout << "<----- AFTER GEOMETRY -----> "<< endl;
				//*****************
				//*** PROPERTY ****
				//*****************
				auto propSet = ijkGrid->getPropertySet();
				for (const auto & prop : propSet) {
					RESQML2_0_1_NS::ContinuousProperty* continuousProp = dynamic_cast<RESQML2_0_1_NS::ContinuousProperty*>(prop);
					if (continuousProp != nullptr && dynamic_cast<RESQML2_0_1_NS::ContinuousPropertySeries*>(continuousProp) == nullptr &&
							continuousProp->getAttachmentKind() == gsoap_resqml2_0_1::resqml2__IndexableElements::resqml2__IndexableElements__cells) {
						std::cout << "Continuous property " << prop->getTitle() << " : " << prop->getUuid() << std::endl;
						auto cellCount = ijkGrid->getCellCount();
						std::unique_ptr<double[]> values(new double[cellCount * continuousProp->getElementCountPerValue()]);
						continuousProp->getDoubleValuesOfPatch(0, values.get());
						for (auto cellIndex = 0; cellIndex < cellCount; ++cellIndex) {
							for (unsigned int elementIndex = 0; elementIndex < continuousProp->getElementCountPerValue(); ++elementIndex) {
								std::cout << "Cell Index " << cellIndex << " : " << values[cellIndex] << std::endl;
							}
						}
					}
					else {
						std::cout << "Non continuous property " << prop->getTitle() << " : " << prop->getUuid() << std::endl;
					}
				}
				cout << "<----- AFTER PROPERTY -----> "<< endl;
				std::cout << std::endl;
			}
			//			this->launch_command();
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
			//			this->launch_command();
		}
	}
	else if (commandTokens.size() == 2) {
		if (commandTokens[0] == "GetContent") {
			Energistics::Etp::v12::Protocol::DirectedDiscovery::GetContent mb;
			mb.m_uri = commandTokens[1];
			response_queue_command.push_back(command);
			client_session->send(mb);
		}
		else if (commandTokens[0] == "GetSources") {
			Energistics::Etp::v12::Protocol::DirectedDiscovery::GetSources mb;
			mb.m_uri = commandTokens[1];
			response_queue_command.push_back(command);
			client_session->send(mb);
		}
		else if (commandTokens[0] == "GetTargets") {
			Energistics::Etp::v12::Protocol::DirectedDiscovery::GetTargets mb;
			mb.m_uri = commandTokens[1];
			response_queue_command.push_back(command);
			client_session->send(mb);
		}
		else if (commandTokens[0] == "GetObject") {
			Energistics::Etp::v12::Protocol::Store::GetObject_ getO;
			getO.m_uri = commandTokens[1];
			response_queue_command.push_back(command);
			client_session->send(getO);
		}
		else if (commandTokens[0] == "GetSourceObjects") {
			Energistics::Etp::v12::Protocol::DirectedDiscovery::GetSources mb;
			mb.m_uri = commandTokens[1];
			response_queue_command.push_back("GetSources");
			std::static_pointer_cast<EtpFesppDirectedDiscoveryProtocolHandlers>(client_session->getDirectedDiscoveryProtocolHandlers())->getObjectWhenDiscovered.push_back(client_session->send(mb));
		}
		else if (commandTokens[0] == "GetTargetObjects") {
			Energistics::Etp::v12::Protocol::DirectedDiscovery::GetTargets mb;
			mb.m_uri = commandTokens[1];
			response_queue_command.push_back("GetTargets");
			std::static_pointer_cast<EtpFesppDirectedDiscoveryProtocolHandlers>(client_session->getDirectedDiscoveryProtocolHandlers())->getObjectWhenDiscovered.push_back(client_session->send(mb));
		}
		else if (commandTokens[0] == "GetResourceObjects") {
			Energistics::Etp::v12::Protocol::Discovery::GetResources2 mb;
			mb.m_context.m_uri = commandTokens[1];
			mb.m_context.m_scope = Energistics::Etp::v12::Datatypes::Object::ContextScopeKind::targetsOrSelf;
			mb.m_context.m_depth = 1;
			response_queue_command.push_back("GetResources2");
			std::static_pointer_cast<EtpFesppDirectedDiscoveryProtocolHandlers>(client_session->getDiscoveryProtocolHandlers())->getObjectWhenDiscovered.push_back(client_session->send(mb));

			mb.m_context.m_scope = Energistics::Etp::v12::Datatypes::Object::ContextScopeKind::sources;
			response_queue_command.push_back("GetResources2");
			std::static_pointer_cast<EtpFesppDirectedDiscoveryProtocolHandlers>(client_session->getDiscoveryProtocolHandlers())->getObjectWhenDiscovered.push_back(client_session->send(mb));
		}
	}
	else if (commandTokens.size() == 3) {
		if (commandTokens[0] == "GetDataArray") {
			Energistics::Etp::v12::Protocol::DataArray::GetDataArray gda;
			gda.m_uri = commandTokens[1];
			gda.m_pathInResource = commandTokens[2];
			std::cout << gda.m_pathInResource << std::endl;
			response_queue_command.push_back(command);
			client_session->send(gda);
		}
	}
	else if (commandTokens.empty() || commandTokens[0] != "quit") {
		std::cout << "List of available commands :" << std::endl;
		std::cout << "\tList" << std::endl << "\t\tList the objects which have been got from ETP to the in memory epc" << std::endl << std::endl;
		std::cout << "\tGetContent folderUri" << std::endl << "\t\tGet content of a folder in an ETP store" << std::endl << std::endl;
		std::cout << "\tGetSources dataObjectURI" << std::endl << "\t\tGet dataobject sources of a dataobject in an ETP store" << std::endl << std::endl;
		std::cout << "\tGetTargets dataObjectURI" << std::endl << "\t\tGet dataobject targets of a dataobject in an ETP store" << std::endl << std::endl;
		std::cout << "\tGetResources dataObjectURI scope(default self) depth(default 1) contentTypeFilter,contentTypeFilter,...(default noFilter)" << std::endl << "\t\tSame as GetContent, GetSources and GetTargets but in the Discovery protocol instead of the directed discovery protocol." << std::endl << std::endl;
		std::cout << "\tGetObject dataObjectURI" << std::endl << "\t\tGet the object from an ETP store and store it into the in memory epc (only create partial TARGET relationships, not any SOURCE relationships)" << std::endl << std::endl;
		std::cout << "\tGetSourceObjects dataObjectURI" << std::endl << "\t\tGet the source objects of another object from an ETP store and put it into the in memory epc" << std::endl << std::endl;
		std::cout << "\tGetTargetObjects dataObjectURI" << std::endl << "\t\tGet the target objects of another object from an ETP store and put it into the in memory epc" << std::endl << std::endl;
		std::cout << "\tGetResourceObjects dataObjectURI" << std::endl << "\t\tGet the object, its source and its target objects from an ETP store and put it into the in memory epc" << std::endl << std::endl;
		std::cout << "\tGetDataArray epcExternalPartURI datasetPathInEpcExternalPart" << std::endl << "\t\tGet the numerical values from a dataset included in an EpcExternalPart over ETP." << std::endl << std::endl;
		std::cout << "\tGetXyzOfIjkGrids" << std::endl << "\t\tGet all the XYZ points of the retrieved IJK grids and also their continuous property values." << std::endl << std::endl;
		std::cout << "\tquit" << std::endl << "\t\tQuit the session." << std::endl << std::endl;
	}
}


//----------------------------------------------------------------------------
void VtkEtpDocument::createTree()
{
	cout << "VtkEtpDocument::createTree()" << endl;
	wait_queue_command.push_back("GetContent eml://");
	this->launch_command();

}

//----------------------------------------------------------------------------
void VtkEtpDocument::visualize(const std::string & rec_uri)
{
	auto pos1 = rec_uri.find_last_of("/")+1;
	auto lenght = rec_uri.find_last_of("(") - pos1;
	std::string type = rec_uri.substr(pos1,lenght);

	if(type=="obj_IjkGridRepresentation")
	{
		wait_queue_command.push_back("GetObject "+rec_uri);
		wait_queue_command.push_back("GetSourceObjects "+rec_uri);
		wait_queue_command.push_back("GetTargetObjects "+rec_uri);
		wait_queue_command.push_back("GetXyzOfIjkGrids");
	}
	this->launch_command();
}

//----------------------------------------------------------------------------
void VtkEtpDocument::unvisualize(const std::string & uuid)
{
	cout << "VtkEtpDocument::unvisualize = " << uuid << endl;
}

//----------------------------------------------------------------------------
void VtkEtpDocument::remove(const std::string & uuid)
{

}

void VtkEtpDocument::launch_command()
{
	if (wait_queue_command.size() > 0)
	{
		cout << "------------ AFFICHAGE QUEUE command ------------------" << endl<< endl;
		for (std::list<std::string>::iterator it= wait_queue_command.begin(); it != wait_queue_command.end();)
		{
			cout << *it << endl;
			++it;
		}
		cout << "------------ AFFICHAGE QUEUE command ------------------" << endl<<endl;

		bool firstWait = false;

		if (wait_queue_command.front() == "Wait")
			firstWait = true;

		this->push_command(wait_queue_command.front());

		wait_queue_command.pop_front();

		if (wait_queue_command.size() > 0)
			if (wait_queue_command.front() != "Wait" && firstWait){
				this->push_command(wait_queue_command.front());
				wait_queue_command.pop_front();
			}
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
	if(treeViewMode)
	{
		cout << "VtkEtpDocument::receive_resources_tree"<<endl;
		bool executed_next_command = true;

		if (response_waiting > 0){
			response_waiting--;
		}

		auto leaf = new VtkEpcCommon();

		leaf->setUuid(rec_uri);
		leaf->setParentType(VtkEpcCommon::Resqml2Type::INTERPRETATION);

		if (response_queue.size() > 0) {
			leaf->setParent(response_queue.front()->getUuid());
			leaf->setParentType(response_queue.front()->getType());

			response_queue.pop_front();
		}
		//		leaf->setName(rec_name);
		leaf->setName(rec_uri);

		if (rec_resourceType==Energistics::Etp::v12::Datatypes::Object::ResourceKind::UriProtocol) {
			executed_next_command = false;
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
					for(auto i=0; i < rec_sourceCount; ++i){
						response_queue.push_back(leaf);
					}
					response_waiting += rec_sourceCount;

					wait_queue_command.push_back("GetSources "+rec_uri);
				}

			}
			if(type=="obj_SubRepresentation") {
				leaf->setType(VtkEpcCommon::Resqml2Type::SUB_REP);

				treeView.push_back(leaf);

				if (rec_sourceCount>0) {
					for(auto i=0; i < rec_sourceCount; ++i){
						response_queue.push_back(leaf);
					}
					response_waiting += rec_sourceCount;

					wait_queue_command.push_back("GetSources "+rec_uri);
				}

			}
			if(type=="obj_ContinuousProperty" || type=="obj_DiscreteProperty") {
				leaf->setType(VtkEpcCommon::Resqml2Type::PROPERTY);

				treeView.push_back(leaf);
			}
		}

		if (rec_contentCount>0) {
			for(auto i=0; i < rec_contentCount; ++i){
				response_queue.push_back(leaf);
			}
			response_waiting += rec_contentCount;

			wait_queue_command.push_back("GetContent "+rec_uri);
		}

		cout << "response_waiting = " << response_waiting << "\n";

		if (response_waiting==0)
		{

			client_session->close();
			client_session->epcDoc.close();
			//while(!client_session->isClosed()) {cout << "wait close"<<endl;}
			getPQEtpPanel()->setEtpTreeView(this->getTreeView());

		}

		this->launch_command();
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
	cout << "VtkEtpDocument::receive_resources_representation" << endl;

	//	if (all_sended && response_queue_command.size()==0)
	//		all_received = true;


	this->launch_command();
}

//----------------------------------------------------------------------------
//void VtkEtpDocument::receive_resources_new_ask(const std::string & ask)
//{
//	cout << "VtkEtpDocument::receive_resources_new_ask" << endl;
//	cout << ask << endl;
//	response_queue_command.push_back(ask);
//}

//----------------------------------------------------------------------------
void VtkEtpDocument::receive_resources_finished()
{
	//	cout << "VtkEtpDocument::receive_resources_finished" << endl;
	//	response_queue_command.pop_front();
	//	all_sended = true;
	this->launch_command();
}

//----------------------------------------------------------------------------
vtkSmartPointer<vtkMultiBlockDataSet> VtkEtpDocument::getVisualization() const
{
	return vtkOutput;
}
