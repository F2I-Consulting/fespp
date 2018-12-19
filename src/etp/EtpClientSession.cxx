
#include <etp/EtpClientSession.h>

#include <etp/ProtocolHandlers/CoreHandlers.h>
#include <etp/ProtocolHandlers/ProtocolHandlers.h>
#include <etp/ProtocolHandlers/DataArrayHandlers.h>
#include <etp/VtkEtpDocument.h>

// Fespp include
#include "etp/EtpFesppStoreProtocolHandlers.h"
#include "etp/EtpFesppDirectedDiscoveryProtocolHandlers.h"
#include "etp/EtpFesppCoreProtocolHandlers.h"

EtpClientSession::EtpClientSession(boost::asio::io_context& ioc,
		const std::string & host, const std::string & port,
		const std::vector<Energistics::Etp::v12::Datatypes::SupportedProtocol> & requestedProtocols,
		const std::vector<std::string>& supportedObjects,
		VtkEtpDocument* my_etp_document,
		const VtkEpcCommon::modeVtkEpc & mode)
: ETP_NS::ClientSession(ioc, host, port, "/", requestedProtocols, supportedObjects),
  epcDoc("/tmp/etp.epc", COMMON_NS::EpcDocument::ETP)
{
		setCoreProtocolHandlers(std::make_shared<ETP_NS::CoreHandlers>(this));
	//setCoreProtocolHandlers(std::make_shared<EtpFesppCoreProtocolHandlers>(this, my_etp_document));
	setDiscoveryProtocolHandlers(std::make_shared<ETP_NS::DiscoveryHandlers>(this));
	setDirectedDiscoveryProtocolHandlers(std::make_shared<EtpFesppDirectedDiscoveryProtocolHandlers>(this, my_etp_document, mode));
	setStoreProtocolHandlers(std::make_shared<EtpFesppStoreProtocolHandlers>(this, my_etp_document));
	setDataArrayProtocolHandlers(std::make_shared<ETP_NS::DataArrayHandlers>(this));

}
