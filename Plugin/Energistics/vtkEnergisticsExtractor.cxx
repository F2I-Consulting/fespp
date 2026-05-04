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
#include <vtkInformation.h>
#include <vtkInformationVector.h>
#include <vtkPartitionedDataSet.h>
#include <vtkPolyData.h>
#include <vtkImageData.h>
#include <vtkRectilinearGrid.h>
#include <vtkStructuredGrid.h>
#include <vtkExplicitStructuredGrid.h>
#include <vtkStreamingDemandDrivenPipeline.h>
#include <vtkDemandDrivenPipeline.h>
#include <vtkSmartPointer.h>

#include <cstring>

vtkStandardNewMacro(vtkEnergisticsExtractor);

//-----------------------------------------------------------------------------
vtkEnergisticsExtractor::vtkEnergisticsExtractor() :
	ExtractPath("")
{
	this->SetNumberOfInputPorts(1);
	// Single output port: the inner partition exposed in its actual VTK type
	// (vtkPolyData / vtkUnstructuredGrid / vtkExplicitStructuredGrid / etc.).
	// RequestInformation propagates the correct DATA_TYPE_NAME and (for
	// structured types) WHOLE_EXTENT downstream — without those, the Streaming
	// executive defaults to a structured policy and rejects the pipeline with
	// "No whole extent has been set".
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
	if (request->Has(vtkDemandDrivenPipeline::REQUEST_DATA()))
	{
		return this->RequestData(request, inputVector, outputVector);
	}
	if (request->Has(vtkDemandDrivenPipeline::REQUEST_DATA_OBJECT()))
	{
		return this->RequestDataObject(request, inputVector, outputVector);
	}
	if (request->Has(vtkDemandDrivenPipeline::REQUEST_INFORMATION()))
	{
		return this->RequestInformation(request, inputVector, outputVector);
	}

	return this->Superclass::ProcessRequest(request, inputVector, outputVector);
}

//----------------------------------------------------------------------------
// RequestDataObject runs early in the pipeline pass, before RequestInformation
// and RequestData. Its job is to populate the output port with a data object
// of the correct concrete class so downstream filters (vtkPVGeometryFilter,
// the display rep, etc.) can run their own RequestDataObject and decide what
// they're dealing with.
//
// FillOutputPortInformation only declares "vtkDataObject" (the abstract base)
// because we cannot know the actual partition class at construction time.
// Without this RequestDataObject override, the executive would allocate a
// generic vtkDataObject which downstream filters reject ("Missing input
// data") because they need a vtkDataSet subclass.
//
// We try to peek at the input's data assembly to determine the partition
// class. If unavailable (input pipeline not yet executed), we fall back to
// an empty vtkPolyData placeholder — that satisfies downstream filters; the
// next RequestDataObject pass will replace it with the real type once the
// input is populated.
int vtkEnergisticsExtractor::RequestDataObject(
	vtkInformation* /*request*/, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
	vtkInformation* outInfo = outputVector->GetInformationObject(0);

	// Try to peek at the source partition to determine the concrete output
	// class. If the input pipeline hasn't executed yet (assembly empty or
	// ExtractPath unresolved), `sample` stays null and we fall back to an
	// empty vtkPolyData placeholder.
	vtkDataObject* sample = nullptr;
	vtkPartitionedDataSetCollection* input =
		vtkPartitionedDataSetCollection::GetData(inputVector[0]);
	if (input && input->GetDataAssembly())
	{
		int node = input->GetDataAssembly()->GetFirstNodeByPath(ExtractPath.c_str());
		if (node > -1)
		{
			auto indices = input->GetDataAssembly()->GetDataSetIndices(node);
			if (!indices.empty())
			{
				vtkPartitionedDataSet* pds = input->GetPartitionedDataSet(indices[0]);
				if (pds && pds->GetNumberOfPartitions() > 0)
				{
					sample = pds->GetPartitionAsDataObject(0);
				}
			}
		}
	}

	vtkDataObject* currentOutput = outInfo->Get(vtkDataObject::DATA_OBJECT());
	if (sample)
	{
		// Replace only if the type doesn't match — type-exact NewInstance().
		const char* desiredClass = sample->GetClassName();
		if (!currentOutput || strcmp(currentOutput->GetClassName(), desiredClass) != 0)
		{
			vtkSmartPointer<vtkDataObject> newOutput;
			newOutput.TakeReference(sample->NewInstance());
			outInfo->Set(vtkDataObject::DATA_OBJECT(), newOutput);
		}
	}
	else if (!currentOutput)
	{
		// First call with no input data ready — use vtkPolyData placeholder.
		// Will be replaced on the next pass once input is populated.
		vtkSmartPointer<vtkPolyData> placeholder;
		placeholder.TakeReference(vtkPolyData::New());
		outInfo->Set(vtkDataObject::DATA_OBJECT(), placeholder);
	}
	// else: input is transiently empty but a previous output type is known
	// (e.g. vtkExplicitStructuredGrid set by an earlier pass). Keep it —
	// downgrading to vtkPolyData here breaks downstream filters that
	// require the original concrete type (ExplicitStructuredGridCrop, …).
	return 1;
}

