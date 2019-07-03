#include "etp/EtpFesppStoreProtocolHandlers.h"
#include <etp/VtkEtpDocument.h>


void EtpFesppStoreProtocolHandlers::on_GetDataObjectsResponse(const Energistics::Etp::v12::Protocol::Store::GetDataObjectsResponse & obj, int64_t correlationId)
{
	std::cout << " store received." << std::endl;
	for (const auto & graphResource : obj.m_dataObjects) {
			COMMON_NS::AbstractObject* importedObj  = dynamic_cast<EtpClientSession*>(session)->epcDoc.addOrReplaceGsoapProxy(graphResource.m_data, graphResource.m_resource.m_contentType);
			importedObj->resolveTargetRelationships(&dynamic_cast<EtpClientSession*>(session)->epcDoc);
	}
	static_cast<EtpClientSession*>(session)->eraseMessageIdTobeAnswered(correlationId);
}
