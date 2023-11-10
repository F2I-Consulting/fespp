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

#include <vtkFieldData.h>

#include <fesapi/witsml2_1/WellboreCompletion.h>
#include <fesapi/resqml2/MdDatum.h>
#include <fesapi/resqml2/WellboreFeature.h>
#include <fesapi/resqml2/WellboreInterpretation.h>
#include <fesapi/resqml2/WellboreTrajectoryRepresentation.h>

//----------------------------------------------------------------------------
WitsmlWellboreCompletionToVtkPartitionedDataSet::WitsmlWellboreCompletionToVtkPartitionedDataSet(const WITSML2_1_NS::WellboreCompletion *completion, int proc_number, int max_proc)
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
const WITSML2_1_NS::WellboreCompletion const* WitsmlWellboreCompletionToVtkPartitionedDataSet::getResqmlData() const
{
	return static_cast<const WITSML2_1_NS::WellboreCompletion const*>(resqmlData);
}

//----------------------------------------------------------------------------
resqml2::WellboreTrajectoryRepresentation const* WitsmlWellboreCompletionToVtkPartitionedDataSet::getWellboreTrajectory() const
{
	return this->wellboreTrajectory;
}

//----------------------------------------------------------------------------
void WitsmlWellboreCompletionToVtkPartitionedDataSet::loadVtkObject()
{
	for (int idx = 0; idx < this->perforations.size(); ++idx)
	{
		this->vtkData->SetPartition(idx, this->perforations[idx]->getOutput());
		this->vtkData->GetMetaData(idx)->Set(vtkCompositeDataSet::NAME(), this->perforations[idx]->getTitle().c_str());

		// Récupérer l'array des données de points
		vtkDataSet* partition = this->vtkData->GetPartition(idx);
		if (partition) {
			vtkFieldData* fieldData = partition->GetFieldData();
			if (fieldData) {
				vtkDataArray* dataArray = fieldData->GetArray("Skin");
				if (dataArray) {
					std::cout << "L'array avec le nom 'Skin' a été trouvé dans les FieldData de la partition " << idx << "." << std::endl;
				}
			}
		}

	}
	
}

void WitsmlWellboreCompletionToVtkPartitionedDataSet::addPerforation(const std::string& connectionuid, const std::string& name)
{
	this->perforations.push_back(new WitsmlWellboreCompletionPerforationToVtkPolyData(this->getWellboreTrajectory(), this->getResqmlData(), connectionuid, name));
	this->loadVtkObject();
}

std::vector<WitsmlWellboreCompletionPerforationToVtkPolyData*> WitsmlWellboreCompletionToVtkPartitionedDataSet::getPerforations()
{
	return perforations;
}


