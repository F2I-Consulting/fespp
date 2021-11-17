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
#include "ResqmlMapping/ResqmlDataRepositoryToVtkPartitionedDataSetCollection.h"

// VTK includes
#include <vtkPartionedDatasSetCollection.h>
#include <vtkDataAssembly.h>

// FESAPI includes
#include <Grid2dRepresentation.h>
#include <AbstractIjkGridRepresentation.h>
#include <PolylineSetRepresentation.h>
#include <SubRepresentation.h>
#include <TriangulatedSetRepresentation.h>
#include <UnstructuredGridRepresentation.h>
#include <WellboreMarkerFrameRepresentation.h>
#include <wellboreTrajectoryRepresentation.h>

ResqmlDataRepositoryToVtkPartitionedDataSetCollection::ResqmlDataRepositoryToVtkPartitionedDataSetCollection(vtkMultiProcessControler *controller) : 
        output(vtkSmartPointer<vtkPartionedDatasSetCollection>::New()),
        repository(new common::DataObjectRepository()),
        controller(controler)
{
    SetNumberOfInputPorts(0);
	SetNumberOfOutputPorts(1);

    output->GetDataAssembly()->SetRootNodeName('data');
}

ResqmlDataRepositoryToVtkPartitionedDataSetCollection::~ResqmlDataRepositoryToVtkPartitionedDataSetCollection()
{
}

void ResqmlDataRepositoryToVtkPartitionedDataSetCollection::addFile(std::string fileName, int status)
{
    /******* TODO ********/ 
    // status = 0 => remove file of repository


    std::string message = '';

    COMMON_NS::EpcDocument pck(fileName);
    const std::string resqmlResult = pck.deserializeInto(*repository);
    pck.close();
    if (!resqmlResult.empty())
    {
        message = message + resqmlResult;
    }

    // create vtkDataAssembly: create treeView in property panel

    // get polylines
    message = message + searchFaultPolylines(fileName);
    message = message + searchHorizonPolylines(fileName);
    // get unstructuredGrid
    message = message + searchUnstructuredGrid(fileName);
    // get triangulated
    message = message + searchTriangulated(fileName);
    // get grid2D
    message = message + searchGrid2d(fileName);
    // get ijkGrid
    message = message + searchIjkGrid(fileName);
    // get WellboreTrajectory
    message = message + searchWellboreTrajectory(fileName);
    // get subRepresentation
    message = message + searchSubRepresentation(fileName);
    // get TimeSeries
    message = message + searchTimeSeries(fileName);

	if (!message.empty()) 
    {
        vtkErrorMacro(<< message.c_str());
    }
}

std::string ResqmlDataRepositoryToVtkPartitionedDataSetCollection::searchFaultPolylines(const std::string &fileName)
{
    std::vector<RESQML2_NS::PolylineSetRepresentation *> polylines;
    try
    {
        polylines = repository->getFaultPolylineSetRepSet();
        searchRepresentations(polylines);
    }
    catch (const std::exception &e)
    {
        return "EXCEPTION in fesapi when calling getFaultPolylineSetRepSet with file: " + fileName + " : " + e.what();
    }
    return "";
}

std::string ResqmlDataRepositoryToVtkPartitionedDataSetCollection::searchHorizonPolylines(const std::string &fileName)
{
    std::vector<RESQML2_NS::PolylineSetRepresentation *> polylines;
    try
    {
        polylines = repository->getHorizonPolylineSetRepSet();
        searchRepresentations(polylines);
    }
    catch (const std::exception &e)
    {
        return "EXCEPTION in fesapi when calling getHorizonPolylineSetRepSet with file: " + fileName + " : " + e.what();
    }
    return "";
}

std::string ResqmlDataRepositoryToVtkPartitionedDataSetCollection::searchUnstructuredGrid(const std::string &fileName)
{
    std::vector<RESQML2_NS::UnstructuredGridRepresentation *> unstructuredGrid_set;
    try
    {
        unstructuredGrid_set = repository->getUnstructuredGridRepresentationSet();
        searchRepresentations(unstructuredGrid_set);
    }
    catch (const std::exception &e)
    {
        return "EXCEPTION in fesapi when calling getHorizonPolylineSetRepSet with file: " + fileName + " : " + e.what();
    }
    return "";
}

std::string ResqmlDataRepositoryToVtkPartitionedDataSetCollection::searchTriangulated(const std::string &fileName)
{
    std::vector<RESQML2_NS::TriangulatedSetRepresentation *> triangulated_set;
    try
    {
        triangulated_set = repository->getAllTriangulatedSetRepSet();
        searchRepresentations(triangulated_set);
    }
    catch (const std::exception &e)
    {
        return "EXCEPTION in fesapi when calling getAllTriangulatedSetRepSet with file: " + fileName + " : " + e.what();
    }
    return "";
}

std::string ResqmlDataRepositoryToVtkPartitionedDataSetCollection::searchGrid2d(const std::string &fileName)
{
    std::vector<RESQML2_NS::Grid2dRepresentation *> grid2D_set;
    try
    {
        grid2D_set = repository->getHorizonGrid2dRepSet();
        searchRepresentations(grid2D_set);
    }
    catch (const std::exception &e)
    {
        return "EXCEPTION in fesapi when calling getHorizonGrid2dRepSet with file: " + fileName + " : " + e.what();
    }
    return "";
}

std::string ResqmlDataRepositoryToVtkPartitionedDataSetCollection::searchIjkGrid(const std::string &fileName)
{
    std::vector<RESQML2_NS::AbstractIjkGridRepresentation *> ijkGrid_set;
    try
    {
        ijkGrid_set = repository->getIjkGridRepresentationSet();
        searchRepresentations(ijkGrid_set);
    }
    catch (const std::exception &e)
    {
        return "EXCEPTION in fesapi when calling getIjkGridRepresentationSet with file: " + fileName + " : " + e.what();
    }
    return "";
}

