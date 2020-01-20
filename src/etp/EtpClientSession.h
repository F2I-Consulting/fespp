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
#ifndef _etpClientSession_h_
#define _etpClientSession_h_

#include <set>

// include FESAPI
#include <fesapi/etp/PlainClientSession.h>
#include <fesapi/common/EpcDocument.h>

//include Fespp
#include "VTK/VtkEpcCommon.h"

class VtkEtpDocument;
class EtpClientSession : public ETP_NS::PlainClientSession
{
public:
	/**
	 * @param host		The IP address on which the server is listening for etp (websocket) connection
	 * @param port		The port on which the server is listening for etp (websocket) connection
	 * @param requestedProtocols An array of protocol IDs that the client expects to communicate on for this session. If the server does not support all of the protocols, the client may or may not continue with the protocols that are supported.
	 * @param supportedObjects		A list of the Data Objects supported by the client. This list MUST be empty if the client is a customer. This field MUST be supplied if the client is a Store and is requesting a customer role for the server.
	 */
	EtpClientSession(
			const std::string & host, const std::string & port, const std::string & target, const std::string & authorization,
			const std::vector<Energistics::Etp::v12::Datatypes::SupportedProtocol> & requestedProtocols,
			const std::vector<std::string>& supportedObjects,
			const VtkEpcCommon::modeVtkEpc & mode);

	~EtpClientSession() {};

	bool isTreeViewMode() { return treeViewMode;}

	/**
	* Indicates if the session is still waiting for answer or not.
	* @return True if the session is not waiting for any message else False
	*/
	bool isWaitingForAnswer() const;

	/**
	* Insert a message id which requires an answer by the server.
	* @param messageId	The message id which requires an answer from the server
	*/
	void insertMessageIdTobeAnswered(int64_t messageId);

	/**
	* Erase a message id from the list of message if to be answered.
	* Generally it means that the associated message hase been answered by the server.
	* @param messageId	The message id which must be removed
	*/
	void eraseMessageIdTobeAnswered(int64_t messageId);

	COMMON_NS::DataObjectRepository repo;

private:
	/**
	* A set of message ids which has not been answered yet by the server
	* Does not use a vector because we want direct access to messageId instead of direct access to index.
	*/
	std::set<int64_t> messageIdToBeAnswered;
	bool treeViewMode;
	bool representationMode;
};
#endif
