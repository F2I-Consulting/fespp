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
#include "ResqmlDataRepositoryToVtkPartitionedDataSetCollection.h"

#include <algorithm>
#include <vector>
#include <set>
#include <list>

// VTK includes
#include <vtkPartitionedDataSetCollection.h>
#include <vtkPartitionedDataSet.h>
#include <vtkInformation.h>
#include <vtkDataAssembly.h>
#include <vtkDataArraySelection.h>

// FESAPI includes
#include <fesapi/eml2/TimeSeries.h>
#include <fesapi/resqml2/Grid2dRepresentation.h>
#include <fesapi/resqml2/AbstractIjkGridRepresentation.h>
#include <fesapi/resqml2/PolylineSetRepresentation.h>
#include <fesapi/resqml2/SubRepresentation.h>
#include <fesapi/resqml2/TriangulatedSetRepresentation.h>
#include <fesapi/resqml2/UnstructuredGridRepresentation.h>
#include <fesapi/resqml2/WellboreMarkerFrameRepresentation.h>
#include <fesapi/resqml2/WellboreMarker.h>
#include <fesapi/resqml2/WellboreTrajectoryRepresentation.h>
#include <fesapi/resqml2/AbstractFeatureInterpretation.h>
#include <fesapi/resqml2/ContinuousProperty.h>
#include <fesapi/resqml2/DiscreteProperty.h>
#include <fesapi/common/EpcDocument.h>
#include <fesapi/common/AbstractObject.h>

#include "ResqmlAbstractRepresentationToVtkDataset.h"
#include "ResqmlIjkGridToVtkUnstructuredGrid.h"
#include "ResqmlGrid2dToVtkPolyData.h"
#include "ResqmlPolylineToVtkPolyData.h"
#include "ResqmlTriangulatedSetToVtkPartitionedDataSet.h"
#include "ResqmlUnstructuredGridToVtkUnstructuredGrid.h"
#include "ResqmlWellboreTrajectoryToVtkPolyData.h"
#include "ResqmlWellboreMarkerFrameToVtkPartitionedDataSet.h"
#include "ResqmlWellboreFrameToVtkPartitionedDataSet.h"

ResqmlDataRepositoryToVtkPartitionedDataSetCollection::ResqmlDataRepositoryToVtkPartitionedDataSetCollection() : markerOrientation(false),
                                                                                                                 markerSize(10),
                                                                                                                 repository(new common::DataObjectRepository()),
                                                                                                                 output(vtkSmartPointer<vtkPartitionedDataSetCollection>::New()),
                                                                                                                 nodeId_to_uuid(),
                                                                                                                 nodeId_to_EntityType(),
                                                                                                                 nodeId_to_resqml(),
                                                                                                                 current_selection(),
                                                                                                                 old_selection()
{
    auto assembly = vtkSmartPointer<vtkDataAssembly>::New();
    assembly->SetRootNodeName("data");
    output->SetDataAssembly(assembly);
    times_step.clear();

    nodeId_to_EntityType[0] = ResqmlDataRepositoryToVtkPartitionedDataSetCollection::EntityType::INTERPRETATION;
}

ResqmlDataRepositoryToVtkPartitionedDataSetCollection::~ResqmlDataRepositoryToVtkPartitionedDataSetCollection()
{
    delete repository;
    for (const auto &keyVal : nodeId_to_resqml)
    {
        delete keyVal.second;
    }
}

//----------------------------------------------------------------------------
std::string ResqmlDataRepositoryToVtkPartitionedDataSetCollection::addFile(const char *fileName)
{
    COMMON_NS::EpcDocument pck(fileName);
    const std::string resqmlResult = pck.deserializeInto(*repository);
    pck.close();

    std::string message;
    if (!resqmlResult.empty())
    {
        message += resqmlResult;
    }

    // create vtkDataAssembly: create treeView in property panel
    // get grid2D
    message += searchGrid2d(fileName);
    // get ijkGrid
    message += searchIjkGrid(fileName);
    // get polylines
    message += searchFaultPolylines(fileName);
    message += searchHorizonPolylines(fileName);
    // get triangulated
    message += searchFaultTriangulated(fileName);
    message += searchHorizonTriangulated(fileName);
    // get unstructuredGrid
    message += searchUnstructuredGrid(fileName);
    // get WellboreTrajectory
    message += searchWellboreTrajectory(fileName);

    // get subRepresentation
    message += searchSubRepresentation(fileName);
    // get TimeSeries
    message += searchTimeSeries(fileName);

    return message;
}

