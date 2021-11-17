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
#ifndef __ResqmlDataRepositoryToVtkPartitionedDataSetCollection_h
#define __ResqmlDataRepositoryToVtkPartitionedDataSetCollection_h

// include system
#include <string>
#include <map>
#include <vector>

#include <vtkSmartPointer.h>
#include <vtkPartitionedDataSetCollection.h>
#include <vtkMultiProcessController.h>

namespace common
{
    class DataObjectRepository;
}

namespace RESQML2_NS
{
    class AbstractRepresentation;
}

/** 
 * @brief	class description.
 */
class ResqmlDataRepositoryToVtkPartitionedDataSetCollection : public vtkPartitionedDataSetCollection
{
public:
    ResqmlDataRepositoryToVtkPartitionedDataSetCollection(vtkMultiProcessController *);
    ~ResqmlDataRepositoryToVtkPartitionedDataSetCollection() final;
    
	void addFile(const char* file, int status);

    // Wellbore Options
    void setMarkerOrientation(bool orientation);
	void setMarkerSize(int size);

    vtkPartitionedDataSetCollection *getVtkPartionedDatasSetCollection() { return output; }

    

protected:
private:
    ResqmlDataRepositoryToVtkPartitionedDataSetCollection(const ResqmlDataRepositoryToVtkPartitionedDataSetCollection&);

    std::string searchFaultPolylines(const std::string &fileName);
    std::string searchHorizonPolylines(const std::string &fileName);
    std::string searchUnstructuredGrid(const std::string &fileName);
    std::string searchTriangulated(const std::string &fileName);
    std::string searchGrid2d(const std::string &fileName);
    std::string searchIjkGrid(const std::string &fileName);
    std::string searchWellboreTrajectory(const std::string &fileName);
    std::string searchSubRepresentation(const std::string &fileName);
    std::string searchTimeSeries(const std::string &fileName);
    void searchRepresentations(std::vector<RESQML2_NS::AbstractRepresentation *> representation_set);

	char* FileName; // pipename

    bool markerOrientation; 
    int markerSize;

 	vtkMultiProcessController *controler;
    
    common::DataObjectRepository *repository;

    vtkSmartPointer<vtkPartitionedDataSetCollection> output;

    std::map<int, std::string> nodeId_to_uuid; // index of VtkDataAssembly to Resqml uuid
};
#endif