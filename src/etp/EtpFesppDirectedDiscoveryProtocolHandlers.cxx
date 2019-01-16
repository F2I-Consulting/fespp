#include <etp/VtkEtpDocument.h>
#include "EtpFesppDirectedDiscoveryProtocolHandlers.h"
#include <algorithm>


void EtpFesppDirectedDiscoveryProtocolHandlers::on_GetResourcesResponse(const Energistics::Etp::v12::Protocol::DirectedDiscovery::GetResourcesResponse & grr, int64_t correlationId)
{
	Energistics::Etp::v12::Datatypes::Object::GraphResource graphResource =  grr.m_resource;

	int32_t sourceCount, targetCount, contentCount;
	sourceCount = (!graphResource.m_sourceCount.is_null()) ? graphResource.m_sourceCount.get_int() : 0;
	targetCount = (!graphResource.m_targetCount.is_null()) ? graphResource.m_targetCount.get_int() : 0;
	contentCount = (!graphResource.m_contentCount.is_null()) ? graphResource.m_contentCount.get_int() : 0;

	if (std::find(getObjectWhenDiscovered.begin(), getObjectWhenDiscovered.end(), correlationId) != getObjectWhenDiscovered.end()) {
		size_t openingParenthesis = graphResource.m_uri.find('(', 5);
		if (openingParenthesis != std::string::npos) {
			auto resqmlObj = static_cast<EtpClientSession*>(session)->epcDoc.getResqmlAbstractObjectByUuid(graphResource.m_uri.substr(openingParenthesis + 1, 36));
			if (resqmlObj == nullptr || resqmlObj->isPartial()) {
				Energistics::Etp::v12::Protocol::Store::GetObject_ getO;
				getO.m_uri = graphResource.m_uri;
				static_cast<EtpClientSession*>(session)->newAnsweredMessages(session->send(getO));
			}
		}
	}

	static_cast<EtpClientSession*>(session)->receivedAnsweredMessages(correlationId);

	if(static_cast<EtpClientSession*>(session)->isTreeViewMode()) {
		etp_document->receive_resources_tree(graphResource.m_uri,
				graphResource.m_contentType,
				graphResource.m_name,
				graphResource.m_resourceType,
				sourceCount,
				targetCount,
				contentCount,
				graphResource.m_lastChanged
		);
	}
}

