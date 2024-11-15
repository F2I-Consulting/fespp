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

#include <exception>
#include <iterator>
#include <algorithm>
#include <limits>
#include <sstream>

#include "vtkEnergisticsExtractor.h"

#include <vtkIndent.h>
#include <vtkInformation.h>
#include <vtkInformationVector.h>
#include <vtkPartitionedDataSetCollection.h>
#include <vtkDataAssembly.h>
#include <vtkObjectFactory.h>
#include <vtkMultiProcessController.h>
#include <vtkStreamingDemandDrivenPipeline.h>
#include <vtkDataObject.h>
#include <vtkPVDataInformation.h>

#include <vtkPartitionedDataSet.h>

#include "vtkSMInputProperty.h"
#include "vtkSMProxyManager.h"
#include "vtkSMSessionProxyManager.h"
#include "vtkSMSourceProxy.h"
#include "vtkSMViewProxy.h"
#include "vtkSMRenderViewProxy.h"
#include "vtkSMRepresentationProxy.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMProxyIterator.h"
#include <vtkSMParaViewPipelineController.h>
#include <vtkSMOutputPort.h>
#include <vtkCommand.h>
#include <vtkSMProperty.h>
#include <vtkDataSet.h>

#include <vtkSMSession.h>
#include <vtkCollection.h>
#include <vtkCollectionIterator.h>

vtkStandardNewMacro(vtkEPCReader);

//----------------------------------------------------------------------------
vtkEPCReader::vtkEPCReader()
{
	SetNumberOfInputPorts(0);

	vtkSMProxyManager* proxyManager = vtkSMProxyManager::GetProxyManager();
	vtkSMSession* session = proxyManager->GetActiveSession();

	vtkSMSourceProxy* collectorEPCProxy = nullptr;
	vtkNew<vtkSMProxyIterator> iter;
	iter->SetSession(session);
	for (iter->Begin("sources"); !iter->IsAtEnd(); iter->Next())
	{
		vtkSMSourceProxy* sourceProxy = vtkSMSourceProxy::SafeDownCast(iter->GetProxy());
		std::string proxyName = sourceProxy->GetXMLName();
		if (proxyName == "EPCCollector")
		{
			collectorEPCProxy = sourceProxy;
		}
	}

	vtkSMSessionProxyManager* sessionProxyManager = vtkSMProxyManager::GetProxyManager()->GetActiveSessionProxyManager();

	if (!collectorEPCProxy)
	{

		vtkSMSourceProxy* collectorEPCProxy = vtkSMSourceProxy::SafeDownCast(sessionProxyManager->NewProxy("sources", "EPCCollector"));;

		//vtkSMPropertyHelper(collecterEPCProxy, "Files").Set(FileName);

		vtkNew<vtkSMParaViewPipelineController> controller;
		controller->InitializeProxy(collectorEPCProxy);
		controller->RegisterPipelineProxy(collectorEPCProxy, "EPC Collector");
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
		for (iter->Begin("sources"); !iter->IsAtEnd(); iter->Next())
		{
			vtkSMSourceProxy* sourceProxy = vtkSMSourceProxy::SafeDownCast(iter->GetProxy());
			std::string proxyName = sourceProxy->GetXMLName();
			if (proxyName == "EPCCollector")
			{
				vtkSMPropertyHelper(sourceProxy, "Files").Set(fname);
				//vtkSMPropertyHelper(sourceProxy, "MarkerSize").Set(100);
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
