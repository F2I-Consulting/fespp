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

#ifndef vtkEnergisticsExtractor_h
#define vtkEnergisticsExtractor_h

// FESPP
#include "EnergisticsModule.h"

// System
#include <set>

// ParaView
#include <vtkAlgorithm.h>
#include <vtkNew.h>
#include <vtkDataObject.h>

class vtkDataAssembly;
class vtkIdList;

class ENERGISTICS_EXPORT vtkEnergisticsExtractor : public vtkAlgorithm
{
public:
	static vtkEnergisticsExtractor* New();
	vtkTypeMacro(vtkEnergisticsExtractor, vtkAlgorithm);

	vtkSetMacro(ExtractPath, std::string);
	vtkGetMacro(ExtractPath, std::string);

protected:
	vtkEnergisticsExtractor();
	~vtkEnergisticsExtractor() override;

	int FillInputPortInformation(int,
		vtkInformation*);

	int FillOutputPortInformation(int,
		vtkInformation*);

	int RequestDataObject(vtkInformation*,
		vtkInformationVector**,
		vtkInformationVector*);

	int RequestInformation(vtkInformation*,
		vtkInformationVector**,
		vtkInformationVector*);

	int RequestData(vtkInformation*,
		vtkInformationVector**,
		vtkInformationVector*);

	int ProcessRequest(vtkInformation*,
		vtkInformationVector**,
		vtkInformationVector*) override;

	std::string ExtractPath;
};

#endif
