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
#include "VtkAbstractObject.h"

//----------------------------------------------------------------------------
VtkAbstractObject::VtkAbstractObject( const std::string & fileName, const std::string & name, const std::string & uuid, const std::string & uuidParent, int idProc, int maxProc):
	fileName(fileName), name(name), uuid(uuid), uuidParent(uuidParent), idProc(idProc), maxProc(maxProc)
{
}

//----------------------------------------------------------------------------
VtkAbstractObject::~VtkAbstractObject()
{
	fileName = "";
	name = "";
	uuid = "";
	uuidParent = "";

	idProc = 0;
	maxProc = 0;
}

//----------------------------------------------------------------------------
int VtkAbstractObject::getIdProc() const
{
	return idProc;
}

//----------------------------------------------------------------------------
int VtkAbstractObject::getMaxProc() const
{
	return maxProc == 0 ? 1 : maxProc;
}

//----------------------------------------------------------------------------
void VtkAbstractObject::setFileName(const std::string & newFileName) 
{
	fileName = newFileName;
}

//----------------------------------------------------------------------------
void VtkAbstractObject::setName(const std::string & newName) 
{
	name = newName;
}

//----------------------------------------------------------------------------
void VtkAbstractObject::setUuid(const std::string & newUuid) 
{
	uuid = newUuid;
}

//----------------------------------------------------------------------------
void VtkAbstractObject::setParent(const std::string & newUuidParent)
{
	uuidParent = newUuidParent;
}