std::string ResqmlDataRepositoryToVtkPartitionedDataSetCollection::searchFaultPolylines(const std::string &fileName)
{
    std::string result;

    std::list<std::string> name_to_sort;
    std::map<std::string, std::string> mapping_title_to_uuid;

    try
    {
        for (auto *polyline : repository->getFaultPolylineSetRepresentationSet())
        {
            if (searchNodeByUuid(polyline->getUuid()) == -1)
            { // verify uuid exist in treeview
                name_to_sort.push_back(polyline->getTitle());
                mapping_title_to_uuid[polyline->getTitle()] = polyline->getUuid();
            }
        }
        name_to_sort.sort();
        for (const auto &name : name_to_sort)
        {
            if (searchNodeByUuid(mapping_title_to_uuid[name]) == -1)
            { // verify uuid exist in treeview
                auto *representation = repository->getDataObjectByUuid<resqml2::AbstractRepresentation>(mapping_title_to_uuid[name]);
                result = result + searchRepresentations(representation, ResqmlDataRepositoryToVtkPartitionedDataSetCollection::EntityType::POLYLINE_SET);
            }
        }
    }
    catch (const std::exception &e)
    {
        return "EXCEPTION in fesapi when calling getFaultPolylineSetRepresentationSet with file: " + fileName + " : " + e.what();
    }
    return result;
}

std::string ResqmlDataRepositoryToVtkPartitionedDataSetCollection::searchHorizonPolylines(const std::string &fileName)
{
    std::string result;
    std::list<std::string> name_to_sort;
    std::map<std::string, std::string> mapping_title_to_uuid;

    try
    {
        for (auto *polyline : repository->getHorizonPolylineSetRepresentationSet())
        {
            if (searchNodeByUuid(polyline->getUuid()) == -1)
            { // verify uuid exist in treeview
                name_to_sort.push_back(polyline->getTitle());
                mapping_title_to_uuid[polyline->getTitle()] = polyline->getUuid();
            }
        }
        name_to_sort.sort();
        for (const auto &name : name_to_sort)
        {
            if (searchNodeByUuid(mapping_title_to_uuid[name]) == -1)
            { // verify uuid exist in treeview
                auto *representation = repository->getDataObjectByUuid<resqml2::AbstractRepresentation>(mapping_title_to_uuid[name]);
                result = result + searchRepresentations(representation, ResqmlDataRepositoryToVtkPartitionedDataSetCollection::EntityType::POLYLINE_SET);
            }
        }
    }
    catch (const std::exception &e)
    {
        return "EXCEPTION in fesapi when calling getHorizonPolylineSetRepresentationSet with file: " + fileName + " : " + e.what();
    }
    return result;
}

std::string ResqmlDataRepositoryToVtkPartitionedDataSetCollection::searchUnstructuredGrid(const std::string &fileName)
{
    std::string result;
    std::list<std::string> name_to_sort;
    std::map<std::string, std::string> mapping_title_to_uuid;

    try
    {
        for (auto *unstructuredGrid : repository->getUnstructuredGridRepresentationSet())
        {
            if (searchNodeByUuid(unstructuredGrid->getUuid()) == -1)
            { // verify uuid exist in treeview
                name_to_sort.push_back(unstructuredGrid->getTitle());
                mapping_title_to_uuid[unstructuredGrid->getTitle()] = unstructuredGrid->getUuid();
            }
        }
        name_to_sort.sort();
        for (const auto &name : name_to_sort)
        {
            if (searchNodeByUuid(mapping_title_to_uuid[name]) == -1)
            { // verify uuid exist in treeview
                auto *representation = repository->getDataObjectByUuid<resqml2::AbstractRepresentation>(mapping_title_to_uuid[name]);
                result = result + searchRepresentations(representation, ResqmlDataRepositoryToVtkPartitionedDataSetCollection::EntityType::UNSTRUC_GRID);
            }
        }
    }
    catch (const std::exception &e)
    {
        return "EXCEPTION in fesapi when calling getUnstructuredGridRepresentationSet with file: " + fileName + " : " + e.what();
    }
    return result;
}

std::string ResqmlDataRepositoryToVtkPartitionedDataSetCollection::searchFaultTriangulated(const std::string &fileName)
{
    std::string result;
    std::list<std::string> name_to_sort;
    std::map<std::string, std::string> mapping_title_to_uuid;

    try
    {
        for (auto *triangulated : repository->getFaultTriangulatedSetRepresentationSet())
        {
            if (searchNodeByUuid(triangulated->getUuid()) == -1)
            { // verify uuid exist in treeview
                name_to_sort.push_back(triangulated->getTitle());
                mapping_title_to_uuid[triangulated->getTitle()] = triangulated->getUuid();
            }
        }
        name_to_sort.sort();
        for (const auto &name : name_to_sort)
        {
            if (searchNodeByUuid(mapping_title_to_uuid[name]) == -1)
            { // verify uuid exist in treeview
                auto *representation = repository->getDataObjectByUuid<resqml2::AbstractRepresentation>(mapping_title_to_uuid[name]);
                result = result + searchRepresentations(representation, ResqmlDataRepositoryToVtkPartitionedDataSetCollection::EntityType::TRIANGULATED_SET);
            }
        }
    }
    catch (const std::exception &e)
    {
        return "EXCEPTION in fesapi when calling getFaultTriangulatedSetRepresentationSet with file: " + fileName + " : " + e.what();
    }
    return result;
}

