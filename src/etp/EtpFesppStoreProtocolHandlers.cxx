#include "etp/EtpFesppStoreProtocolHandlers.h"
#include <etp/VtkEtpDocument.h>


void EtpFesppStoreProtocolHandlers::on_Object(const Energistics::Etp::v12::Protocol::Store::Object & obj, int64_t correlationId)
{
	auto graphResource =  obj.m_dataObject;

	COMMON_NS::AbstractObject* importedObj  = static_cast<EtpClientSession*>(session)->epcDoc.addOrReplaceGsoapProxy(graphResource.m_data, graphResource.m_resource.m_contentType);

	importedObj->resolveTargetRelationships(&static_cast<EtpClientSession*>(session)->epcDoc);

	static_cast<EtpClientSession*>(session)->receivedAnsweredMessages(correlationId);
}
