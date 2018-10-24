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
#include "etpFesppStoreProtocolHandlers.h"
#include "PQEtpPanel.h"
#include <QApplication>

namespace
{
	PQEtpPanel* getPQEtpPanel()
	{
	    // get multi-block inspector panel
		PQEtpPanel *panel = 0;
	    foreach(QWidget *widget, qApp->topLevelWidgets())
		{
			panel = widget->findChild<PQEtpPanel *>();

			if(panel)
	        {
			    break;
			}
		}
		return panel;
	}
}

void etpFesppStoreProtocolHandlers::on_Object(const Energistics::Etp::v12::Protocol::Store::Object & obj)
{
//	std::string result ="";
	auto graphResource =  obj.m_dataObject;
	std::cout << "*************************************************" << std::endl;
	std::cout << "Resource received : " << std::endl;
	std::cout << "uri : " << graphResource.m_resource.m_uri << std::endl;
	std::cout << "contentType : " << graphResource.m_resource.m_contentType << std::endl;
	std::cout << "name : " << graphResource.m_resource.m_name << std::endl;
	std::cout << "type : " << graphResource.m_resource.m_resourceType << std::endl;
	std::cout << "uuid : " << graphResource.m_resource.m_uuid << std::endl;
	std::cout << "*************************************************" << std::endl;

//	static_cast<MyOwnEtpClientSession*>(session)->epcDoc.addOrReplaceGsoapProxy(graphResource.m_data, graphResource.m_resource.m_contentType);

//	return result;
}
