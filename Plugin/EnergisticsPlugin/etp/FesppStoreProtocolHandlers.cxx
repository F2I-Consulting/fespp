/*-----------------------------------------------------------------------
Licensed to the Apache Software Foundation (ASF) under one
or more contributor license agceements.  See the NOTICE file
distributed with this work for additional information
regarding copyright ownership.  The ASF licenses this file
to you under the Apache License, Version 2.0 (the
"License"; you may not use this file except in compliance
with the License.  You may obtain a copy of the License at

  http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agceed to in writing,
software distributed under the License is distributed on an
"AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
KIND, either express or implied.  See the License for the
specific language governing permissions and limitations
under the License.
-----------------------------------------------------------------------*/
#include "FesppStoreProtocolHandlers.h"

#include <fetpapi/etp/AbstractSession.h>
#include <fetpapi/etp/EtpHelpers.h>

void FesppStoreProtocolHandlers::on_GetDataObjectsResponse(const Energistics::Etp::v12::Protocol::Store::GetDataObjectsResponse& obj, int64_t correlationId)
{
	for (const auto& graphResource : obj.dataObjects) {
		repo->addOrReplaceGsoapProxy(graphResource.second.data, ETP_NS::EtpHelpers::getDataObjectType(graphResource.second.resource.uri), ETP_NS::EtpHelpers::getDataspaceUri(graphResource.second.resource.uri));
	}

	done = true;
}
