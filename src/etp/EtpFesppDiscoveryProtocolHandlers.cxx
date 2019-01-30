#include <etp/VtkEtpDocument.h>
#include "EtpFesppDiscoveryProtocolHandlers.h"
#include <algorithm>


void EtpFesppDiscoveryProtocolHandlers::on_GetResourcesResponse(const Energistics::Etp::v12::Protocol::Discovery::GetResourcesResponse & grr, int64_t correlationId)
{
	std::cout << grr.m_resources.size() << " resources received." << std::endl;
	if(static_cast<EtpClientSession*>(session)->isTreeViewMode()) {
		etp_document->receive_nbresources_tree(grr.m_resources.size());
	}
	for (Energistics::Etp::v12::Datatypes::Object::Resource graphResource : grr.m_resources) {
		auto sourceCount = (!graphResource.m_sourceCount.is_null()) ? graphResource.m_sourceCount.get_int() : 0;
//		auto targetCount = (!graphResource.m_targetCount.is_null()) ? graphResource.m_targetCount.get_int() : 0;
//		auto contentCount = (!graphResource.m_contentCount.is_null()) ? graphResource.m_contentCount.get_int() : 0;

		if (std::find(getObjectWhenDiscovered.begin(), getObjectWhenDiscovered.end(), correlationId) != getObjectWhenDiscovered.end()) {
			size_t openingParenthesis = graphResource.m_uri.find('(', 5);
			if (openingParenthesis != std::string::npos) {
				auto resqmlObj = static_cast<EtpClientSession*>(session)->epcDoc.getResqmlAbstractObjectByUuid(graphResource.m_uri.substr(openingParenthesis + 1, 36));
				if (resqmlObj == nullptr || resqmlObj->isPartial()) {
					Energistics::Etp::v12::Protocol::Store::GetDataObjects getO;
					getO.m_uris.push_back(graphResource.m_uri);
					static_cast<EtpClientSession*>(session)->newAnsweredMessages(session->send(getO));
				}
			}
		}

		if(static_cast<EtpClientSession*>(session)->isTreeViewMode()) {
			etp_document->receive_resources_tree(graphResource.m_uri, graphResource.m_name,graphResource.m_contentType, sourceCount);
		}
	}
	static_cast<EtpClientSession*>(session)->receivedAnsweredMessages(correlationId);
}