std::string ResqmlDataRepositoryToVtkPartitionedDataSetCollection::searchHorizonTriangulated(const std::string &fileName)
{
    std::string result;
    std::list<std::string> name_to_sort;
    std::map<std::string, std::string> mapping_title_to_uuid;

    try
    {
        for (auto *triangulated : repository->getHorizonTriangulatedSetRepresentationSet())
        {
            if (searchNodeByUuid(triangulated->getUuid()) == -1)
            { // verify uuid exist in treeview
                name_to_sort.push_back(triangulated->getTitle());
                mapping_title_to_uuid[triangulated->getTitle()] = triangulated->getUuid();
            }
        }
        name_to_sort.sort();
        for (const auto &name : name_to_sort)
        {
            if (searchNodeByUuid(mapping_title_to_uuid[name]) == -1)
            { // verify uuid exist in treeview
                auto *representation = repository->getDataObjectByUuid<resqml2::AbstractRepresentation>(mapping_title_to_uuid[name]);
                result = result + searchRepresentations(representation, ResqmlDataRepositoryToVtkPartitionedDataSetCollection::EntityType::TRIANGULATED_SET);
            }
        }
    }
    catch (const std::exception &e)
    {
        return "EXCEPTION in fesapi when calling getHorizonTriangulatedSetRepresentationSet with file: " + fileName + " : " + e.what();
    }
    return result;
}

std::string ResqmlDataRepositoryToVtkPartitionedDataSetCollection::searchGrid2d(const std::string &fileName)
{
    std::string result;
    std::list<std::string> name_to_sort;
    std::map<std::string, std::string> mapping_title_to_uuid;

    try
    {
        for (auto *grid2D : repository->getHorizonGrid2dRepresentationSet())
        {
            if (searchNodeByUuid(grid2D->getUuid()) == -1)
            { // verify uuid exist in treeview
                name_to_sort.push_back(grid2D->getTitle());
                mapping_title_to_uuid[grid2D->getTitle()] = grid2D->getUuid();
            }
        }
        name_to_sort.sort();
        for (const auto &name : name_to_sort)
        {
            if (searchNodeByUuid(mapping_title_to_uuid[name]) == -1)
            { // verify uuid exist in treeview
                auto *representation = repository->getDataObjectByUuid<resqml2::AbstractRepresentation>(mapping_title_to_uuid[name]);
                result = result + searchRepresentations(representation, ResqmlDataRepositoryToVtkPartitionedDataSetCollection::EntityType::GRID_2D);
            }
        }
    }
    catch (const std::exception &e)
    {
        return "EXCEPTION in fesapi when calling getHorizonGrid2dRepresentationSet with file: " + fileName + " : " + e.what();
    }
    return result;
}

std::string ResqmlDataRepositoryToVtkPartitionedDataSetCollection::searchIjkGrid(const std::string &fileName)
{
    std::string result;

    std::list<std::string> name_to_sort;
    std::map<std::string, std::string> mapping_title_to_uuid;

    try
    {
        for (auto *ijkGrid : repository->getIjkGridRepresentationSet())
        {
            if (searchNodeByUuid(ijkGrid->getUuid()) == -1)
            { // verify uuid exist in treeview
                name_to_sort.push_back(ijkGrid->getTitle());
                mapping_title_to_uuid[ijkGrid->getTitle()] = ijkGrid->getUuid();
            }
        }
        name_to_sort.sort();
        for (auto const &name : name_to_sort)
        {
            if (searchNodeByUuid(mapping_title_to_uuid[name]) == -1)
            { // verify uuid exist in treeview
                auto *representation = repository->getDataObjectByUuid<resqml2::AbstractRepresentation>(mapping_title_to_uuid[name]);
                result = result + searchRepresentations(representation, ResqmlDataRepositoryToVtkPartitionedDataSetCollection::EntityType::IJK_GRID);
            }
        }
    }
    catch (const std::exception &e)
    {
        return "EXCEPTION in fesapi when calling getIjkGridRepresentationSet with file: " + fileName + " : " + e.what();
    }
    return result;
}

std::string ResqmlDataRepositoryToVtkPartitionedDataSetCollection::searchSubRepresentation(const std::string &fileName)
{
    std::string result;

    std::list<std::string> name_to_sort;
    std::map<std::string, std::string> mapping_title_to_uuid;

    try
    {
        for (auto *subRepresentation : repository->getSubRepresentationSet())
        {
            if (searchNodeByUuid(subRepresentation->getUuid()) == -1)
            { // verify uuid exist in treeview
                name_to_sort.push_back(subRepresentation->getTitle());
                mapping_title_to_uuid[subRepresentation->getTitle()] = subRepresentation->getUuid();
            }
        }
        name_to_sort.sort();
        for (auto const &name : name_to_sort)
        {
            if (searchNodeByUuid(mapping_title_to_uuid[name]) == -1)
            { // verify uuid exist in treeview
                auto *representation = repository->getDataObjectByUuid<resqml2::AbstractRepresentation>(mapping_title_to_uuid[name]);
                result = result + searchRepresentations(representation, ResqmlDataRepositoryToVtkPartitionedDataSetCollection::EntityType::SUB_REP);
            }
        }
    }
    catch (const std::exception &e)
    {
        return "EXCEPTION in fesapi when calling getSubRepresentationSet with file: " + fileName + " : " + e.what() + ".\n";
    }
    return result;
}

