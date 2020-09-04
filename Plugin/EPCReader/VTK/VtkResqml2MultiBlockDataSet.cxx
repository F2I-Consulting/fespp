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
#include "VtkResqml2MultiBlockDataSet.h"

//----------------------------------------------------------------------------
VtkResqml2MultiBlockDataSet::VtkResqml2MultiBlockDataSet(const std::string & fileName, const std::string & name, const std::string & uuid, const std::string & uuidParent, int idProc, int maxProc):
	VtkAbstractObject(fileName, name, uuid, uuidParent, idProc, maxProc)
{
	vtkOutput = vtkSmartPointer<vtkMultiBlockDataSet>::New();
}

//----------------------------------------------------------------------------
vtkSmartPointer<vtkMultiBlockDataSet> VtkResqml2MultiBlockDataSet::getOutput() const
{
	return vtkOutput;
}

//----------------------------------------------------------------------------
void VtkResqml2MultiBlockDataSet::detach()
{
	while (vtkOutput->GetNumberOfBlocks() != 0) {
		vtkOutput->RemoveBlock(0);
	}
}
