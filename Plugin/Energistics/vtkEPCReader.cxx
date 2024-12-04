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
#include "vtkEPCReader.h"

#include "vtkEnergisticsExtractor.h"

#include <vtkSMProxyManager.h>
#include <vtkSMSessionProxyManager.h>
#include <vtkSMSourceProxy.h>
#include <vtkSMViewProxy.h>
#include <vtkSMRenderViewProxy.h>
#include <vtkSMPropertyHelper.h>
#include <vtkSMProxyIterator.h>
#include <vtkSMParaViewPipelineControllerWithRendering.h>
#include <vtkSMSession.h>
#include <vtkPVRenderView.h>

vtkStandardNewMacro(vtkEPCReader);

//----------------------------------------------------------------------------
vtkEPCReader::vtkEPCReader()
{
	SetNumberOfInputPorts(0);

	vtkSMProxyManager* proxyManager = vtkSMProxyManager::GetProxyManager();
	vtkSMSession* session = proxyManager->GetActiveSession();

	vtkSMSourceProxy* collectorEPCProxy = nullptr;
	vtkSMViewProxy* collectorEPCview = nullptr;
	vtkNew<vtkSMProxyIterator> iterProxy;
	iterProxy->SetSession(session);
	// search EPC Collector proxy
	for (iterProxy->Begin("sources"); !iterProxy->IsAtEnd(); iterProxy->Next())
	{
		vtkSMSourceProxy* sourceProxy = vtkSMSourceProxy::SafeDownCast(iterProxy->GetProxy());
		std::string proxyName = sourceProxy->GetXMLName();
		if (proxyName == "EPCCollector")
		{
			collectorEPCProxy = sourceProxy;
		}
	}

	if (!collectorEPCProxy)
	{
		//
		// create ParaView pipeline EPC Collector source
		//

		// search a RenderView to link EPC Collector
		for (iterProxy->Begin("views"); !iterProxy->IsAtEnd(); iterProxy->Next())
		{
			vtkSMViewProxy* view = vtkSMViewProxy::SafeDownCast(iterProxy->GetProxy());
			if (view && vtkPVRenderView::SafeDownCast(view->GetClientSideView()))
			{
				collectorEPCview = view;
				break;
			}
		}

		vtkSMSessionProxyManager* sessionProxyManager = proxyManager->GetActiveSessionProxyManager();
		vtkSMSourceProxy* collectorEPCProxy = vtkSMSourceProxy::SafeDownCast(sessionProxyManager->NewProxy("sources", "EPCCollector"));;

		// create ParaView pipeline
		vtkNew<vtkSMParaViewPipelineControllerWithRendering> controller;
		controller->InitializeProxy(collectorEPCProxy);
		collectorEPCProxy->SetSession(session);
		controller->RegisterPipelineProxy(collectorEPCProxy, "EPC Collector");
		// link ParaView pipeline with PVRenderView
		controller->SetVisibility(collectorEPCProxy, 0, collectorEPCview, true);
	}
}

vtkEPCReader::~vtkEPCReader()
{
}

//----------------------------------------------------------------------------
void vtkEPCReader::AddFileNameToFiles(const char* fname)
{
	if (fname != nullptr)
	{
		vtkSMProxyManager* proxyManager = vtkSMProxyManager::GetProxyManager();
		vtkSMSession* session = proxyManager->GetActiveSession();

		vtkSMSourceProxy* readerProxy = nullptr;
		vtkNew<vtkSMProxyIterator> iter;
		iter->SetSession(session);
		// search EPC Collector Proxy
		for (iter->Begin("sources"); !iter->IsAtEnd(); iter->Next())
		{
			vtkSMSourceProxy* sourceProxy = vtkSMSourceProxy::SafeDownCast(iter->GetProxy());
			std::string proxyName = sourceProxy->GetXMLName();
			if (proxyName == "EPCCollector")
			{
				// add FileName to EPC Collector Proxy
				vtkSMPropertyHelper(sourceProxy, "Files").Set(fname);
				sourceProxy->UpdateVTKObjects();
				sourceProxy->UpdatePipelineInformation();
			}
		}
	}
}

//----------------------------------------------------------------------------
void vtkEPCReader::ClearFileName()
{
}