std::string ResqmlDataRepositoryToVtkPartitionedDataSetCollection::searchRepresentations(resqml2::AbstractRepresentation *representation, EntityType type)
{
    std::string result = "";
    vtkDataAssembly *data_assembly = output->GetDataAssembly();

    int idNode = 0; // 0 is root's id

    if (representation->isPartial())
    {
        // check if it has already been added
        bool uuid_exist = false;
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
            return " Partial UUID: (" + representation->getUuid() + ") is not loaded \n";
        } /******* TODO ********/ // exist but not the same type ?
    }
    else
    {
        representation->setTitle(changeInvalidCharacter(representation->getXmlTag() + '.' + representation->getTitle()));
        idNode = data_assembly->AddNode(representation->getTitle().c_str(), idNode);
        nodeId_to_uuid[idNode] = representation->getUuid();
        nodeId_to_EntityType[idNode] = type;
    }

    std::list<std::string> name_to_sort;
    std::map<std::string, std::vector<std::string>> mapping_title_to_uuidSet;
    try
    {
        // property
        for (auto *property : representation->getValuesPropertySet())
        {
            property->setTitle(changeInvalidCharacter(property->getXmlTag() + '.' + property->getTitle()));

            if (searchNodeByUuid(property->getUuid()) == -1)
            { // verify uuid exist in treeview
                name_to_sort.push_back(property->getTitle());
                mapping_title_to_uuidSet[property->getTitle()].push_back(property->getUuid());
            }
        }
        name_to_sort.sort();
        for (auto const &name : name_to_sort)
        {
            for (std::string uuid : mapping_title_to_uuidSet[name])
            {
                if (searchNodeByUuid(uuid) == -1)
                { // verify uuid exist in treeview
                    int property_idNode = data_assembly->AddNode(name.c_str(), idNode);
                    nodeId_to_uuid[property_idNode] = uuid;
                    nodeId_to_EntityType[property_idNode] = ResqmlDataRepositoryToVtkPartitionedDataSetCollection::EntityType::PROP;
                }
            }
        }
    }
    catch (const std::exception &e)
    {
        return "EXCEPTION in fesapi when calling getValuesPropertySet with representation uuid: " + nodeId_to_uuid[idNode] + " : " + e.what() + ".\n";
    }
    return result;
}

std::string ResqmlDataRepositoryToVtkPartitionedDataSetCollection::searchWellboreTrajectory(const std::string &fileName)
{
    std::string result;

    vtkDataAssembly *data_assembly = output->GetDataAssembly();

    int idNode = 0; // 0 is root's id

    for (auto *wellboreTrajectory : repository->getWellboreTrajectoryRepresentationSet())
    {
        if (searchNodeByUuid(wellboreTrajectory->getUuid()) == -1)
        { // verify uuid exist in treeview
            if (wellboreTrajectory->isPartial())
            {
                // check if it has already been added
                bool uuid_exist = false;
                for (std::map<int, std::string>::iterator it = nodeId_to_uuid.begin(); it != nodeId_to_uuid.end(); ++it)
                {
                    if (wellboreTrajectory->getUuid() == it->second)
                    {
                        uuid_exist = true;
                        continue;
                    }
                }

                // not exist => not loaded
                if (!uuid_exist)
                {
                    result = result + " Partial UUID: (" + wellboreTrajectory->getUuid() + ") is not loaded \n";
                    continue;
                } /******* TODO ********/ // exist but not the same type ?
            }
            else
            {
                /*
                                const auto *interpretation = wellboreTrajectory->getInterpretation();
                                if (interpretation != nullptr)
                                {
                                    idNode = data_assembly->AddNode(changeInvalidCharacter(interpretation->getXmlTag() + '.' + interpretation->getTitle()).c_str());
                                    nodeId_to_uuid[idNode] = interpretation->getUuid();
                                    nodeId_to_EntityType[idNode] = ResqmlDataRepositoryToVtkPartitionedDataSetCollection::EntityType::INTERPRETATION;
                                }
                                */
                wellboreTrajectory->setTitle(changeInvalidCharacter(wellboreTrajectory->getXmlTag() + '.' + wellboreTrajectory->getTitle()));
                idNode = data_assembly->AddNode(wellboreTrajectory->getTitle().c_str(), idNode);
                nodeId_to_uuid[idNode] = wellboreTrajectory->getUuid();
                nodeId_to_EntityType[idNode] = ResqmlDataRepositoryToVtkPartitionedDataSetCollection::EntityType::WELL_TRAJ;
            }
        }
        // wellboreFrame
        for (auto *wellboreFrame : wellboreTrajectory->getWellboreFrameRepresentationSet())
        {
            if (searchNodeByUuid(wellboreFrame->getUuid()) == -1)
            { // verify uuid exist in treeview
                auto *wellboreMarkerFrame = dynamic_cast<RESQML2_NS::WellboreMarkerFrameRepresentation const *>(wellboreFrame);
                if (wellboreMarkerFrame == nullptr)
                { // WellboreFrame
                    wellboreFrame->setTitle(changeInvalidCharacter(wellboreFrame->getXmlTag() + '.' + wellboreFrame->getTitle()));
                    int frame_idNode = data_assembly->AddNode(wellboreFrame->getTitle().c_str(), idNode);
                    nodeId_to_uuid[frame_idNode] = wellboreFrame->getUuid();
                    nodeId_to_EntityType[frame_idNode] = ResqmlDataRepositoryToVtkPartitionedDataSetCollection::EntityType::WELL_FRAME;
                    for (auto *property : wellboreFrame->getValuesPropertySet())
                    {
                        property->setTitle(changeInvalidCharacter(property->getXmlTag() + '.' + property->getTitle()));
                        int property_idNode = data_assembly->AddNode(property->getTitle().c_str(), frame_idNode);
                        nodeId_to_uuid[property_idNode] = property->getUuid();
                        nodeId_to_EntityType[property_idNode] = ResqmlDataRepositoryToVtkPartitionedDataSetCollection::EntityType::WELL_CHANNEL;
                    }
                }
                else
                { // WellboreMarkerFrame
                    wellboreFrame->setTitle(changeInvalidCharacter(wellboreFrame->getXmlTag() + '.' + wellboreFrame->getTitle()));
                    int markerFrame_idNode = data_assembly->AddNode(wellboreFrame->getTitle().c_str(), idNode);
                    nodeId_to_uuid[markerFrame_idNode] = wellboreFrame->getUuid();
                    nodeId_to_EntityType[markerFrame_idNode] = ResqmlDataRepositoryToVtkPartitionedDataSetCollection::EntityType::WELL_MARKER_FRAME;
                    for (auto *wellboreMarker : wellboreMarkerFrame->getWellboreMarkerSet())
                    {
                        wellboreMarker->setTitle(changeInvalidCharacter(wellboreMarker->getXmlTag() + '.' + wellboreMarker->getTitle()));
                        int marker_idNode = data_assembly->AddNode(wellboreMarker->getTitle().c_str(), markerFrame_idNode);
                        nodeId_to_uuid[marker_idNode] = wellboreMarker->getUuid();
                        nodeId_to_EntityType[marker_idNode] = ResqmlDataRepositoryToVtkPartitionedDataSetCollection::EntityType::WELL_MARKER;
                    }
                }
            }
        }
    }
    return result;
}

