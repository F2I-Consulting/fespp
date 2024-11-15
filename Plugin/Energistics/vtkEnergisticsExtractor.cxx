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
#include "vtkEnergisticsExtractor.h"

#include <vtkDataAssembly.h>
#include <vtkPartitionedDataSetCollection.h>
#include <vtkObjectFactory.h>
#include <vtkPVDataInformation.h>
#include <vtkInformation.h>
#include <vtkInformationVector.h>
#include <vtkDataSet.h>
#include <vtkPolyData.h>
#include <vtkPartitionedDataSet.h>
#include <vtkDemandDrivenPipeline.h>
#include <memory>

#include <sstream>

vtkStandardNewMacro(vtkEnergisticsExtractor);

//-----------------------------------------------------------------------------
vtkEnergisticsExtractor::vtkEnergisticsExtractor() :
	PartitionIndex(0),
	PartitionType(0)
{
	this->SetNumberOfInputPorts(1);
	this->SetNumberOfOutputPorts(1);
}

//-----------------------------------------------------------------------------
vtkEnergisticsExtractor::~vtkEnergisticsExtractor()
{
}

//----------------------------------------------------------------------------
int vtkEnergisticsExtractor::ProcessRequest(
	vtkInformation* request, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
	// generate the data
	if (request->Has(vtkDemandDrivenPipeline::REQUEST_DATA()))
	{
		return this->RequestData(request, inputVector, outputVector);
	}

	return this->Superclass::ProcessRequest(request, inputVector, outputVector);
}

//-----------------------------------------------------------------------------
int vtkEnergisticsExtractor::FillInputPortInformation(int vtkNotUsed(port), vtkInformation* info)
{
	info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkPartitionedDataSetCollection");
	return 1;
}

//------------------------------------------------------------------------------
int vtkEnergisticsExtractor::FillOutputPortInformation(
	int vtkNotUsed(port), vtkInformation* info)
{
	vtkOutputWindowDisplayText("FillOutputPortInformation\n");
	vtkOutputWindowDisplayText(ExtractPath.c_str());
	vtkOutputWindowDisplayText("\n");

	// Get the type of the first partition in the requested partitioned dataset
	vtkDataObject* inputData = this->GetInputDataObject(0, 0);
	vtkPartitionedDataSetCollection* input = vtkPartitionedDataSetCollection::SafeDownCast(inputData);
	if (input)
	{
		if (input->GetPartitionedDataSet(this->PartitionIndex)->GetNumberOfPartitions() > 0)
		{
			vtkDataSet* firstPartition = input->GetPartitionedDataSet(this->PartitionIndex)->GetPartition(0);
			info->Set(vtkDataObject::DATA_TYPE_NAME(), firstPartition->GetClassName());
		}
		else
		{
			info->Set(vtkDataObject::DATA_TYPE_NAME(), "");
		}
	}
	else
	{
		vtkOutputWindowDisplayText("input manquant\n");
		info->Set(vtkDataObject::DATA_TYPE_NAME(), "");
	}

	return 1;
}

//----------------------------------------------------------------------------
int vtkEnergisticsExtractor::RequestData(vtkInformation* vtkNotUsed(request),
	vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
	vtkInformation* info = outputVector->GetInformationObject(0);
	vtkPartitionedDataSetCollection* input = vtkPartitionedDataSetCollection::GetData(inputVector[0]);

	if (!input)
	{
		vtkErrorMacro("Missing input!");
		return 0;
	}

	vtkSmartPointer<vtkDataObject> ouput = vtkSmartPointer<vtkDataObject>::New();

	// Get the requested partitioned dataset
	vtkPartitionedDataSet* partitionedDataSet = input->GetPartitionedDataSet(this->PartitionIndex);
	if (partitionedDataSet)
	{
		if (partitionedDataSet->GetNumberOfPartitions() > 0)
		{
			// **TODO** faire avec path

			// Create a copy of the partitioned dataset
			vtkSmartPointer<vtkPartitionedDataSet> outputPartitionedDataSet = vtkSmartPointer<vtkPartitionedDataSet>::New();
			ouput = partitionedDataSet->GetPartitionAsDataObject(0);

			// **TODO** multi partition => append each partition
		}
	}

	vtkDataSet::GetData(info)->DeepCopy(ouput);
	return 1;
}
