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
	ExtractPath(""),
	dataType("vtkDataObject")
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
	try {
		// Get the type of the first partition in the requested partitioned dataset
		vtkDataObject* inputData = this->GetInputDataObject(0, 0);
		vtkPartitionedDataSetCollection* input = vtkPartitionedDataSetCollection::SafeDownCast(inputData);
		if (input)
		{
			if (input->GetDataAssembly())
			{
				int node = input->GetDataAssembly()->GetFirstNodeByPath(ExtractPath.c_str());
				if (node > -1)
				{
					dataType = input->GetPartitionedDataSet(input->GetDataAssembly()->GetDataSetIndices(node)[0])->GetPartition(0)->GetClassName();
				}
			}
		}

		info->Set(vtkDataObject::DATA_TYPE_NAME(), dataType);
	}
	catch (const std::exception& e)
	{
		vtkOutputWindowDisplayErrorText((std::string("vtkEnergisticsExtractor error > ") + e.what()).c_str());
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
		vtkErrorMacro("Missing Input!");
		return 0;
	}

	int partitionIndex = -1;
	if (input->GetDataAssembly())
	{
		
		int node = input->GetDataAssembly()->GetFirstNodeByPath(ExtractPath.c_str());
		if (node > -1)
		{
			partitionIndex = input->GetDataAssembly()->GetDataSetIndices(node)[0];
		}
	}
	vtkSmartPointer<vtkDataObject> ouput = vtkSmartPointer<vtkDataObject>::New();

	// Get the requested partitioned dataset
	vtkPartitionedDataSet* partitionedDataSet = input->GetPartitionedDataSet(partitionIndex);
	if (partitionedDataSet)
	{
		if (partitionedDataSet->GetNumberOfPartitions() > 0)
		{
			// Create a copy of the partitioned dataset
			vtkSmartPointer<vtkPartitionedDataSet> outputPartitionedDataSet = vtkSmartPointer<vtkPartitionedDataSet>::New();
			ouput = partitionedDataSet->GetPartitionAsDataObject(0);
		}
	}

	vtkDataSet::GetData(info)->DeepCopy(ouput);
	return 1;
}