std::string ResqmlDataRepositoryToVtkPartitionedDataSetCollection::searchTimeSeries(const std::string &fileName)
{
    times_step.clear();

    std::string return_message = "";
    auto assembly = output->GetDataAssembly();
    std::vector<EML2_NS::TimeSeries *> timeSeries;
    try
    {
        timeSeries = repository->getTimeSeriesSet();
    }
    catch (const std::exception &e)
    {
        return_message = return_message + "EXCEPTION in fesapi when calling getTimeSeriesSet with file: " + fileName + " : " + e.what();
    }

    /****
     *  change property parent to times serie parent
     ****/
    for (auto *timeSerie : timeSeries)
    {
        bool valid = true;

        // get properties link to Times series
        try
        {
            auto propSeries = timeSerie->getPropertySet();

            std::vector<std::string> property_name_set;
            std::map<std::string, std::vector<int>> property_name_to_node_set;
            int node_parent = -1;
            for (auto *prop : propSeries)
            {
                if (prop->getXmlTag() == RESQML2_NS::ContinuousProperty::XML_TAG ||
                    prop->getXmlTag() == RESQML2_NS::DiscreteProperty::XML_TAG)
                {
                    auto node_id = (searchNodeByUuid(prop->getUuid()));
                    if (node_id == -1)
                    {
                        return_message = return_message + "The property " + prop->getUuid() + " is not supported and consequently cannot be associated to its time series.\n";
                        continue;
                    }
                    else
                    {
                        // same node parent else not supported
                        if (node_parent == -1)
                        {
                            node_parent = assembly->GetParent(node_id);
                        }

                        if (node_parent == assembly->GetParent(node_id))
                        {
                            if (prop->getTimeIndicesCount() > 1)
                            {
                                return_message = return_message + "The property " + prop->getUuid() + " correspond to more than one time index. It is not supported yet.\n";
                                continue;
                            }
                            property_name_to_node_set[prop->getTitle()].push_back(node_id);
                            times_step.push_back(prop->getTimeIndexStart());
                            timeSeries_uuid_and_title_to_index_and_properties_uuid[timeSerie->getUuid()][changeInvalidCharacter(timeSerie->getXmlTag() + '.' + prop->getTitle())][prop->getTimeIndexStart()] = prop->getUuid();
                        }
                        else
                        {
                            valid = false;
                            return_message = return_message + "The properties of time series " + timeSerie->getUuid() + " aren't same parent and is not supported.\n";
                        }
                    }
                }
            }
            if (valid)
            {
                // erase duplicate Index
                sort(times_step.begin(), times_step.end());
                times_step.erase(unique(times_step.begin(), times_step.end()), times_step.end());

                for (const auto &myPair : property_name_to_node_set)
                {
                    std::vector<int> property_node_set = myPair.second;

                    for (auto node : property_node_set)
                    {
                        output->GetDataAssembly()->RemoveNode(node);
                        nodeId_to_EntityType.erase(node);
                        nodeId_to_resqml.erase(node);
                        nodeId_to_uuid.erase(node);
                    }
                    std::string name = changeInvalidCharacter(timeSerie->getXmlTag() + '.' + myPair.first);
                    auto times_serie_node_id = output->GetDataAssembly()->AddNode(name.c_str(), node_parent);

                    nodeId_to_EntityType[times_serie_node_id] = ResqmlDataRepositoryToVtkPartitionedDataSetCollection::EntityType::TIMES_SERIE;
                    // for selection add node name to timeserie uuid
                    nodeId_to_uuid[times_serie_node_id] = timeSerie->getUuid()+name;
                }
            }
        }
        catch (const std::exception &e)
        {
            return_message = return_message + "EXCEPTION in fesapi when calling getPropertySet with file: " + fileName + " : " + e.what();
        }
    }

    return return_message;
}

