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

#ifndef vtkEPCViewClone_h
#define vtkEPCViewClone_h

// FESPP
#include "EnergisticsModule.h"

// ParaView
#include <vtkPartitionedDataSetCollectionAlgorithm.h>

/**
 * @brief View-scoped pass-through clone of a vtkEPCCollector.
 *
 * Drop-in placeholder in the SM proxy graph that simply
 * ShallowCopy's its input vtkPartitionedDataSetCollection (data
 * blocks AND assembly) onto its output. Designed to give each
 * per-view sub-pipeline its own "scene root" without duplicating
 * the data — every downstream filter (slice, clip, threshold,
 * etc.) chains on a view-local proxy whose contents track the
 * shared vtkEPCCollector through PV's native pipeline propagation.
 *
 * Memory: ShallowCopy shares buffers, so N clones cost N proxy
 * objects but no extra data arrays.
 *
 * Companion to the trame-side `ViewScene` class — see the python
 * refactor in `fespp-on-trame/doc/REFACTOR_VIEW_SCENES.md`.
 */
class ENERGISTICS_EXPORT vtkEPCViewClone : public vtkPartitionedDataSetCollectionAlgorithm
{
public:
	static vtkEPCViewClone* New();
	vtkTypeMacro(vtkEPCViewClone, vtkPartitionedDataSetCollectionAlgorithm);

protected:
	vtkEPCViewClone();
	~vtkEPCViewClone() override = default;

	int RequestData(vtkInformation*,
		vtkInformationVector**,
		vtkInformationVector*) override;

private:
	vtkEPCViewClone(const vtkEPCViewClone&) = delete;
	void operator=(const vtkEPCViewClone&) = delete;
};

#endif
