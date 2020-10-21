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
#include "PropertyDiscoveryHandler.h"

void PropertyDiscoveryHandler::on_GetResourcesResponse(const Energistics::Etp::v12::Protocol::Discovery::GetResourcesResponse & msg, int64_t correlationId)
{
	std::cout << msg.m_resources.size() << " property resources received." << std::endl;
	for (Energistics::Etp::v12::Datatypes::Object::Resource resource : msg.m_resources) {
		if(etp_document_->getClientSession()->isTreeViewMode()) {
			VtkEpcCommon leaf;
			leaf.setName(resource.m_name);
			leaf.setUuid(resource.m_uri);
			leaf.setType(VtkEpcCommon::PROPERTY);
			leaf.setParent(parent_);
			leaf.setParentType(parentType_);
			leaf.setTimeIndex(-1);
			leaf.setTimestamp(0);
			etp_document_->getTreeView().push_back(leaf);
		}
	}

	etp_document_->getClientSession()->eraseMessageIdTobeAnswered(correlationId);
}