void ResqmlDataRepositoryToVtkPartitionedDataSetCollection::selectNodeId(int node)
{
    auto *assembly = this->output->GetDataAssembly();

    this->selectNodeIdParent(node);

    this->current_selection.insert(node);
    this->old_selection.erase(node);

    this->selectNodeIdChildren(node);
}

void ResqmlDataRepositoryToVtkPartitionedDataSetCollection::selectNodeIdParent(int node)
{
    auto *assembly = this->output->GetDataAssembly();

    if (assembly->GetParent(node) != -1)
    {
        this->current_selection.insert(assembly->GetParent(node));
        this->old_selection.erase(assembly->GetParent(node));
        this->selectNodeIdParent(assembly->GetParent(node));
    }
}
void ResqmlDataRepositoryToVtkPartitionedDataSetCollection::selectNodeIdChildren(int node)
{
    auto *assembly = this->output->GetDataAssembly();

    for (int index_child : assembly->GetChildNodes(node))
    {
        this->current_selection.insert(index_child);
        this->old_selection.erase(index_child);
        this->selectNodeIdChildren(index_child);
    }
}

void ResqmlDataRepositoryToVtkPartitionedDataSetCollection::clearSelection()
{
    this->old_selection = this->current_selection;
    this->current_selection.clear();
}

