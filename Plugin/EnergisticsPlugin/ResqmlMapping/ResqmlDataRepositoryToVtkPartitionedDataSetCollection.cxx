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

#include <algorithm>
#include <vector>
#include <set>
//#include <map>

// VTK includes
#include <vtkPartitionedDataSetCollection.h>
#include <vtkPartitionedDataSet.h>
#include <vtkInformation.h>
#include <vtkDataAssembly.h>
#include <vtkDataArraySelection.h>

// FESAPI includes
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

ResqmlDataRepositoryToVtkPartitionedDataSetCollection::ResqmlDataRepositoryToVtkPartitionedDataSetCollection(int proc_number, int max_proc)
    : output(vtkSmartPointer<vtkPartitionedDataSetCollection>::New()),
      repository(new common::DataObjectRepository()),
      proc_number(proc_number),
      max_proc(max_proc)
{
    auto assembly = vtkSmartPointer<vtkDataAssembly>::New();
    assembly->SetRootNodeName("data");
    nodeId_to_EntityType[0] = ResqmlDataRepositoryToVtkPartitionedDataSetCollection::EntityType::INTERPRETATION;
    this->output->SetDataAssembly(assembly);

    this->current_selection = {};
    this->old_selection = {};
}

ResqmlDataRepositoryToVtkPartitionedDataSetCollection::~ResqmlDataRepositoryToVtkPartitionedDataSetCollection()
{
}

//----------------------------------------------------------------------------
vtkDataAssembly *ResqmlDataRepositoryToVtkPartitionedDataSetCollection::GetAssembly()
{
    return output->GetDataAssembly();
}

