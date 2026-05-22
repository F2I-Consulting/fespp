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
#include "vtkEPCViewClone.h"

#include <vtkObjectFactory.h>
#include <vtkPartitionedDataSetCollection.h>

vtkStandardNewMacro(vtkEPCViewClone);

//-----------------------------------------------------------------------------
vtkEPCViewClone::vtkEPCViewClone()
{
	// vtkPartitionedDataSetCollectionAlgorithm's default ctor already
	// sets one input + one output port, both typed
	// vtkPartitionedDataSetCollection. No override needed here.
}

//-----------------------------------------------------------------------------
int vtkEPCViewClone::RequestData(vtkInformation* /*request*/,
	vtkInformationVector** inputVector,
	vtkInformationVector* outputVector)
{
	auto* input = vtkPartitionedDataSetCollection::GetData(inputVector[0], 0);
	auto* output = vtkPartitionedDataSetCollection::GetData(outputVector, 0);

	if (input == nullptr || output == nullptr)
	{
		return 0;
	}

	// ShallowCopy on vtkPartitionedDataSetCollection brings over the
	// partitioned datasets (sharing the underlying vtkDataObject
	// buffers) AND the vtkDataAssembly — so the per-view clone
	// stays in lockstep with its upstream EPCCollector without any
	// per-update bookkeeping.
	output->ShallowCopy(input);
	return 1;
}
