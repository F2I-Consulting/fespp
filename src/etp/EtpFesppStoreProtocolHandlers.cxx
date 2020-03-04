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
#include "etp/EtpFesppStoreProtocolHandlers.h"

void EtpFesppStoreProtocolHandlers::on_GetDataObjectsResponse(const Energistics::Etp::v12::Protocol::Store::GetDataObjectsResponse & msg, int64_t correlationId)
{
	std::cout << " store received." << std::endl;
	for (const auto & dataObject : msg.m_dataObjects) {
		COMMON_NS::AbstractObject* importedObj  = repo.addOrReplaceGsoapProxy(dataObject.second.m_data, dataObject.second.m_resource.m_dataObjectType);
		importedObj->loadTargetRelationships();
	}
	std::static_pointer_cast<EtpClientSession>(session)->eraseMessageIdTobeAnswered(correlationId);
}
