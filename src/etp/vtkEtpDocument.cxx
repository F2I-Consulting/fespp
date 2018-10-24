#include <etp/vtkEtpDocument.h>
#include <algorithm>
#include <sstream>
#include <thread>

// include Vtk
#include <vtkInformation.h>

// include Fespp
#include <etp/etpClientSession.h>

// include FESAPI


// include Vtk for Plugin


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
	cout <<"vtkEtpDocument::vtkEtpDocument - constructor\n";

	response_waiting = 0;

	cout <<"response_waiting = 0\n";
	// Run the I/O service. The call will return when the socket is closed.
	cout << ipAddress << " : " << port << "\n";



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
	response_waiting = 2;
	client_session->pushCommand("GetContent eml://");
	while (waiting!=0)
	{

	}
	cout << "\n\n\nlibérer\n\n\n\n";
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
	response_waiting--;
	if (rec_contentCount!=-1)
	{
		cout << "receive msg \n";
		response_waiting += rec_contentCount;

		std::stringstream ss;
		ss << "GetContent " <<  rec_uri;

		client_session->pushCommand(ss.str());
	}
	if (rec_sourceCount!=-1)
	{
		cout << "receive msg \n";
		response_waiting += rec_sourceCount;

		std::stringstream ss;
		ss << "GetSources " <<  rec_uri;

		client_session->pushCommand(ss.str());
	}


	cout << "receive msg :" << response_waiting << "\n";

	if (response_waiting == 0)
	{
		waiting = false;
	}

}
