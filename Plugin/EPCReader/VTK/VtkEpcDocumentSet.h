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
#ifndef __VtkEpcDocumentSet_h
#define __VtkEpcDocumentSet_h

#include "EPCReaderModule.h"

// include system
#include <string>
#include <unordered_map>
#include <vector>

// include VTK
#include <vtkSmartPointer.h>
#include <vtkMultiBlockDataSet.h>

#include "VtkEpcCommon.h"

class VtkEpcDocument;

/** @brief	The epc document set representation (vtkMultiBlockDataSet)
 */
class EPCREADER_EXPORT VtkEpcDocumentSet
{
public:
	/**
	* Constructor
	*/
	VtkEpcDocumentSet (int idProc=0, int maxProc=0, VtkEpcCommon::modeVtkEpc mode = VtkEpcCommon::modeVtkEpc::Both);

	/**
	* Destructor
	*/
	~VtkEpcDocumentSet();

	/**
	 * load a data object in VTK object
	 *
	 * @param 	uuid	a uuid of epc document to load
	 */
	std::string visualize(const std::string & uuid);

	/**
	 * load all epc documents in VTK object
	 */
	void visualizeFull();

	/**
	 * load all Wellbores for an epc document in VTK object
	 * 
	 * @param 	fileName	epc document fileName
	 */
	void visualizeFullWell(std::string fileName);
	
	/**
	 * unload a data object in VTK object
	 *
	 * @param 	uuid	a uuid of epc document to load
	 */
	void unvisualize(const std::string & uuid);

	/**
	 * unload all Wellbores for an epc document in VTK object
	 * 
	 * @param 	fileName	epc document fileName
	 */	
	void unvisualizeFullWell(std::string fileName);

	/**
	* get type for a uuid
	*
	* @param 	uuid	
	*/
	VtkEpcCommon::Resqml2Type getType(std::string uuid);

	/**
	* get all info (VtkEpcCommon) for a uuid
	*
	* @param 	uuid	
	*/
	VtkEpcCommon getInfoUuid(std::string);

	/**
	 * Change orientation state for all Welbore markers
	 *
	 * @param 	orientation	false - no orientation for all welbore markers
	 * 						true - dips/azimuth orientation for all wellbore markers
	 */
	void toggleMarkerOrientation(const bool orientation);

	/**
	 * Change for all markers size
	 *
	 * @param 	size	size in pixel for all markers
	 */
	void setMarkerSize(int size);

	/**
	* return the vtkMultiBlockDataSet of epc document set.
	*/
	vtkSmartPointer<vtkMultiBlockDataSet> getVisualization() const;

	/**
	* return all VtkEpcCommons for calculate TreeView representation
	* notes: when no time series link => timeIndex = -1 
	*/
	std::vector<VtkEpcCommon const *> getAllVtkEpcCommons() const;

	/**
	* add a epc document to the set
	*
	* @param filename	epc document filename's
	*/
	std::string addEpcDocument(const std::string & fileName);

	/**
	* return all VtkEpcCommons for calculate TreeView representation
	* notes: when no time series link => timeIndex = -1 
	*/
	const std::vector<VtkEpcDocument*>& getAllVtkEpcDocuments() const { return vtkEpcList; }

	/**
	* return VtkEpcDocument which contain uuid
	*
	* @param uuid	uuid to search
	*/	
	VtkEpcDocument* getVtkEpcDocument(const std::string& uuid);

	/**
	* Get the datatype of a particular dataobject according to its uuid
	*
	* @param uuid	The UUID of the dataobject we want to know it datatype
	* @return	An unknown datatype is returned if the dataobject is not contained on this set
	*/
	VtkEpcCommon::Resqml2Type getTypeInEpcDocument(const std::string & uuid);

private:

	// The list of EPC documents contained into this set
	std::vector<VtkEpcDocument*> vtkEpcList;
	// A list of uuid which cannot be visualized for some reasons (generally FESAPI exception)
	std::vector<std::string> badUuid;
	// link from a dataobject uuid to the VtkEpcDocument it belongs to
	std::unordered_map<std::string, VtkEpcDocument*> uuidToVtkEpc;

	vtkSmartPointer<vtkMultiBlockDataSet> vtkOutput;

	int procRank;
	int nbProc;

	bool treeViewMode;
	bool representationMode;

};
#endif
