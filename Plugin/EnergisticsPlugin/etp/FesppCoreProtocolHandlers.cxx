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
#include "FesppCoreProtocolHandlers.h"

#include <fetpapi/etp/AbstractSession.h>

void FesppCoreProtocolHandlers::on_OpenSession(const Energistics::Etp::v12::Protocol::Core::OpenSession & os, int64_t correlationId)
{
	Energistics::Etp::v12::Protocol::Discovery::GetResources mb;
	mb.context.uri = "eml:///dataspace('demo/tmp')";
	mb.scope = Energistics::Etp::v12::Datatypes::Object::ContextScopeKind::self;
	mb.context.depth = 1;
	mb.context.navigableEdges = Energistics::Etp::v12::Datatypes::Object::RelationshipKind::Primary;
	mb.countObjects = true;

	session->send(mb, 0, 0x02);
}
