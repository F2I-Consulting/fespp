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
#ifndef __WitsmlWellboreCompletionConnectionToVtkDataSet_H_
#define __WitsmlWellboreCompletionConnectionToVtkDataSet_H_

#include <vtkDataSet.h>

namespace WITSML2_1_NS
{
	class WellboreCompletion;
}

class WitsmlWellboreCompletionConnectionToVtkDataSet : public vtkDataSet
{
public:
	static WitsmlWellboreCompletionConnectionToVtkDataSet* New();

	virtual double getDepth() = 0;
	virtual std::string getType() = 0;

	std::string getType() const;

protected:
	WitsmlWellboreCompletionConnectionToVtkDataSet();
	virtual ~WitsmlWellboreCompletionConnectionToVtkDataSet();

	std::string type;

};
#endif
