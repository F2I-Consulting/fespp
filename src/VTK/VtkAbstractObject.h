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
#ifndef __VtkAbstractObject_h
#define __VtkAbstractObject_h

// include system
#include <string>

#include "VtkEpcCommon.h"

class VtkAbstractObject
{
public:
	VtkAbstractObject (const std::string & fileName, const std::string & name="", const std::string & uuid="", const std::string & uuidParent="", int idProc=0, int maxProc=0);

	/**
	* Destructor
	*/
	virtual ~VtkAbstractObject();

	const std::string& getFileName() const { return fileName; }
	const std::string& getName() const { return name; }
	const std::string& getUuid() const { return uuid; }
	const std::string& getParent() const { return uuidParent; }
	
	int getIdProc() const;
	int getMaxProc() const;

	void setFileName(const std::string &);
	void setName(const std::string &);
	void setUuid(const std::string &);
	void setParent(const std::string &);

	/**
	* load & display representation uuid's
	*/
	virtual void visualize(const std::string & uuid) = 0;
	
	/**
	* create vtk resqml2 element
	*/
	virtual void createTreeVtk(const std::string & uuid, const std::string & parent, const std::string & name, VtkEpcCommon::Resqml2Type resqmlType) = 0;

	/**
	* remove representation uuid's
	*/
	virtual void remove(const std::string & uuid) = 0;

	/**
	*/
	virtual long getAttachmentPropertyCount(const std::string & uuid, VtkEpcCommon::FesppAttachmentProperty propertyUnit) = 0;

private:
	std::string fileName;
	std::string name;
	std::string uuid;
	std::string uuidParent;

	int idProc;
	int maxProc;
};
#endif
