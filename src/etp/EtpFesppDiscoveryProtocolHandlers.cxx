/*-----------------------------------------------------------------------
Licensed to the Apache Software Foundation (ASF) under one
or more contributor license agreements.  See the NOTICE file
distributed with this work for additional information
regarding copyright ownership.  The ASF licenses this file
to you under the Apache License, Version 2.0 (the
"License"; you may not use this file except in compliance
with the License.  You may obtain a copy of the License at

  http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing,
software distributed under the License is distributed on an
"AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
KIND, either express or implied.  See the License for the
specific language governing permissions and limitations
under the License.
-----------------------------------------------------------------------*/
#include "EtpFesppDiscoveryProtocolHandlers.h"

#include "etp/VtkEtpDocument.h"

void EtpFesppDiscoveryProtocolHandlers::on_GetResourcesResponse(const Energistics::Etp::v12::Protocol::Discovery::GetResourcesResponse & msg, int64_t correlationId)
{
	Energistics::Etp::v12::Protocol::Store::GetDataObjects getO;
	size_t index = 0;

	std::cout << msg.m_resources.size() << " resources received." << std::endl;
	if(std::static_pointer_cast<EtpClientSession>(session)->isTreeViewMode()) {
		etp_document->receive_nbresources_tree(msg.m_resources.size());
	}
	for (Energistics::Etp::v12::Datatypes::Object::Resource resource : msg.m_resources) {
		auto sourceCount = (!resource.m_sourceCount.is_null()) ? resource.m_sourceCount.get_int() : 0;
//		auto targetCount = (!graphResource.m_targetCount.is_null()) ? graphResource.m_targetCount.get_int() : 0;
//		auto contentCount = (!graphResource.m_contentCount.is_null()) ? graphResource.m_contentCount.get_int() : 0;

		// Check if the client also needs to get the object once discovered.
		if (std::find(getObjectWhenDiscovered.begin(), getObjectWhenDiscovered.end(), correlationId) != getObjectWhenDiscovered.end()) {
			size_t openingParenthesis = resource.m_uri.find('(', 5);
			if (openingParenthesis != std::string::npos) {
				auto resqmlObj = std::static_pointer_cast<EtpClientSession>(session)->repo.getDataObjectByUuid(resource.m_uri.substr(openingParenthesis + 1, 36));
				if (resqmlObj == nullptr || resqmlObj->isPartial()) {
					getO.m_uris[std::to_string(index)] = resource.m_uri;
					++index;
				}
			}
		}

		if(std::static_pointer_cast<EtpClientSession>(session)->isTreeViewMode()) {
			etp_document->receive_resources_tree(resource.m_uri, resource.m_name,resource.m_dataObjectType, sourceCount);
		}
	}

	if (!getO.m_uris.empty()) {
		std::static_pointer_cast<EtpClientSession>(session)->insertMessageIdTobeAnswered(session->send(getO));
	}

	std::static_pointer_cast<EtpClientSession>(session)->eraseMessageIdTobeAnswered(correlationId);
}
