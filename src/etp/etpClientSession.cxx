
#include <etp/etpClientSession.h>
#include "etp/ProtocolHandlers/CoreHandlers.h"
#include "etpFesppDirectedDiscoveryProtocolHandlers.h"
#include "etp/ProtocolHandlers/DataArrayHandlers.h"
#include "etpFesppStoreProtocolHandlers.h"



etpClientSession::etpClientSession(boost::asio::io_context& ioc,
		const std::string & host, const std::string & port,
		const std::vector<Energistics::Etp::v12::Datatypes::SupportedProtocol> & requestedProtocols,
		const std::vector<std::string>& supportedObjects,
		vtkEtpDocument* my_etp_document)
: ETP_NS::ClientSession(ioc, host, port, "/", requestedProtocols, supportedObjects),
  epcDoc("/tmp/etp.epc", COMMON_NS::EpcDocument::OVERWRITE)
{
	setCoreProtocolHandlers(std::make_shared<ETP_NS::CoreHandlers>(this));
	setDirectedDiscoveryProtocolHandlers(std::make_shared<etpFesppDirectedDiscoveryProtocolHandlers>(this, my_etp_document));
	setStoreProtocolHandlers(std::make_shared<etpFesppStoreProtocolHandlers>(this));
	setDataArrayProtocolHandlers(std::make_shared<ETP_NS::DataArrayHandlers>(this));

	id = std::time(NULL);
    std::cout << "id client session: " << id << "\n";
}

etpClientSession::~etpClientSession()
{
	epcDoc.close();
}

std::vector<std::string> etpClientSession::listCommand()
{
    std::cout << "id client session: " << id << "\n";
	std::vector<std::string> result;
	std::string command = "GetContent";
	result.push_back(command);
	command = "GetSources";
	result.push_back(command);
	command = "GetTargets";
	result.push_back(command);
	command = "GetObject";
	result.push_back(command);
	command = "GetDataArray";
	result.push_back(command);
	command = "List";
	result.push_back(command);
	command = "quit";
	result.push_back(command);

	return result;
}

void etpClientSession::pushCommand(std::string command)
{
	if (command == "quit") {
		close();
	}
	else if (command.substr(0, 10) == "GetContent") {
		Energistics::Etp::v12::Protocol::DirectedDiscovery::GetContent mb;
		mb.m_uri = command.size() > 11 ? command.substr(11) : "";
		send(mb);
	}
	else if (command.substr(0, 10) == "GetSources") {
		Energistics::Etp::v12::Protocol::DirectedDiscovery::GetSources mb;
		mb.m_uri = command.size() > 11 ? command.substr(11) : "";
		send(mb);
	}
	else if (command.substr(0, 10) == "GetTargets") {
		Energistics::Etp::v12::Protocol::DirectedDiscovery::GetTargets mb;
		mb.m_uri = command.size() > 11 ? command.substr(11) : "";
		send(mb);
	}
	else if (command.substr(0, 9) == "GetObject") {
		Energistics::Etp::v12::Protocol::Store::GetObject getO;
		getO.m_uri = command.size() > 10 ? command.substr(10) : "";
		send(getO);
	}
	else if (command.substr(0, 12) == "GetDataArray") {
		Energistics::Etp::v12::Protocol::DataArray::GetDataArray gda;
		size_t lastSpace = command.rfind(' ');
		gda.m_uri = command.size() > 13 ? command.substr(13, lastSpace-13) : "";
		++lastSpace;
		gda.m_pathInResource = command.substr(lastSpace);
		std::cout << gda.m_pathInResource << std::endl;
		send(gda);
	}
}