ResqmlAbstractRepresentationToVtkDataset *ResqmlDataRepositoryToVtkPartitionedDataSetCollection::loadToVtk(std::string uuid, EntityType entity, double time)
{
    switch (entity)
    {
    case ResqmlDataRepositoryToVtkPartitionedDataSetCollection::EntityType::IJK_GRID:
    {
        try
        {
            auto *ijkGrid = repository->getDataObjectByUuid<RESQML2_NS::AbstractIjkGridRepresentation>(uuid);
            auto *rep = new ResqmlIjkGridToVtkUnstructuredGrid(ijkGrid);
            nodeId_to_resqml[searchNodeByUuid(uuid)] = rep;
            return rep;
        }
        catch (const std::exception &e)
        {
            vtkOutputWindowDisplayErrorText(("Error in ijkgrid load for uuid: " + uuid).c_str());
        }
        break;
    }
    case ResqmlDataRepositoryToVtkPartitionedDataSetCollection::EntityType::GRID_2D:
    {
        try
        {
            auto *grid2D = repository->getDataObjectByUuid<RESQML2_NS::Grid2dRepresentation>(uuid);
            auto *rep = new ResqmlGrid2dToVtkPolyData(grid2D);
            nodeId_to_resqml[searchNodeByUuid(uuid)] = rep;
            return rep;
        }
        catch (const std::exception &e)
        {
            vtkOutputWindowDisplayErrorText(("Error in grid 2D load for uuid: " + uuid + "\n").c_str());
        }
        break;
    }
    case ResqmlDataRepositoryToVtkPartitionedDataSetCollection::EntityType::TRIANGULATED_SET:
    {
        try
        {
            auto *triangles = repository->getDataObjectByUuid<RESQML2_NS::TriangulatedSetRepresentation>(uuid);
            auto *rep = new ResqmlTriangulatedSetToVtkPartitionedDataSet(triangles);
            nodeId_to_resqml[searchNodeByUuid(uuid)] = rep;
            return rep;
        }
        catch (const std::exception &e)
        {
            vtkOutputWindowDisplayErrorText(("Error in triangulated load for uuid: " + uuid + "\n").c_str());
        }
        break;
    }
    case ResqmlDataRepositoryToVtkPartitionedDataSetCollection::EntityType::POLYLINE_SET:
    {
        try
        {
            auto *polyline = repository->getDataObjectByUuid<RESQML2_NS::PolylineSetRepresentation>(uuid);
            auto *rep = new ResqmlPolylineToVtkPolyData(polyline);
            nodeId_to_resqml[searchNodeByUuid(uuid)] = rep;
            return rep;
        }
        catch (const std::exception &e)
        {
            vtkOutputWindowDisplayErrorText(("Error in polyline load for uuid: " + uuid + "\n").c_str());
        }
        break;
    }
    case ResqmlDataRepositoryToVtkPartitionedDataSetCollection::EntityType::UNSTRUC_GRID:
    {
        try
        {
            auto *unstruct = repository->getDataObjectByUuid<RESQML2_NS::UnstructuredGridRepresentation>(uuid);
            auto *rep = new ResqmlUnstructuredGridToVtkUnstructuredGrid(unstruct);
            nodeId_to_resqml[searchNodeByUuid(uuid)] = rep;
            return rep;
        }
        catch (const std::exception &e)
        {
            vtkOutputWindowDisplayErrorText(("Error in unstructured grid load for uuid: " + uuid + "\n").c_str());
        }
        break;
    }
    case ResqmlDataRepositoryToVtkPartitionedDataSetCollection::EntityType::WELL_TRAJ:
    {
        try
        {
            auto *wellbore_traj = repository->getDataObjectByUuid<RESQML2_NS::WellboreTrajectoryRepresentation>(uuid);
            auto *rep = new ResqmlWellboreTrajectoryToVtkPolyData(wellbore_traj);
            nodeId_to_resqml[searchNodeByUuid(uuid)] = rep;
            return rep;
        }
        catch (const std::exception &e)
        {
            vtkOutputWindowDisplayErrorText(("Error in wellbore trajectory load for uuid: " + uuid + "\n").c_str());
        }
        break;
    }
    case ResqmlDataRepositoryToVtkPartitionedDataSetCollection::EntityType::WELL_FRAME:
    {
        try
        {
            auto wellbore_frame = repository->getDataObjectByUuid<RESQML2_NS::WellboreFrameRepresentation>(uuid);
            auto rep = new ResqmlWellboreFrameToVtkPartitionedDataSet(wellbore_frame);
            nodeId_to_resqml[searchNodeByUuid(uuid)] = rep;
            return rep;
        }
        catch (const std::exception &e)
        {
            vtkOutputWindowDisplayErrorText(("Error in wellbore frame load for uuid: " + uuid + "\n").c_str());
        }
        break;
    }
    case ResqmlDataRepositoryToVtkPartitionedDataSetCollection::EntityType::WELL_CHANNEL:
    {
        try
        {
            auto *assembly = this->output->GetDataAssembly();
            const int node_parent = assembly->GetParent(searchNodeByUuid(uuid));

            if (nodeId_to_resqml[node_parent])
            {
                auto wellbore_frame = repository->getDataObjectByUuid<RESQML2_NS::WellboreFrameRepresentation>(nodeId_to_uuid[node_parent]);
                for (auto *property : wellbore_frame->getValuesPropertySet())
                {
                    if (property->getUuid() == uuid)
                    {
                        auto *resqmlWellboreFrameToVtkPartitionedDataSet = dynamic_cast<ResqmlWellboreFrameToVtkPartitionedDataSet *>(nodeId_to_resqml[node_parent]);
                        resqmlWellboreFrameToVtkPartitionedDataSet->addChannel(uuid, property);
                    }
                }
            }
        }
        catch (const std::exception &e)
        {
            vtkOutputWindowDisplayErrorText(("Error in wellbore channel load for uuid: " + uuid + "\n").c_str());
        }
        break;
    }
    case ResqmlDataRepositoryToVtkPartitionedDataSetCollection::EntityType::WELL_MARKER_FRAME:
    {
        try
        {
            auto *wellbore_marker_frame = repository->getDataObjectByUuid<RESQML2_NS::WellboreMarkerFrameRepresentation>(uuid);
            auto *rep = new ResqmlWellboreMarkerFrameToVtkPartitionedDataSet(wellbore_marker_frame, this->markerOrientation, this->markerSize);
            nodeId_to_resqml[searchNodeByUuid(uuid)] = rep;
            return rep;
        }
        catch (const std::exception &e)
        {
            vtkOutputWindowDisplayErrorText(("Error in wellbore marker frame load for uuid: " + uuid + "\n").c_str());
        }
        break;
    }
    case ResqmlDataRepositoryToVtkPartitionedDataSetCollection::EntityType::WELL_MARKER:
    {
        try
        {
            auto *assembly = this->output->GetDataAssembly();
            const int node_parent = assembly->GetParent(searchNodeByUuid(uuid));
            if (nodeId_to_resqml[node_parent])
            {
                auto *resqmlWellboreMarkerFrameToVtkPartitionedDataSet = dynamic_cast<ResqmlWellboreMarkerFrameToVtkPartitionedDataSet *>(nodeId_to_resqml[node_parent]);
                resqmlWellboreMarkerFrameToVtkPartitionedDataSet->addMarker(uuid);
            }
        }
        catch (const std::exception &e)
        {
            vtkOutputWindowDisplayErrorText(("Error in wellbore marker load for uuid: " + uuid + "\n").c_str());
        }
        break;
    }
    case ResqmlDataRepositoryToVtkPartitionedDataSetCollection::EntityType::PROP:
    {
        try
        {
            auto *assembly = this->output->GetDataAssembly();
            const int node_parent = assembly->GetParent(searchNodeByUuid(uuid));
            if (nodeId_to_resqml[node_parent])
            {
                nodeId_to_resqml[node_parent]->addDataArray(uuid);
            }
        }
        catch (const std::exception &e)
        {
            vtkOutputWindowDisplayErrorText(("Error in property load for uuid: " + uuid + "\n").c_str());
        }
        break;
    }
    case ResqmlDataRepositoryToVtkPartitionedDataSetCollection::EntityType::TIMES_SERIE:
    {
        try
        {
            // decompose uuid by Timeserie uuid + node name
            std::string ts_uuid = uuid.substr (0, 36); 
            std::string node_name = uuid.substr (36); 

            auto *assembly = this->output->GetDataAssembly();
            const int node_parent = assembly->GetParent(searchNodeByUuid(uuid));
            if (nodeId_to_resqml[node_parent])
            {
                nodeId_to_resqml[node_parent]->addDataArray(timeSeries_uuid_and_title_to_index_and_properties_uuid[ts_uuid][node_name][time]);
            }
        }
        catch (const std::exception &e)
        {
            vtkOutputWindowDisplayErrorText(("Error in property load for uuid: " + uuid + "\n").c_str());
        }
        break;
    }
    }
    return nullptr;
}