//----------------------------------------------------------------------------
void ResqmlDataRepositoryToVtkPartitionedDataSetCollection::addFile(const char *fileName)
{
    std::string message;

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
    message = message + searchFaultTriangulated(fileName);
    message = message + searchHorizonTriangulated(fileName);
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
    std::string result = "";

    std::vector<RESQML2_NS::PolylineSetRepresentation *> polylines;
    try
    {
        polylines = repository->getFaultPolylineSetRepresentationSet();
        for (auto &polyline : polylines)
        {
            if (searchNodeByUuid(polyline->getUuid()) == -1)
            { // verify uuid exist in treeview
                auto representation = dynamic_cast<resqml2::AbstractRepresentation *>(polyline);
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
    std::string result = "";

    std::vector<RESQML2_NS::PolylineSetRepresentation *> polylines;
    try
    {
        polylines = repository->getHorizonPolylineSetRepresentationSet();
        for (auto &polyline : polylines)
        {
            if (searchNodeByUuid(polyline->getUuid()) == -1)
            { // verify uuid exist in treeview
                auto representation = dynamic_cast<resqml2::AbstractRepresentation *>(polyline);
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
    std::string result = "";

    std::vector<RESQML2_NS::UnstructuredGridRepresentation *> unstructuredGrid_set;
    try
    {
        unstructuredGrid_set = repository->getUnstructuredGridRepresentationSet();
        for (auto &unstructuredGrid : unstructuredGrid_set)
        {
            if (searchNodeByUuid(unstructuredGrid->getUuid()) == -1)
            { // verify uuid exist in treeview
                auto representation = dynamic_cast<resqml2::AbstractRepresentation *>(unstructuredGrid);
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
    std::string result = "";

    std::vector<RESQML2_NS::TriangulatedSetRepresentation *> triangulated_set;
    try
    {
        triangulated_set = repository->getFaultTriangulatedSetRepresentationSet();
        for (auto &triangulated : triangulated_set)
        {
            if (searchNodeByUuid(triangulated->getUuid()) == -1)
            { // verify uuid exist in treeview
                auto representation = dynamic_cast<resqml2::AbstractRepresentation *>(triangulated);
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
    std::string result = "";

    std::vector<RESQML2_NS::TriangulatedSetRepresentation *> triangulated_set;
    try
    {
        triangulated_set = repository->getHorizonTriangulatedSetRepresentationSet();
        for (auto &triangulated : triangulated_set)
        {
            if (searchNodeByUuid(triangulated->getUuid()) == -1)
            { // verify uuid exist in treeview
                auto representation = dynamic_cast<resqml2::AbstractRepresentation *>(triangulated);
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
    std::string result = "";

    std::vector<RESQML2_NS::Grid2dRepresentation *> grid2D_set;
    try
    {
        grid2D_set = repository->getHorizonGrid2dRepresentationSet();
        for (auto &grid2D : grid2D_set)
        {
            if (searchNodeByUuid(grid2D->getUuid()) == -1)
            { // verify uuid exist in treeview
                auto representation = dynamic_cast<resqml2::AbstractRepresentation *>(grid2D);
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
    std::string result = "";

    std::vector<RESQML2_NS::AbstractIjkGridRepresentation *> ijkGrid_set;
    try
    {
        ijkGrid_set = repository->getIjkGridRepresentationSet();
        for (auto &ijkGrid : ijkGrid_set)
        {
            if (searchNodeByUuid(ijkGrid->getUuid()) == -1)
            { // verify uuid exist in treeview
                auto representation = dynamic_cast<resqml2::AbstractRepresentation *>(ijkGrid);
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
    std::string result = "";

    std::vector<RESQML2_NS::SubRepresentation *> subRepresentationSet;
    try
    {
        subRepresentationSet = repository->getSubRepresentationSet();
        for (auto &subRepresentation : subRepresentationSet)
        {
            if (searchNodeByUuid(subRepresentation->getUuid()) == -1)
            { // verify uuid exist in treeview
                auto representation = dynamic_cast<resqml2::AbstractRepresentation *>(subRepresentation);
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
        const auto* interpretation = representation->getInterpretation();
        if (interpretation != nullptr)
        {
            std::string name_node = changeInvalidCharacter(interpretation->getXmlTag() + '.' + interpretation->getTitle());
            idNode = data_assembly->GetFirstNodeByPath(name_node.c_str());
            if (idNode == -1)
            {
                idNode = data_assembly->AddNode(name_node.c_str());
                nodeId_to_uuid[idNode] = interpretation->getUuid();
                nodeId_to_EntityType[idNode] = ResqmlDataRepositoryToVtkPartitionedDataSetCollection::EntityType::INTERPRETATION;
            }
        }
        idNode = data_assembly->AddNode(changeInvalidCharacter(representation->getXmlTag() + '.' + representation->getTitle()).c_str(), idNode);
        nodeId_to_uuid[idNode] = representation->getUuid();
        nodeId_to_EntityType[idNode] = type;
    }

    // property
    for (const auto* property : representation->getValuesPropertySet())
    {
        int property_idNode = data_assembly->AddNode(changeInvalidCharacter(property->getXmlTag() + '.' + property->getTitle()).c_str(), idNode);
        nodeId_to_uuid[property_idNode] = property->getUuid();
        nodeId_to_EntityType[property_idNode] = ResqmlDataRepositoryToVtkPartitionedDataSetCollection::EntityType::PROP;
    }
    return "";
}

std::string ResqmlDataRepositoryToVtkPartitionedDataSetCollection::searchWellboreTrajectory(const std::string &fileName)
{
    std::string result = "";

    vtkDataAssembly *data_assembly = output->GetDataAssembly();

    int idNode = 0; // 0 is root's id

    for (const auto* wellboreTrajectory : repository->getWellboreTrajectoryRepresentationSet())
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
                const auto* interpretation = wellboreTrajectory->getInterpretation();
                if (interpretation != nullptr)
                {
                    idNode = data_assembly->AddNode(changeInvalidCharacter(interpretation->getXmlTag() + '.' + interpretation->getTitle()).c_str());
                    nodeId_to_uuid[idNode] = interpretation->getUuid();
                    nodeId_to_EntityType[idNode] = ResqmlDataRepositoryToVtkPartitionedDataSetCollection::EntityType::INTERPRETATION;
                }
                idNode = data_assembly->AddNode(changeInvalidCharacter(wellboreTrajectory->getXmlTag() + '.' + wellboreTrajectory->getTitle()).c_str(), idNode);
                nodeId_to_uuid[idNode] = wellboreTrajectory->getUuid();
                nodeId_to_EntityType[idNode] = ResqmlDataRepositoryToVtkPartitionedDataSetCollection::EntityType::WELL_TRAJ;
            }
        }
        // wellboreFrame
        for (const auto* wellboreFrame : wellboreTrajectory->getWellboreFrameRepresentationSet())
        {
            if (searchNodeByUuid(wellboreFrame->getUuid()) == -1)
            { // verify uuid exist in treeview
                const auto* wellboreMarkerFrame = dynamic_cast<RESQML2_NS::WellboreMarkerFrameRepresentation const*>(wellboreFrame);
                if (wellboreMarkerFrame == nullptr)
                { // WellboreFrame
                    int frame_idNode = data_assembly->AddNode(changeInvalidCharacter(wellboreFrame->getXmlTag() + '.' + wellboreFrame->getTitle()).c_str(), idNode);
                    nodeId_to_uuid[frame_idNode] = wellboreFrame->getUuid();
                    nodeId_to_EntityType[frame_idNode] = ResqmlDataRepositoryToVtkPartitionedDataSetCollection::EntityType::WELL_MARKER_FRAME;
                    for (const auto* property : wellboreFrame->getValuesPropertySet())
                    {
                        int property_idNode = data_assembly->AddNode(changeInvalidCharacter(property->getXmlTag() + '.' + property->getTitle()).c_str(), frame_idNode);
                        nodeId_to_uuid[property_idNode] = property->getUuid();
                        nodeId_to_EntityType[property_idNode] = ResqmlDataRepositoryToVtkPartitionedDataSetCollection::EntityType::PROP;
                    }
                }
                else
                { // WellboreMarkerFrame
                    int markerFrame_idNode = data_assembly->AddNode(changeInvalidCharacter(wellboreFrame->getXmlTag() + '.' + wellboreFrame->getTitle()).c_str(), idNode);
                    nodeId_to_uuid[markerFrame_idNode] = wellboreFrame->getUuid();
                    nodeId_to_EntityType[markerFrame_idNode] = ResqmlDataRepositoryToVtkPartitionedDataSetCollection::EntityType::WELL_MARKER_FRAME;
                    for (const auto* wellboreMarker : wellboreMarkerFrame->getWellboreMarkerSet())
                    {
                        int marker_idNode = data_assembly->AddNode(changeInvalidCharacter(wellboreMarker->getXmlTag() + '.' + wellboreMarker->getTitle()).c_str(), markerFrame_idNode);
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
    return return_message;
    /******* TODO ********/
}

void ResqmlDataRepositoryToVtkPartitionedDataSetCollection::selectNodeId(int node)
{
    auto assembly = this->output->GetDataAssembly();

    this->selectNodeIdParent(node);

    this->current_selection.insert(node);
    this->old_selection.erase(node);

    this->selectNodeIdChildren(node);
}

void ResqmlDataRepositoryToVtkPartitionedDataSetCollection::selectNodeIdParent(int node)
{
    auto assembly = this->output->GetDataAssembly();

    if (assembly->GetParent(node) != -1)
    {
        this->current_selection.insert(assembly->GetParent(node));
        this->old_selection.erase(assembly->GetParent(node));
        this->selectNodeIdParent(assembly->GetParent(node));
    }
}
void ResqmlDataRepositoryToVtkPartitionedDataSetCollection::selectNodeIdChildren(int node)
{
    auto assembly = this->output->GetDataAssembly();

    for (auto &index_child : assembly->GetChildNodes(node))
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

ResqmlAbstractRepresentationToVtkDataset *ResqmlDataRepositoryToVtkPartitionedDataSetCollection::loadToVtk(std::string uuid, EntityType entity)
{
    switch (entity)
    {
    case ResqmlDataRepositoryToVtkPartitionedDataSetCollection::EntityType::IJK_GRID:
    {
        auto ijkGrid = repository->getDataObjectByUuid<RESQML2_NS::AbstractIjkGridRepresentation>(uuid);
        auto rep = new ResqmlIjkGridToVtkUnstructuredGrid(ijkGrid);
        nodeId_to_resqml[searchNodeByUuid(uuid)] = rep;
        return rep;
        break;
    }
    case ResqmlDataRepositoryToVtkPartitionedDataSetCollection::EntityType::GRID_2D:
    {
        auto grid2D = repository->getDataObjectByUuid<RESQML2_NS::Grid2dRepresentation>(uuid);
        auto rep = new ResqmlGrid2dToVtkPolyData(grid2D);
        nodeId_to_resqml[searchNodeByUuid(uuid)] = rep;
        return rep;
        break;
    }
    case ResqmlDataRepositoryToVtkPartitionedDataSetCollection::EntityType::TRIANGULATED_SET:
    {
        auto triangles = repository->getDataObjectByUuid<RESQML2_NS::TriangulatedSetRepresentation>(uuid);
        auto rep = new ResqmlTriangulatedSetToVtkPartitionedDataSet(triangles);
        nodeId_to_resqml[searchNodeByUuid(uuid)] = rep;
        return rep;
        break;
    }
    case ResqmlDataRepositoryToVtkPartitionedDataSetCollection::EntityType::POLYLINE_SET:
    {
        auto polyline = repository->getDataObjectByUuid<RESQML2_NS::PolylineSetRepresentation>(uuid);
        auto rep = new ResqmlPolylineToVtkPolyData(polyline);
        nodeId_to_resqml[searchNodeByUuid(uuid)] = rep;
        return rep;
        break;
    }
    case ResqmlDataRepositoryToVtkPartitionedDataSetCollection::EntityType::UNSTRUC_GRID:
    {
        auto unstruct = repository->getDataObjectByUuid<RESQML2_NS::UnstructuredGridRepresentation>(uuid);
        auto rep = new ResqmlUnstructuredGridToVtkUnstructuredGrid(unstruct);
        nodeId_to_resqml[searchNodeByUuid(uuid)] = rep;
        return rep;
        break;
    }
    case ResqmlDataRepositoryToVtkPartitionedDataSetCollection::EntityType::WELL_TRAJ:
    {
        auto wellbore_traj = repository->getDataObjectByUuid<RESQML2_NS::WellboreTrajectoryRepresentation>(uuid);
        auto rep = new ResqmlWellboreTrajectoryToVtkPolyData(wellbore_traj);
        nodeId_to_resqml[searchNodeByUuid(uuid)] = rep;
        return rep;
        break;
    }
    case ResqmlDataRepositoryToVtkPartitionedDataSetCollection::EntityType::WELL_FRAME:
    {
        /*
               auto wellbore_traj = repository->getDataObjectByUuid<RESQML2_NS::WellboreTrajectoryRepresentation>(uuid);
               auto rep = new ResqmlWellboreTrajectoryToVtkPolyData(wellbore_traj);
               nodeId_to_resqml[searchNodeByUuid(uuid)] = rep;
               return rep;
               */
        break;
    }
    case ResqmlDataRepositoryToVtkPartitionedDataSetCollection::EntityType::WELL_MARKER_FRAME:
    {
        auto wellbore_marker_frame = repository->getDataObjectByUuid<RESQML2_NS::WellboreMarkerFrameRepresentation>(uuid);
        auto rep = new ResqmlWellboreMarkerFrameToVtkPartitionedDataSet(wellbore_marker_frame, this->markerOrientation, this->markerSize);
        nodeId_to_resqml[searchNodeByUuid(uuid)] = rep;
        return rep;
        break;
    }
    case ResqmlDataRepositoryToVtkPartitionedDataSetCollection::EntityType::WELL_MARKER:
    {
        auto assembly = this->output->GetDataAssembly();
        const int node_parent = assembly->GetParent(searchNodeByUuid(uuid));
        nodeId_to_resqml[node_parent]->setUuid(uuid);
        auto representation = dynamic_cast<ResqmlWellboreMarkerFrameToVtkPartitionedDataSet *>(nodeId_to_resqml[node_parent]);
        representation->loadVtkObject();
        break;
    }
    case ResqmlDataRepositoryToVtkPartitionedDataSetCollection::EntityType::PROP:
    {
        auto assembly = this->output->GetDataAssembly();
        const int node_parent = assembly->GetParent(searchNodeByUuid(uuid));
        nodeId_to_resqml[node_parent]->addDataArray(uuid);
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

vtkPartitionedDataSetCollection *ResqmlDataRepositoryToVtkPartitionedDataSetCollection::getVtkPartionedDatasSetCollection()
{

    for (unsigned int index_partitioned = 0; index_partitioned < this->output->GetNumberOfPartitionedDataSets(); index_partitioned++)
    {
        this->output->RemovePartitionedDataSet(index_partitioned);
    }
	unsigned int index = 0;
    for (const int node_selection : this->current_selection)
    {
        ResqmlAbstractRepresentationToVtkDataset *rep = nullptr;
        if (this->nodeId_to_resqml.find(node_selection) != this->nodeId_to_resqml.end())
        {
            rep = this->nodeId_to_resqml[node_selection];
        }
        else
        {
            rep = loadToVtk(nodeId_to_uuid[node_selection], nodeId_to_EntityType[node_selection]);
        }
        if (rep)
        {
            this->output->SetPartitionedDataSet(index, rep->getOutput());
            this->output->GetMetaData(index++)->Set(vtkCompositeDataSet::NAME(), this->output->GetDataAssembly()->GetNodeName(node_selection));
        }
    }

    for (const int old_selection : this->old_selection)
    {
        if (this->nodeId_to_resqml.find(old_selection) != this->nodeId_to_resqml.end())
        {
            this->nodeId_to_resqml.erase(old_selection); // erase object
        }
    }

    this->output->Modified();
    return this->output;
}

int ResqmlDataRepositoryToVtkPartitionedDataSetCollection::searchNodeByUuid(std::string uuid)
{
    for (auto it = this->nodeId_to_uuid.begin(); it != nodeId_to_uuid.end(); ++it)
        if (it->second == uuid)
            return it->first;

    return -1;
}
