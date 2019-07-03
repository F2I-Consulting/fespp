
#include <etp/EtpClientSession.h>
#include <etp/EtpFesppDiscoveryProtocolHandlers.h>

#include <etp/ProtocolHandlers/CoreHandlers.h>
#include <etp/ProtocolHandlers/ProtocolHandlers.h>
#include <etp/ProtocolHandlers/DataArrayHandlers.h>
#include <etp/VtkEtpDocument.h>

// Fespp include
#include "etp/EtpFesppStoreProtocolHandlers.h"
#include "etp/EtpFesppDiscoveryProtocolHandlers.h"

EtpClientSession::EtpClientSession(boost::asio::io_context& ioc,
		const std::string & host, const std::string & port, const std::string & target, const std::string & authorization,
		const std::vector<Energistics::Etp::v12::Datatypes::SupportedProtocol> & requestedProtocols,
		const std::vector<std::string>& supportedObjects,
		VtkEtpDocument* my_etp_document,
		const VtkEpcCommon::modeVtkEpc & mode)
: ETP_NS::PlainClientSession(ioc, host, port, target, authorization, requestedProtocols, supportedObjects)
{
	treeViewMode = (mode==VtkEpcCommon::Both || mode==VtkEpcCommon::TreeView);
	representationMode = (mode==VtkEpcCommon::Both || mode==VtkEpcCommon::Representation);

	setCoreProtocolHandlers(std::make_shared<ETP_NS::CoreHandlers>(this));
	setDiscoveryProtocolHandlers(std::make_shared<EtpFesppDiscoveryProtocolHandlers>(this, my_etp_document));
	setStoreProtocolHandlers(std::make_shared<EtpFesppStoreProtocolHandlers>(this, my_etp_document));
	setDataArrayProtocolHandlers(std::make_shared<ETP_NS::DataArrayHandlers>(this));
}

bool EtpClientSession::isWaitingForAnswer() const {
	return !messageIdToBeAnswered.empty();
}

void EtpClientSession::insertMessageIdTobeAnswered(int64_t messageId) {
	messageIdToBeAnswered.insert(messageId);
}

void EtpClientSession::eraseMessageIdTobeAnswered(int64_t messageId) {
	messageIdToBeAnswered.erase(messageId);
}
