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
#include "IjkGridRepDiscoveryHandler.h"

#include "VtkEtpDocument.h"
#include "PropertyDiscoveryHandler.h"

void IjkGridRepDiscoveryHandler::on_GetResourcesResponse(const Energistics::Etp::v12::Protocol::Discovery::GetResourcesResponse & msg, int64_t correlationId)
{
	auto fesppSession = std::static_pointer_cast<EtpClientSession>(session);
	std::cout << msg.m_resources.size() << " ijk grid rep resources received." << std::endl;
	for (Energistics::Etp::v12::Datatypes::Object::Resource resource : msg.m_resources) {
		if(std::static_pointer_cast<EtpClientSession>(session)->isTreeViewMode()) {
			VtkEpcCommon leaf;
			leaf.setName(resource.m_name);
			leaf.setUuid(resource.m_uri);
			leaf.setType(VtkEpcCommon::IJK_GRID);
			leaf.setParent("EtpDoc");
			leaf.setParentType(VtkEpcCommon::INTERPRETATION);
			leaf.setTimeIndex(-1);
			leaf.setTimestamp(0);
			etp_document->getTreeView().push_back(leaf);

			//push_command("GetResources " + rec_uri + " sources 1 false resqml20.obj_ContinuousProperty");
			Energistics::Etp::v12::Protocol::Discovery::GetResources mb;
			mb.m_context.m_uri = resource.m_uri;
			mb.m_scope = Energistics::Etp::v12::Datatypes::Object::ContextScopeKind::sources;
			mb.m_context.m_depth = 1;
			mb.m_context.m_dataObjectTypes.push_back("resqml20.obj_ContinuousProperty");
			mb.m_context.m_dataObjectTypes.push_back("resqml20.obj_DiscreteProperty");
			auto msgId = session->sendWithSpecificHandler(mb, std::make_shared<PropertyDiscoveryHandler>(fesppSession, etp_document, resource.m_uri, VtkEpcCommon::IJK_GRID));
			fesppSession->insertMessageIdTobeAnswered(msgId);
		}
	}

	fesppSession->eraseMessageIdTobeAnswered(correlationId);
}