//----------------------------------------------------------------------------
// RequestInformation propagates pipeline metadata downstream BEFORE RequestData
// runs. Two things matter:
//   1. The DATA_TYPE_NAME must match the actual partition class so the
//      Streaming executive picks the right pipeline policy. With a generic
//      "vtkDataObject" fallback it defaults to a structured-data policy and
//      throws "No whole extent has been set in the information" on every
//      execution — that error is what makes vtkPolyData/vtkStructuredGrid
//      outputs invisible (initially mistaken for a parent/child visibility
//      cascade).
//   2. For structured types (vtkImageData / vtkRectilinearGrid /
//      vtkStructuredGrid / vtkExplicitStructuredGrid), WHOLE_EXTENT must be
//      set on the output port info or downstream filters/mappers can't
//      schedule data updates correctly.
int vtkEnergisticsExtractor::RequestInformation(
	vtkInformation* /*request*/, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
	vtkPartitionedDataSetCollection* input =
		vtkPartitionedDataSetCollection::GetData(inputVector[0]);
	if (!input)
	{
		return 1;
	}

	vtkDataObject* samplePartition = nullptr;
	if (input->GetDataAssembly())
	{
		int node = input->GetDataAssembly()->GetFirstNodeByPath(ExtractPath.c_str());
		if (node > -1)
		{
			auto indices = input->GetDataAssembly()->GetDataSetIndices(node);
			if (!indices.empty())
			{
				vtkPartitionedDataSet* pds = input->GetPartitionedDataSet(indices[0]);
				if (pds && pds->GetNumberOfPartitions() > 0)
				{
					samplePartition = pds->GetPartitionAsDataObject(0);
				}
			}
		}
	}
	if (!samplePartition)
	{
		// Input data not ready yet (assembly not populated). Leave the
		// output's DATA_TYPE_NAME at its FillOutputPortInformation default;
		// the next RequestInformation pass (after the input's RequestData)
		// will fix it.
		return 1;
	}

	vtkInformation* outInfo = outputVector->GetInformationObject(0);
	outInfo->Set(vtkDataObject::DATA_TYPE_NAME(), samplePartition->GetClassName());

	// WHOLE_EXTENT propagation for structured types.
	int extent[6] = {0, -1, 0, -1, 0, -1};
	bool hasExtent = false;
	if (auto* sg = vtkStructuredGrid::SafeDownCast(samplePartition))
	{
		sg->GetExtent(extent);
		hasExtent = true;
	}
	else if (auto* esg = vtkExplicitStructuredGrid::SafeDownCast(samplePartition))
	{
		esg->GetExtent(extent);
		hasExtent = true;
	}
	else if (auto* img = vtkImageData::SafeDownCast(samplePartition))
	{
		img->GetExtent(extent);
		hasExtent = true;
	}
	else if (auto* rg = vtkRectilinearGrid::SafeDownCast(samplePartition))
	{
		rg->GetExtent(extent);
		hasExtent = true;
	}
	if (hasExtent)
	{
		outInfo->Set(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(), extent, 6);
	}

	return 1;
}

//-----------------------------------------------------------------------------
int vtkEnergisticsExtractor::FillInputPortInformation(int vtkNotUsed(port), vtkInformation* info)
{
	info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkPartitionedDataSetCollection");
	return 1;
}

//------------------------------------------------------------------------------
int vtkEnergisticsExtractor::FillOutputPortInformation(
	int /*port*/, vtkInformation* info)
{
	// Conservative default at pipeline-info time when the input may not be
	// ready. RequestInformation will replace this with the actual partition
	// class as soon as the input data is populated.
	info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkDataObject");
	return 1;
}

//----------------------------------------------------------------------------
int vtkEnergisticsExtractor::RequestData(vtkInformation* vtkNotUsed(request),
	vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
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

	// Resolve the source partition (may be null when ExtractPath does not
	// resolve in the current input assembly: outputs stay empty in that case).
	vtkDataObject* sourcePartition = nullptr;
	if (partitionIndex >= 0)
	{
		vtkPartitionedDataSet* sourcePds = input->GetPartitionedDataSet(partitionIndex);
		if (sourcePds && sourcePds->GetNumberOfPartitions() > 0)
		{
			sourcePartition = sourcePds->GetPartitionAsDataObject(0);
		}
	}

	// Single output: the inner partition exposed in its actual VTK type.
	// FillOutputPortInformation declares the port as the generic vtkDataObject
	// at pipeline-info time. Here we replace the executive-allocated output
	// with one of the source partition's actual class (vtkPolyData /
	// vtkUnstructuredGrid / vtkExplicitStructuredGrid) so ShallowCopy transfers
	// the structural arrays (points, cells, polygons), not just the field
	// data. RequestInformation has already pushed the correct DATA_TYPE_NAME
	// and (for structured types) WHOLE_EXTENT downstream.
	vtkInformation* outInfo = outputVector->GetInformationObject(0);
	vtkDataObject* currentOutput = outInfo->Get(vtkDataObject::DATA_OBJECT());
	if (sourcePartition)
	{
		if (!currentOutput || !currentOutput->IsA(sourcePartition->GetClassName()))
		{
			vtkSmartPointer<vtkDataObject> newOutput;
			newOutput.TakeReference(sourcePartition->NewInstance());
			outInfo->Set(vtkDataObject::DATA_OBJECT(), newOutput);
			currentOutput = newOutput;
		}
		currentOutput->ShallowCopy(sourcePartition);
	}

	return 1;
}
