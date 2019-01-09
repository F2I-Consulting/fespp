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
#include "etp/EtpFesppStoreProtocolHandlers.h"
#include <etp/VtkEtpDocument.h>


void EtpFesppStoreProtocolHandlers::on_Object(const Energistics::Etp::v12::Protocol::Store::Object & obj, int64_t correlationId)
{
	auto graphResource =  obj.m_dataObject;

	COMMON_NS::AbstractObject* importedObj  = static_cast<EtpClientSession*>(session)->epcDoc.addOrReplaceGsoapProxy(graphResource.m_data, graphResource.m_resource.m_contentType);

	importedObj->resolveTargetRelationships(&static_cast<EtpClientSession*>(session)->epcDoc);

	cout << "on_Object " << correlationId << endl;
	static_cast<EtpClientSession*>(session)->answeredMessages[correlationId] = true;
}
