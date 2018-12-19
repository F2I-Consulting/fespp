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
#include <etp/VtkEtpDocument.h>
#include "EtpFesppDirectedDiscoveryProtocolHandlers.h"
#include <algorithm>

EtpFesppDirectedDiscoveryProtocolHandlers::EtpFesppDirectedDiscoveryProtocolHandlers(EtpClientSession* my_session, VtkEtpDocument* my_etp_document, const VtkEpcCommon::modeVtkEpc & mode)
: ETP_NS::DirectedDiscoveryHandlers(my_session), etp_document(my_etp_document)
{
	if (mode==VtkEpcCommon::Both || mode==VtkEpcCommon::TreeView)
	{
		treeViewMode=true;;
	}
	if (mode==VtkEpcCommon::Both || mode==VtkEpcCommon::Representation)
	{
		representationMode=true;
	}
}

void EtpFesppDirectedDiscoveryProtocolHandlers::on_GetResourcesResponse(const Energistics::Etp::v12::Protocol::DirectedDiscovery::GetResourcesResponse & grr, int64_t correlationId)
{
	Energistics::Etp::v12::Datatypes::Object::GraphResource graphResource =  grr.m_resource;

	std::cout << "*******EtpFesppDirectedDiscoveryProtocolHandlers*******" << std::endl;
	std::cout << "GraphResource received : " << std::endl;
	std::cout << "uri : " << graphResource.m_uri << std::endl;
	std::cout << "contentType : " << graphResource.m_contentType << std::endl;
	std::cout << "name : " << graphResource.m_name << std::endl;
	std::cout << "type : " << static_cast<size_t>(graphResource.m_resourceType) << std::endl;
	if (!graphResource.m_sourceCount.is_null())
		std::cout << "sourceCount : " << graphResource.m_sourceCount.get_int() << std::endl;
	if (!graphResource.m_targetCount.is_null())
		std::cout << "targetCount : " << graphResource.m_targetCount.get_int() << std::endl;
	if (!graphResource.m_contentCount.is_null())
		std::cout << "contentCount : " << graphResource.m_contentCount.get_int() << std::endl;
	std::cout << "lastChanged : " << graphResource.m_lastChanged << std::endl;
	std::cout << "*************************************************" << std::endl;

	int32_t sourceCount, targetCount, contentCount;
	if (!graphResource.m_sourceCount.is_null())
		sourceCount = graphResource.m_sourceCount.get_int();
	else
		sourceCount = 0;
	if (!graphResource.m_targetCount.is_null())
		targetCount = graphResource.m_targetCount.get_int();
	else
		targetCount = 0;
	if (!graphResource.m_contentCount.is_null())
		contentCount = graphResource.m_contentCount.get_int();
	else
		contentCount = 0;

	if (std::find(getObjectWhenDiscovered.begin(), getObjectWhenDiscovered.end(), correlationId) != getObjectWhenDiscovered.end()) {
		size_t openingParenthesis = graphResource.m_uri.find('(', 5);
		if (openingParenthesis != std::string::npos) {
			auto resqmlObj = static_cast<EtpClientSession*>(session)->epcDoc.getResqmlAbstractObjectByUuid(graphResource.m_uri.substr(openingParenthesis + 1, 36));
			if (resqmlObj == nullptr || resqmlObj->isPartial()) {
				std::cout << "GET OBJECT -----------------------------------------------------------------------------------------------------------------------------------------------------------------------" << std::endl;
				etp_document->add_command("Wait");
				etp_document->add_command("Wait");
				Energistics::Etp::v12::Protocol::Store::GetObject_ getO;
				getO.m_uri = graphResource.m_uri;
				session->send(getO);
//				etp_document->add_command("GetObject "+graphResource.m_uri);

			}
		}
	}
//	etp_document->receive_resources_finished();

	if(treeViewMode)
	{
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
	if (representationMode)
	{
		etp_document->receive_resources_representation(graphResource.m_uri,
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