std::string ResqmlDataRepositoryToVtkPartitionedDataSetCollection::searchSubRepresentation(const std::string &fileName)
{
    std::vector<RESQML2_NS::SubRepresentation *> subRepresentationSet;
    try
    {
        subRepresentation_set = repository->getSubRepresentationSet();
        searchRepresentations(subRepresentation_set);
    }
    catch (const std::exception &e)
    {
        return "EXCEPTION in fesapi when calling getSubRepresentationSet with file: " + fileName + " : " + e.what() + ".\n";
    }
    return "";
}

void ResqmlDataRepositoryToVtkPartitionedDataSetCollection::searchRepresentations(std::vector<RESQML2_NS::AbstractRepresentation *> representation_set)
{
    vtkDataAssembly *data_assembly = output->GetDataAssembly();

    for (auto &representation : representation_set)
    {
        int idNode = 0; // 0 is root's id

        if (representation->isPartial())
        {
            // check if it has already been added
            boolean uuid_exist = false;
            for (std::map<int, std::string>::iterator it = nodeId_to_uuid.begin(); it != nodeId_to_uuid.end(); ++it)
            {
                if (representation->getUuid() == it->second)
                {
                    uuid_exist = true;
                    continue;
                }
            }

            // not exist => not loaded
            if (!uuid_exist)
            {
                epc_error = epc_error + " Partial UUID: (" + representation->getUuid() + ") is not loaded \n";
                continue;
            } /******* TODO ********/ // exist but not the same type ?
        }
        else
        {
            const auto interpretation = representation->getInterpretation();
            if (interpretation != nullptr)
            {
                idNode = data_assembly->AddNode(interpretation->getTitle().c_str());
                nodeId_to_uuid[idNode] = interpretation->getUuid();
            }
            idNode = data_assembly->AddNode(representation->getTitle().c_str(), idNode);
            nodeId_to_uuid[idNode] = representation->getUuid();
        }

        //property
        for (const auto property : representation->getValuesPropertySet())
        {
            int property_idNode = data_assembly->AddNode(property->getTitle().c_str(), idNode);
            nodeId_to_uuid[property_idNode] = property->getUuid();
        }
    }
}

void ResqmlDataRepositoryToVtkPartitionedDataSetCollection::searchWellboreTrajectory(const std::string &fileName)
{
    vtkDataAssembly *data_assembly = output->GetDataAssembly();

    for (const auto wellboreTrajectory : repository->getWellboreTrajectoryRepresentationSet())
    {
        if (representation->isPartial())
        {
            // check if it has already been added
            boolean uuid_exist = false;
            for (std::map<int, std::string>::iterator it = nodeId_to_uuid.begin(); it != nodeId_to_uuid.end(); ++it)
            {
                if (representation->getUuid() == it->second)
                {
                    uuid_exist = true;
                    continue;
                }
            }

            // not exist => not loaded
            if (!uuid_exist)
            {
                epc_error = epc_error + " Partial UUID: (" + representation->getUuid() + ") is not loaded \n";
                continue;
            } /******* TODO ********/ // exist but not the same type ?
        }
        else
        {
            const auto interpretation = representation->getInterpretation();
            if (interpretation != nullptr)
            {
                idNode = data_assembly->AddNode(interpretation->getTitle().c_str());
                nodeId_to_uuid[idNode] = interpretation->getUuid();
            }
            idNode = data_assembly->AddNode(representation->getTitle().c_str(), idNode);
            nodeId_to_uuid[idNode] = representation->getUuid();
        }

        //wellboreFrame
        for (const auto wellboreFrame : wellboreTrajectory->getWellboreFrameRepresentationSet())
        {
            const auto wellboreMarkerFrame = dynamic_cast<RESQML2_NS::WellboreMarkerFrameRepresentation *>(wellboreFrame);
            if (wellboreMarkerFrame == nullptr)
            { // WellboreFrame
                int frame_idNode = data_assembly->AddNode(wellboreFrame->getTitle().c_str(), idNode);
                nodeId_to_uuid[frame_idNode] = wellboreFrame->getUuid();
                for (const auto property : wellboreFrame->getValuesPropertySet())
                {
                    int property_idNode = data_assembly->AddNode(property->getTitle().c_str(), frame_idNode);
                    nodeId_to_uuid[property_idNode] = property->getUuid();
                }
            }
            else
            { // WellboreMarkerFrame
                int markerFrame_idNode = data_assembly->AddNode(wellboreFrame->getTitle().c_str(), idNode);
                nodeId_to_uuid[markerFrame_idNode] = wellboreFrame->getUuid();
                for (const auto wellboreMarker : wellboreMarkerFrame->getWellboreMarkerSet())
                {
                    int markerFrame_idNode = data_assembly->AddNode(wellboreFrame->getTitle().c_str(), idNode);
                    nodeId_to_uuid[markerFrame_idNode] = wellboreFrame->getUuid();
                }
            }
        }
    }
    return "";
}

void VtkEpcDocument::searchTimeSeries(const std::string &fileName)
{
    std::string return_message = "";
    std::vector<EML2_NS::TimeSeries *> timeSeries;
    try
    {
        timeSeries = repository->getTimeSeriesSet();
    }
    catch (const std::exception &e)
    {
        return_message = return_message + "EXCEPTION in fesapi when calling getTimeSeriesSet with file: " + fileName + " : " + e.what();
    }

    /******* TODO ********/
}