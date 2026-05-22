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

#ifndef vtkEPCCollectorClone_h
#define vtkEPCCollectorClone_h

// FESPP
#include "EnergisticsModule.h"

// ParaView
#include <vtkPartitionedDataSetCollectionAlgorithm.h>

/**
 * @brief Pass-through ShallowCopy of a vtkEPCCollector output.
 *
 * Drop-in placeholder in the SM proxy graph that simply
 * ShallowCopy's its input vtkPartitionedDataSetCollection (data
 * blocks AND assembly) onto its output. Plugin-agnostic — it knows
 * nothing about "views"; downstream callers (e.g. fespp_on_trame's
 * per-view scenes) chain whatever sub-pipeline they need on this
 * clone, and the data tracks the upstream EPCCollector through PV's
 * native pipeline propagation.
 *
 * Memory: ShallowCopy shares buffers, so N clones cost N proxy
 * objects but no extra data arrays.
 *
 * Companion to the trame-side `ViewScene` class — see the python
 * refactor in `fespp-on-trame/doc/REFACTOR_VIEW_SCENES.md`.
 */
class ENERGISTICS_EXPORT vtkEPCCollectorClone : public vtkPartitionedDataSetCollectionAlgorithm
{
public:
	static vtkEPCCollectorClone* New();
	vtkTypeMacro(vtkEPCCollectorClone, vtkPartitionedDataSetCollectionAlgorithm);

protected:
	vtkEPCCollectorClone();
	~vtkEPCCollectorClone() override = default;

	int RequestData(vtkInformation*,
		vtkInformationVector**,
		vtkInformationVector*) override;

private:
	vtkEPCCollectorClone(const vtkEPCCollectorClone&) = delete;
	void operator=(const vtkEPCCollectorClone&) = delete;
};

#endif
