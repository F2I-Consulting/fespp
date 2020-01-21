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
#ifndef _IjkGridRepDiscoveryHandler_h
#define _IjkGridRepDiscoveryHandler_h

#include <fesapi/etp/ProtocolHandlers/DiscoveryHandlers.h>

#include <etp/EtpClientSession.h>
class VtkEtpDocument;

class IjkGridRepDiscoveryHandler : public ETP_NS::DiscoveryHandlers
{
public:
	IjkGridRepDiscoveryHandler(std::shared_ptr<EtpClientSession> my_session, VtkEtpDocument* my_etp_document):ETP_NS::DiscoveryHandlers(my_session), etp_document(my_etp_document) {}
	~IjkGridRepDiscoveryHandler() {}

	void on_GetResourcesResponse(const Energistics::Etp::v12::Protocol::Discovery::GetResourcesResponse & msg, int64_t correlationId);

private:
	VtkEtpDocument* etp_document;
};
#endif
