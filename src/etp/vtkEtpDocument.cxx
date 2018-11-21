#include <etp/vtkEtpDocument.h>
#include <algorithm>
#include <thread>

// include Vtk
#include <vtkInformation.h>
#include <QApplication>
//#include <QWidget>

// include Fespp
#include <etp/etpClientSession.h>
#include <PQEtpPanel.h>

// include FESAPI


// include Vtk for Plugin

namespace
{
PQEtpPanel* getPQEtpPanel()
{
	cout << "getpanel()" << endl;
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

void startio(vtkEtpDocument *etp_document, std::string ipAddress, std::string port)
{
	boost::asio::io_context ioc;

	Energistics::Etp::v12::Datatypes::Version protocolVersion;
	protocolVersion.m_major = 1;
	protocolVersion.m_minor = 2;
	protocolVersion.m_patch = 0;
	protocolVersion.m_revision = 0;
	// Requested protocol
	std::vector<Energistics::Etp::v12::Datatypes::SupportedProtocol> requestedProtocols;
	Energistics::Etp::v12::Datatypes::SupportedProtocol protocol;

	protocol.m_protocol = Energistics::Etp::v12::Datatypes::Protocols::Core;
	protocol.m_protocolVersion = protocolVersion;
	protocol.m_role = "server";
	requestedProtocols.push_back(protocol);

	protocol.m_protocol = Energistics::Etp::v12::Datatypes::Protocols::Store;
	protocol.m_protocolVersion = protocolVersion;
	protocol.m_role = "store";
	requestedProtocols.push_back(protocol);

	protocol.m_protocol = Energistics::Etp::v12::Datatypes::Protocols::DataArray;
	protocol.m_protocolVersion = protocolVersion;
	protocol.m_role = "store";
	requestedProtocols.push_back(protocol);

	protocol.m_protocol = Energistics::Etp::v12::Datatypes::Protocols::DirectedDiscovery;
	protocol.m_protocolVersion = protocolVersion;
	protocol.m_role = "store";
	requestedProtocols.push_back(protocol);

	std::vector<std::string> supportedObjects;

	auto client_session_sharedPtr = std::make_shared<etpClientSession>(ioc, ipAddress, port,  requestedProtocols, supportedObjects, etp_document);
	client_session_sharedPtr->run();
	etp_document->setClientSession(client_session_sharedPtr.get());
	ioc.run();
}

//----------------------------------------------------------------------------
vtkEtpDocument::vtkEtpDocument(const std::string & ipAddress, const std::string & port)
{
	response_waiting = 0;

	std::thread askUserThread(startio, this, ipAddress, port);
	askUserThread.detach(); // Detach the thread since we don't want it to be a blocking one.

#ifdef _WIN32
	_CrtDumpMemoryLeaks();
#endif

}

//----------------------------------------------------------------------------
vtkEtpDocument::~vtkEtpDocument()
{
	client_session->pushCommand("quit");
}

//----------------------------------------------------------------------------
void vtkEtpDocument::createTree()
{
	client_session->pushCommand("GetContent eml://");
}

//----------------------------------------------------------------------------
void vtkEtpDocument::visualize(const std::string & uuid)
{

}

//----------------------------------------------------------------------------
void vtkEtpDocument::remove(const std::string & uuid)
{

}

//----------------------------------------------------------------------------
void vtkEtpDocument::receive_resources(const std::string & rec_uri,
		const std::string & rec_contentType,
		const std::string & rec_name,
		const std::string & rec_resourceType,
		const int32_t & rec_sourceCount,
		const int32_t & rec_targetCount,
		const int32_t & rec_contentCount,
		const std::string & rec_uuid,
		const int64_t & rec_lastChanged)
{
	cout << "vtkEtpDocument::receive_resources(" << rec_uuid << ")" << endl;
	if (response_waiting > 0){
		response_waiting--;
	}

	auto leaf = new VtkEpcCommon();
	leaf->setUuid(rec_uuid);
	leaf->setParentType(VtkEpcCommon::Resqml2Type::INTERPRETATION);

	if (response_queue.size() > 0) {
		leaf->setParent(response_queue.front()->getUuid());
		leaf->setParentType(response_queue.front()->getType());

		response_queue.pop_front();
	}
	leaf->setName(rec_name);

	if (rec_resourceType=="DataObject") {
		this->addTreeView(leaf, rec_contentType);
	}

	if (rec_contentCount>0) {
		for(auto i=0; i < rec_contentCount; ++i){
			response_queue.push_back(leaf);
		}
		response_waiting += rec_contentCount;

		client_session->pushCommand("GetContent "+rec_uri);
	}

	if (rec_sourceCount>0) {
		for(auto i=0; i < rec_sourceCount; ++i){
			response_queue.push_back(leaf);
		}
		response_waiting += rec_sourceCount;

		client_session->pushCommand("GetSources "+rec_uri);
	}

	cout << "response_waiting = " << response_waiting << "\n";
	if (response_waiting==0)
	{
		getPQEtpPanel()->setEtpTreeView(this->getTreeView());
	}
}

//----------------------------------------------------------------------------
void vtkEtpDocument::addTreeView(VtkEpcCommon* leaf,const std::string & content_type)
{
	  std::size_t pos = content_type.find(";type=");
	  std::string type = content_type.substr (pos+6);

	  if(type=="obj_IjkGridRepresentation") {
		  leaf->setType(VtkEpcCommon::Resqml2Type::IJK_GRID);

		  treeView.push_back(leaf);
	  }
	  if(type=="obj_SubRepresentation") {
		  leaf->setType(VtkEpcCommon::Resqml2Type::SUB_REP);

		  treeView.push_back(leaf);
	  }
	  if(type=="obj_ContinuousProperty" || type=="obj_DiscreteProperty") {
		  leaf->setType(VtkEpcCommon::Resqml2Type::PROPERTY);

		  treeView.push_back(leaf);
	  }
}

//----------------------------------------------------------------------------
std::vector<VtkEpcCommon*> vtkEtpDocument::getTreeView() const
{
	return treeView;
}