std::string ResqmlDataRepositoryToVtkPartitionedDataSetCollection::changeInvalidCharacter(std::string text)
{
    std::replace(text.begin(), text.end(), ' ', '_');
    std::replace(text.begin(), text.end(), '(', '.');
    std::replace(text.begin(), text.end(), ')', '.');
    std::replace(text.begin(), text.end(), '+', '.');
    return text;
}

vtkPartitionedDataSetCollection *ResqmlDataRepositoryToVtkPartitionedDataSetCollection::getVtkPartionedDatasSetCollection(const double time)
{
    // initialization the output (VtkPartitionedDatasSetCollection)
    for (unsigned int index_partitioned = 0; index_partitioned < this->output->GetNumberOfPartitionedDataSets(); index_partitioned++)
    {
        this->output->RemovePartitionedDataSet(index_partitioned);
    }

    unsigned int index = 0; // index for PartionedDatasSet

    // foreach selection node
    for (const int node_selection : this->current_selection)
    {
        ResqmlAbstractRepresentationToVtkDataset *rep = nullptr;
        if (this->nodeId_to_resqml.find(node_selection) != this->nodeId_to_resqml.end())
        {
            rep = this->nodeId_to_resqml[node_selection];
        }
        else
        {
            rep = loadToVtk(nodeId_to_uuid[node_selection], nodeId_to_EntityType[node_selection], time);
        }
        if (rep)
        {
            this->output->SetPartitionedDataSet(index, rep->getOutput());
            this->output->GetMetaData(index++)->Set(vtkCompositeDataSet::NAME(), this->output->GetDataAssembly()->GetNodeName(node_selection));
        }
    }

    // delete Property or Wellbore marker
    for (const int selection : this->old_selection)
    {
        if (nodeId_to_EntityType[selection] == ResqmlDataRepositoryToVtkPartitionedDataSetCollection::EntityType::PROP)
        {
            try
            {
                auto *assembly = this->output->GetDataAssembly();
                const int node_parent = assembly->GetParent(searchNodeByUuid(nodeId_to_uuid[selection]));
                if (this->nodeId_to_resqml.find(node_parent) != this->nodeId_to_resqml.end())
                {
                    ResqmlAbstractRepresentationToVtkDataset *rep = this->nodeId_to_resqml[node_parent];
                    rep->deleteDataArray(nodeId_to_uuid[selection]);
                }
            }
            catch (const std::exception &e)
            {
                vtkOutputWindowDisplayErrorText(("Error in property load for uuid: " + nodeId_to_uuid[selection] + "\n").c_str());
            }
        }
        if (nodeId_to_EntityType[selection] == ResqmlDataRepositoryToVtkPartitionedDataSetCollection::EntityType::WELL_MARKER)
        {
            try
            {
                auto *assembly = this->output->GetDataAssembly();
                const int node_parent = assembly->GetParent(searchNodeByUuid(nodeId_to_uuid[selection]));
                if (this->nodeId_to_resqml.find(node_parent) != this->nodeId_to_resqml.end())
                {
                    ResqmlWellboreMarkerFrameToVtkPartitionedDataSet *rep = dynamic_cast<ResqmlWellboreMarkerFrameToVtkPartitionedDataSet *>(this->nodeId_to_resqml[node_parent]);
                    rep->removeMarker(nodeId_to_uuid[selection]);
                }
            }
            catch (const std::exception &e)
            {
                vtkOutputWindowDisplayErrorText(("Error in property load for uuid: " + nodeId_to_uuid[selection] + "\n").c_str());
            }
        }
    }

    // delete representation
    for (const int selection : this->old_selection)
    {
        if (this->nodeId_to_resqml.find(selection) != this->nodeId_to_resqml.end())
        {
            this->nodeId_to_resqml.erase(selection); // erase object
        }
    }

    this->output->Modified();
    return this->output;
}

int ResqmlDataRepositoryToVtkPartitionedDataSetCollection::searchNodeByUuid(const std::string &uuid)
{
    for (const auto &it : this->nodeId_to_uuid)
    {
        if (it.second == uuid)
        {
            return it.first;
        }
    }

    return -1;
}
