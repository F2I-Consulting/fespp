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
#include "GetWhenDiscoverHandler.h"

#include <fesapi/common/AbstractObject.h>

#include "EtpClientSession.h"

void GetWhenDiscoverHandler::on_GetResourcesResponse(const Energistics::Etp::v12::Protocol::Discovery::GetResourcesResponse & msg, int64_t correlationId)
{
	std::cout << msg.m_resources.size() << " resources received which will be got." << std::endl;

	Energistics::Etp::v12::Protocol::Store::GetDataObjects toGetMsg;
	for (Energistics::Etp::v12::Datatypes::Object::Resource resource : msg.m_resources) {
		auto dataObject = std::static_pointer_cast<EtpClientSession>(session)->repo.getDataObjectByUuid(resource.m_uri);
		if (dataObject == nullptr || dataObject->isPartial()) {
			toGetMsg.m_uris[resource.m_uri] = resource.m_uri;
		}
	}

	if (!toGetMsg.m_uris.empty()) {
		std::static_pointer_cast<EtpClientSession>(session)->insertMessageIdTobeAnswered(session->send(toGetMsg));
	}

	std::static_pointer_cast<EtpClientSession>(session)->eraseMessageIdTobeAnswered(correlationId);
}
