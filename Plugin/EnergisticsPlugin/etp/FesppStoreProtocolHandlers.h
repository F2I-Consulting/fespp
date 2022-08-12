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
#pragma once

#include "fetpapi/etp/ProtocolHandlers/StoreHandlers.h"

#include "fesapi/common/DataObjectRepository.h"

class FesppStoreProtocolHandlers : public ETP_NS::StoreHandlers
{
private:
	COMMON_NS::DataObjectRepository* repo;

	// Use of atomic because the FESPP ETP session runs on its own thread.
	// This variable is consequently written on a thread and read on another thread. This cannot work without "atomic".
	std::atomic<bool> done = false;

public:
	FesppStoreProtocolHandlers(ETP_NS::AbstractSession* mySession, COMMON_NS::DataObjectRepository* repo_) : ETP_NS::StoreHandlers(mySession), repo(repo_) {}
	~FesppStoreProtocolHandlers() = default;

	bool isDone() const { return done; }

	void on_GetDataObjectsResponse(const Energistics::Etp::v12::Protocol::Store::GetDataObjectsResponse & obj, int64_t correlationId);
};
