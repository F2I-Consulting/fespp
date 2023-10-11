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
#include "WitsmlWellboreCompletionToVtkPartitionedDataSet.h"

#include <vtkInformation.h>

#include <fesapi/witsml2_1/WellboreCompletion.h>
#include <fesapi/resqml2/MdDatum.h>
#include <fesapi/resqml2/WellboreFeature.h>
#include <fesapi/resqml2/WellboreInterpretation.h>
#include <fesapi/resqml2/WellboreTrajectoryRepresentation.h>

//----------------------------------------------------------------------------
WitsmlWellboreCompletionToVtkPartitionedDataSet::WitsmlWellboreCompletionToVtkPartitionedDataSet(WITSML2_1_NS::WellboreCompletion *completion, int proc_number, int max_proc)
	: CommonAbstractObjectToVtkPartitionedDataSet(completion,
											   proc_number,
											   max_proc)
{
	for (auto* interpretation : completion->getWellbore()->getResqmlWellboreFeature(0)->getInterpretationSet()) {
		if (dynamic_cast<RESQML2_NS::WellboreInterpretation*>(interpretation) != nullptr) {
			auto *wellbore_interpretation = static_cast<resqml2::WellboreInterpretation*>(interpretation);
			this->wellboreTrajectory = wellbore_interpretation->getWellboreTrajectoryRepresentation(0);
			continue;
		}
	}

	this->vtkData = vtkSmartPointer<vtkPartitionedDataSet>::New();
	this->vtkData->Modified();
}

//----------------------------------------------------------------------------
WITSML2_1_NS::WellboreCompletion const* WitsmlWellboreCompletionToVtkPartitionedDataSet::getResqmlData() const
{
	return static_cast<WITSML2_1_NS::WellboreCompletion const*>(resqmlData);
}

//----------------------------------------------------------------------------
resqml2::WellboreTrajectoryRepresentation const* WitsmlWellboreCompletionToVtkPartitionedDataSet::getWellboreTrajectory() const
{
	return this->wellboreTrajectory;
}

//----------------------------------------------------------------------------
void WitsmlWellboreCompletionToVtkPartitionedDataSet::loadVtkObject()
{
	for (int idx = 0; idx < this->connections.size(); ++idx)
	{
		this->vtkData->SetPartition(idx, this->connections[idx]);
		this->vtkData->GetMetaData(idx)->Set(vtkCompositeDataSet::NAME(), this->connections[idx]->getType().c_str());
	}
}

void WitsmlWellboreCompletionToVtkPartitionedDataSet::addConnection(const std::string & connectionType)
{
    vtkOutputWindowDisplayErrorText("addConnection");
    if (connectionType == "Perforation") 
    {
        // Iterate over the perforations.
        for (uint64_t perforationIndex = 0; perforationIndex < this->getResqmlData()->getConnectionCount(WITSML2_1_NS::WellboreCompletion::WellReservoirConnectionType::PERFORATION); ++perforationIndex)
        {
            std::string perforation_name = "Perfo";
            auto& extraMetadatas = this->getResqmlData()->getConnectionExtraMetadata(WITSML2_1_NS::WellboreCompletion::WellReservoirConnectionType::PERFORATION, perforationIndex, "Petrel:Name0");
            // arbitrarily select the first event name as perforation name
            if (extraMetadatas.size() > 0)
            {
                perforation_name += "_" + extraMetadatas[0];
            }
            else
            {
                extraMetadatas = this->getResqmlData()->getConnectionExtraMetadata(WITSML2_1_NS::WellboreCompletion::WellReservoirConnectionType::PERFORATION, perforationIndex, "Sismage-CIG:Name");
                if (extraMetadatas.size() > 0)
                {
                    perforation_name += "_" + extraMetadatas[0];
                }
                else
                {
                    perforation_name += "_" + this->getResqmlData()->getConnectionUid(WITSML2_1_NS::WellboreCompletion::WellReservoirConnectionType::PERFORATION, perforationIndex);
                }
            }
            // this->getResqmlData()->getConnectionUid(WITSML2_1_NS::WellboreCompletion::WellReservoirConnectionType::PERFORATION, perforationIndex)
        }
        //connections.push_back(connection);
    }
}

std::vector<WitsmlWellboreCompletionConnectionToVtkDataSet*> WitsmlWellboreCompletionToVtkPartitionedDataSet::getConnections()
{
	return connections;
}


