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
#include "EtpClientSession.h"

#include <fesapi/etp/EtpHdfProxy.h>

EtpClientSession::EtpClientSession(
	const std::string & host, const std::string & port, const std::string & target, const std::string & authorization,
	const std::vector<Energistics::Etp::v12::Datatypes::SupportedProtocol> & requestedProtocols,
	const std::vector<std::string>& supportedObjects,
	VtkEpcCommon::modeVtkEpc mode) :
	ETP_NS::PlainClientSession(host, port, target, authorization, requestedProtocols, supportedObjects),
	treeViewMode(mode == VtkEpcCommon::Both || mode == VtkEpcCommon::TreeView), representationMode(mode == VtkEpcCommon::Both || mode == VtkEpcCommon::Representation),
	connectionError(false)
{
	waiting = false;
	repo.setHdfProxyFactory(new ETP_NS::EtpHdfProxyFactory());
}

volatile bool EtpClientSession::isWaitingForAnswer() const {
	//return !messageIdToBeAnswered.empty();
	return waiting;
}

void EtpClientSession::insertMessageIdTobeAnswered(int64_t messageId) {
	messageIdToBeAnswered.insert(messageId);
	waiting = true;
}

void EtpClientSession::eraseMessageIdTobeAnswered(int64_t messageId) {
	messageIdToBeAnswered.erase(messageId);
	waiting = !messageIdToBeAnswered.empty();
}
