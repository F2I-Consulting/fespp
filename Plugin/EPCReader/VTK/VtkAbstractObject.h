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

/** @brief	An abstract data object.
 *  - tree structure info
 *  - multi-processor info
 */
class VtkAbstractObject
{
public:
	VtkAbstractObject (const std::string & fileName, const std::string & name="", const std::string & uuid="", const std::string & uuidParent="", int idProc=0, int maxProc=0);

	/**
	* Destructor
	*/
	virtual ~VtkAbstractObject() = default;

	/**
	 * Gets the epc fileName of this data object.
	 * 
	 * @returns	The epc fileName of this data object.
	 */
	const std::string& getFileName() const { return fileName; }

	/**
	 * Gets the name (i.e. the title) of this data object.
	 *
	 * @returns	The namz of this data object.
	 */
	const std::string& getName() const { return name; }

	/**
	 * Gets the UUID of this data object. 
	 *
	 * @returns	The UUID of this data object.
	 */
	const std::string& getUuid() const { return uuid; }

	/**
	 * Gets the UUID of data object parent. 
	 *
	 * @returns	The UUID of data object parent.
	 */	
	const std::string& getParent() const { return uuidParent; }
	
	/**
	 * Gets the id of data object processor. 
	 *
	 * @returns	The id of data object processor.
	 */		
	int getIdProc() const;

	/**
	 * Gets the id of data object processor. 
	 *
	 * @returns	The id of data object processor.
	 */	
	int getMaxProc() const;

	/**
	 * Set a epc fileName of this data object.
	 *
	 * @param 	epcFileName	The epc fileName.
	 */
	void setFileName(const std::string &);

	/**
	 * Set a name (i.e. a title) of this data object.
	 *
	 * @param 	name	The name.
	 */
	void setName(const std::string &);

	/**
	 * Set a uuid of this data object.
	 *
	 * @param 	name	The name.
	 */
	void setUuid(const std::string &);
		
	/**
	 * Set a uuid parent of this data object.
	 *
	 * @param 	name	The name.
	 */
	void setParent(const std::string &);

	/**
	 * data object load in VTKMultiBlockDataSet
	 * if uuid param isn't data object uuid then load child of data object uuid.
	 *
	 * @param 	uuid	uuid to load.
	 */
	virtual void visualize(const std::string & uuid) = 0;

	/**
	 * data object unload in VTKMultiBlockDataSet
	 * if uuid param isn't data object uuid then unload child of data object uuid.
	 *
	 * @param 	uuid	uuid to load.
	 */
	virtual void remove(const std::string & uuid) = 0;

	/**
	 * data object unload in VTKMultiBlockDataSet
	 * if uuid param isn't data object uuid then unload child of data object uuid.
	 *
	 * @param 	uuid	uuid of data object property.
	 * @param 	propertyUnit	type of property unit (cells/points).
	 * 
	 * @returns	The number of values.
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
