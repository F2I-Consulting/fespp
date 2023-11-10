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

/*
Documentation:
==============

VtkAssembly => TreeView:
    - each node required 2 attributes:
        - id = uuid of resqml/witsml object
        - label = name to display in TreeView
        - type = type
*/
#include "ResqmlDataRepositoryToVtkPartitionedDataSetCollection.h"

#include <algorithm>
#include <vector>
#include <set>
#include <list>
#include <regex>
#include <numeric>

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
#include <fesapi/resqml2/WellboreFeature.h>
#include <fesapi/witsml2_1/WellboreCompletion.h>
#include <fesapi/witsml2_1/WellCompletion.h>

#ifdef WITH_ETP_SSL
#include <thread>

#include <fetpapi/etp/fesapi/FesapiHdfProxy.h>

#include <fetpapi/etp/ProtocolHandlers/DataspaceHandlers.h>
#include <fetpapi/etp/ProtocolHandlers/DiscoveryHandlers.h>
#include <fetpapi/etp/ProtocolHandlers/StoreHandlers.h>
#include <fetpapi/etp/ProtocolHandlers/DataArrayHandlers.h>
#endif
#include <fesapi/common/EpcDocument.h>

#include "ResqmlIjkGridToVtkExplicitStructuredGrid.h"
#include "ResqmlIjkGridSubRepToVtkExplicitStructuredGrid.h"
#include "ResqmlGrid2dToVtkStructuredGrid.h"
#include "ResqmlPolylineToVtkPolyData.h"
#include "ResqmlTriangulatedSetToVtkPartitionedDataSet.h"
#include "ResqmlUnstructuredGridToVtkUnstructuredGrid.h"
#include "ResqmlUnstructuredGridSubRepToVtkUnstructuredGrid.h"
#include "ResqmlWellboreTrajectoryToVtkPolyData.h"
#include "ResqmlWellboreMarkerFrameToVtkPartitionedDataSet.h"
#include "ResqmlWellboreFrameToVtkPartitionedDataSet.h"
#include "WitsmlWellboreCompletionToVtkPartitionedDataSet.h"
#include "WitsmlWellboreCompletionPerforationToVtkPolyData.h"

ResqmlDataRepositoryToVtkPartitionedDataSetCollection::ResqmlDataRepositoryToVtkPartitionedDataSetCollection() : markerOrientation(false),
                                                                                                                 markerSize(10),
                                                                                                                 repository(new common::DataObjectRepository()),
                                                                                                                 output(vtkSmartPointer<vtkPartitionedDataSetCollection>::New()),
                                                                                                                 nodeId_to_mapper(),
                                                                                                                 current_selection(),
                                                                                                                 old_selection()
{
    auto assembly = vtkSmartPointer<vtkDataAssembly>::New();
    assembly->SetRootNodeName("data");
    output->SetDataAssembly(assembly);
    times_step.clear();
}

ResqmlDataRepositoryToVtkPartitionedDataSetCollection::~ResqmlDataRepositoryToVtkPartitionedDataSetCollection()
{
    delete repository;
    for (const auto &keyVal : nodeId_to_mapper)
    {
        delete keyVal.second;
    }
}

//----------------------------------------------------------------------------
std::vector<std::string> ResqmlDataRepositoryToVtkPartitionedDataSetCollection::connect(const std::string &etp_url, const std::string &data_partition, const std::string &auth_connection)
{
    std::vector<std::string> result;
#ifdef WITH_ETP_SSL
    boost::uuids::random_generator gen;
    ETP_NS::InitializationParameters initializationParams(gen(), etp_url);

    std::map<std::string, std::string> additionalHandshakeHeaderFields = {{"data-partition-id", data_partition}};
    if (etp_url.find("ws://") == 0)
    {
        session = ETP_NS::ClientSessionLaunchers::createWsClientSession(&initializationParams, auth_connection, additionalHandshakeHeaderFields);
    }
    else
    {
        session = ETP_NS::ClientSessionLaunchers::createWssClientSession(&initializationParams, auth_connection, additionalHandshakeHeaderFields);
    }
    try
    {
        session->setDataspaceProtocolHandlers(std::make_shared<ETP_NS::DataspaceHandlers>(session.get()));
    }
    catch (const std::exception &e)
    {
        // vtkErrorMacro(<< " setDataspaceProtocolHandlers: " << e.what());
    }
    try
    {
        session->setDiscoveryProtocolHandlers(std::make_shared<ETP_NS::DiscoveryHandlers>(session.get()));
    }
    catch (const std::exception &e)
    {
        // vtkErrorMacro(<< "setDiscoveryProtocolHandlers: " << e.what());
    }
    try
    {
        session->setStoreProtocolHandlers(std::make_shared<ETP_NS::StoreHandlers>(session.get()));
    }
    catch (const std::exception &e)
    {
        // vtkErrorMacro(<< "setStoreProtocolHandlers: " << e.what());
    }
    try
    {
        session->setDataArrayProtocolHandlers(std::make_shared<ETP_NS::DataArrayHandlers>(session.get()));
    }
    catch (const std::exception &e)
    {
        // vtkErrorMacro(<< "setDataArrayProtocolHandlers: " << e.what());
    }

    repository->setHdfProxyFactory(new ETP_NS::FesapiHdfProxyFactory(session.get()));

    //
    if (etp_url.find("ws://") == 0)
    {
        auto plainSession = std::dynamic_pointer_cast<ETP_NS::PlainClientSession>(session);
        std::thread sessionThread(&ETP_NS::PlainClientSession::run, plainSession);
        sessionThread.detach();
    }
    else
    {
        auto sslSession = std::dynamic_pointer_cast<ETP_NS::SslClientSession>(session);
        std::thread sessionThread(&ETP_NS::SslClientSession::run, sslSession);
        sessionThread.detach();
    }

    // Wait for the ETP session to be opened
    auto t_start = std::chrono::high_resolution_clock::now();
    while (session->isEtpSessionClosed())
    {
        if (std::chrono::duration<double, std::milli>(std::chrono::high_resolution_clock::now() - t_start).count() > 5000)
        {
            throw std::invalid_argument("Did you forget to click apply button before to connect? Time out for websocket connection" +
                                        std::to_string(std::chrono::duration<double, std::milli>(std::chrono::high_resolution_clock::now() - t_start).count()) + "ms.\n");
        }
    }

    //************ LIST DATASPACES ************
    const auto dataspaces = session->getDataspaces();

    std::transform(dataspaces.begin(), dataspaces.end(), std::back_inserter(result),
                   [](const Energistics::Etp::v12::Datatypes::Object::Dataspace &ds)
                   { return ds.uri; });

#endif
    return result;
}

//----------------------------------------------------------------------------
void ResqmlDataRepositoryToVtkPartitionedDataSetCollection::disconnect()
{
#ifdef WITH_ETP_SSL
    session->close();
#endif
}

//----------------------------------------------------------------------------
std::string ResqmlDataRepositoryToVtkPartitionedDataSetCollection::addFile(const char *fileName)
{
    COMMON_NS::EpcDocument pck(fileName);
    std::string message = pck.deserializeInto(*repository);
    pck.close();

    message += buildDataAssemblyFromDataObjectRepo(fileName);
    return message;
}

//----------------------------------------------------------------------------
std::string ResqmlDataRepositoryToVtkPartitionedDataSetCollection::addDataspace(const char *dataspace)
{
#ifdef WITH_ETP_SSL
    //************ LIST RESOURCES ************
    Energistics::Etp::v12::Datatypes::Object::ContextInfo ctxInfo;
    ctxInfo.uri = dataspace;
    ctxInfo.depth = 0;
    ctxInfo.navigableEdges = Energistics::Etp::v12::Datatypes::Object::RelationshipKind::Both;
    ctxInfo.includeSecondaryTargets = false;
    ctxInfo.includeSecondarySources = false;
    const auto resources = session->getResources(ctxInfo, Energistics::Etp::v12::Datatypes::Object::ContextScopeKind::targets);

    //************ GET ALL DATAOBJECTS ************
    repository->setHdfProxyFactory(new ETP_NS::FesapiHdfProxyFactory(session.get()));
    if (!resources.empty())
    {
        std::map<std::string, std::string> query;
        for (size_t i = 0; i < resources.size(); ++i)
        {
            query[std::to_string(i)] = resources[i].uri;
        }
        const auto dataobjects = session->getDataObjects(query);
        for (auto &datoObject : dataobjects)
        {
            repository->addOrReplaceGsoapProxy(datoObject.second.data,
                                               ETP_NS::EtpHelpers::getDataObjectType(datoObject.second.resource.uri),
                                               ETP_NS::EtpHelpers::getDataspaceUri(datoObject.second.resource.uri));
        }
    }
    else
    {
        vtkOutputWindowDisplayWarningText(("There is no dataobject in the dataspace : " + std::string(dataspace) + "\n").c_str());
    }
#endif
    return buildDataAssemblyFromDataObjectRepo("");
}

namespace
{
    auto lexicographicalComparison = [](const COMMON_NS::AbstractObject *a, const COMMON_NS::AbstractObject *b) -> bool
    {
        return a->getTitle().compare(b->getTitle()) < 0;
    };

    template <typename T>
    void sortAndAdd(std::vector<T> source, std::vector<RESQML2_NS::AbstractRepresentation const *> &dest)
    {
        std::sort(source.begin(), source.end(), lexicographicalComparison);
        std::move(source.begin(), source.end(), std::inserter(dest, dest.end()));
    }
}

std::string ResqmlDataRepositoryToVtkPartitionedDataSetCollection::buildDataAssemblyFromDataObjectRepo(const char *fileName)
{
    std::vector<RESQML2_NS::AbstractRepresentation const *> allReps;

    // create vtkDataAssembly: create treeView in property panel
    sortAndAdd(repository->getHorizonGrid2dRepresentationSet(), allReps);
    sortAndAdd(repository->getIjkGridRepresentationSet(), allReps);
    sortAndAdd(repository->getAllPolylineSetRepresentationSet(), allReps);
    sortAndAdd(repository->getAllTriangulatedSetRepresentationSet(), allReps);
    sortAndAdd(repository->getUnstructuredGridRepresentationSet(), allReps);

    // See https://stackoverflow.com/questions/15347123/how-to-construct-a-stdstring-from-a-stdvectorstring
    std::string message = std::accumulate(std::begin(allReps), std::end(allReps), std::string{},
                                          [&](std::string &message, RESQML2_NS::AbstractRepresentation const *rep)
                                          {
                                              return message += searchRepresentations(rep);
                                          });
    // get WellboreTrajectory
    message += searchWellboreTrajectory(fileName);

    // get TimeSeries
    message += searchTimeSeries(fileName);

    return message;
}

std::string ResqmlDataRepositoryToVtkPartitionedDataSetCollection::searchRepresentations(resqml2::AbstractRepresentation const *representation, int idNode)
{
    std::string result;
    vtkDataAssembly *data_assembly = output->GetDataAssembly();

    if (representation->isPartial())
    {
        // check if it has already been added
        // not exist => not loaded
        if (data_assembly->FindFirstNodeWithName(("_" + representation->getUuid()).c_str()) == -1)
        {
            return "Partial representation with UUID \"" + representation->getUuid() + "\" is not loaded.\n";
        } /******* TODO ********/ // exist but not the same type ?
    }
    else
    {
        // The leading underscore is forced by VTK which does not support a node name starting with a digit (probably because it is a QNAME).
        const std::string nodeName = "_" + representation->getUuid();
        const int existingNodeId = output->GetDataAssembly()->FindFirstNodeWithName(nodeName.c_str());
        if (existingNodeId == -1)
        {
            idNode = data_assembly->AddNode(nodeName.c_str(), idNode);

            auto const *subrep = dynamic_cast<RESQML2_NS::SubRepresentation const *>(representation);
            const std::string representationVtkValidName = subrep == nullptr
                                                               ? vtkDataAssembly::MakeValidNodeName((representation->getXmlTag() + "_" + representation->getTitle()).c_str())
                                                               : vtkDataAssembly::MakeValidNodeName((representation->getXmlTag() + "_" + subrep->getSupportingRepresentation(0)->getTitle() + "_" + representation->getTitle()).c_str());
            data_assembly->SetAttribute(idNode, "label", representationVtkValidName.c_str());
            data_assembly->SetAttribute(idNode, "type", std::to_string(static_cast<int>(TreeViewNodeType::Representation)).c_str());
        }
        else
        {
            idNode = existingNodeId;
        }
    }

    // add sub representation with properties (only for ijkGrid and unstructured grid)
    if (dynamic_cast<RESQML2_NS::AbstractIjkGridRepresentation const *>(representation) != nullptr ||
        dynamic_cast<RESQML2_NS::UnstructuredGridRepresentation const *>(representation) != nullptr)
    {
        result += searchSubRepresentation(representation, data_assembly, idNode);
    }

    // add properties to representation
    result += searchProperties(representation, data_assembly, idNode);

    return result;
}

std::string ResqmlDataRepositoryToVtkPartitionedDataSetCollection::searchSubRepresentation(RESQML2_NS::AbstractRepresentation const *representation, vtkDataAssembly *data_assembly, int node_parent)
{
    try
    {
        auto subRepresentationSet = representation->getSubRepresentationSet();
        std::sort(subRepresentationSet.begin(), subRepresentationSet.end(), lexicographicalComparison);

        std::string message = std::accumulate(std::begin(subRepresentationSet), std::end(subRepresentationSet), std::string{},
                                              [&](std::string &message, RESQML2_NS::SubRepresentation *b)
                                              {
                                                  return message += searchRepresentations(b, data_assembly->GetParent(node_parent));
                                              });
    }
    catch (const std::exception &e)
    {
        return "Exception in FESAPI when calling getSubRepresentationSet for uuid : " + representation->getUuid() + " : " + e.what() + ".\n";
    }

    return "";
}

std::string ResqmlDataRepositoryToVtkPartitionedDataSetCollection::searchProperties(RESQML2_NS::AbstractRepresentation const *representation, vtkDataAssembly *data_assembly, int node_parent)
{
    try
    {
        auto valuesPropertySet = representation->getValuesPropertySet();
        std::sort(valuesPropertySet.begin(), valuesPropertySet.end(), lexicographicalComparison);
        // property
        for (auto const *property : valuesPropertySet)
        {
            const std::string propertyVtkValidName = vtkDataAssembly::MakeValidNodeName((property->getXmlTag() + '_' + property->getTitle()).c_str());

            if (output->GetDataAssembly()->FindFirstNodeWithName(("_" + property->getUuid()).c_str()) == -1)
            { // verify uuid exist in treeview
                int property_idNode = data_assembly->AddNode(("_" + property->getUuid()).c_str(), node_parent);
                data_assembly->SetAttribute(property_idNode, "label", propertyVtkValidName.c_str());
                data_assembly->SetAttribute(property_idNode, "type", std::to_string(static_cast<int>(TreeViewNodeType::Properties)).c_str());
            }
        }
    }
    catch (const std::exception &e)
    {
        return "Exception in FESAPI when calling getValuesPropertySet with representation uuid: " + representation->getUuid() + " : " + e.what() + ".\n";
    }

    return "";
}

std::string ResqmlDataRepositoryToVtkPartitionedDataSetCollection::searchWellboreTrajectory(const std::string &fileName)
{
    std::string result;

    vtkDataAssembly *data_assembly = output->GetDataAssembly();

    int idNode = 0; // 0 is root's id

    for (auto *wellboreTrajectory : repository->getWellboreTrajectoryRepresentationSet())
    {
        if (output->GetDataAssembly()->FindFirstNodeWithName(("_" + wellboreTrajectory->getUuid()).c_str()) == -1)
        { // verify uuid exist in treeview
            if (wellboreTrajectory->isPartial())
            {
                // check if it has already been added
                const std::string name_partial_representation = vtkDataAssembly::MakeValidNodeName((wellboreTrajectory->getXmlTag() + "_" + wellboreTrajectory->getTitle()).c_str());
                bool uuid_exist = data_assembly->FindFirstNodeWithName(("_" + name_partial_representation).c_str()) != -1;
                // not exist => not loaded
                if (!uuid_exist)
                {
                    result = result + " Partial UUID: (" + wellboreTrajectory->getUuid() + ") is not loaded \n";
                    continue;
                } /******* TODO ********/ // exist but not the same type ?
            }
            else
            {
                // wellboreTrajectory->setTitle(vtkDataAssembly::MakeValidNodeName((wellboreTrajectory->getXmlTag() + '_' + wellboreTrajectory->getTitle()).c_str()));
                const std::string wellboreTrajectoryVtkValidName = vtkDataAssembly::MakeValidNodeName((wellboreTrajectory->getXmlTag() + '_' + wellboreTrajectory->getTitle()).c_str());
                idNode = data_assembly->AddNode(("_" + wellboreTrajectory->getUuid()).c_str(), 0 /* add to root*/);
                data_assembly->SetAttribute(idNode, "label", wellboreTrajectoryVtkValidName.c_str());
                data_assembly->SetAttribute(idNode, "type", std::to_string(static_cast<int>(TreeViewNodeType::WellboreTrajectory)).c_str());

                /* to delete
                data_assembly->SetAttribute(idNode, "is_checkable", 0);
                */
            }
        }
        // wellboreFrame
        for (auto *wellboreFrame : wellboreTrajectory->getWellboreFrameRepresentationSet())
        {
            if (output->GetDataAssembly()->FindFirstNodeWithName(("_" + wellboreFrame->getUuid()).c_str()) == -1)
            { // verify uuid exist in treeview
                auto *wellboreMarkerFrame = dynamic_cast<RESQML2_NS::WellboreMarkerFrameRepresentation const *>(wellboreFrame);
                if (wellboreMarkerFrame == nullptr)
                { // WellboreFrame
                    wellboreFrame->setTitle(vtkDataAssembly::MakeValidNodeName((wellboreFrame->getXmlTag() + '_' + wellboreFrame->getTitle()).c_str()));
                    const std::string wellboreFrameVtkValidName = vtkDataAssembly::MakeValidNodeName((wellboreFrame->getXmlTag() + '_' + wellboreFrame->getTitle()).c_str());
                    int frame_idNode = data_assembly->AddNode(("_" + wellboreFrame->getUuid()).c_str(), 0 /* add to root*/);
                    data_assembly->SetAttribute(frame_idNode, "label", wellboreFrameVtkValidName.c_str());
                    data_assembly->SetAttribute(frame_idNode, "traj", idNode);
                    data_assembly->SetAttribute(frame_idNode, "type", std::to_string(static_cast<int>(TreeViewNodeType::WellboreFrame)).c_str());
                    for (auto *property : wellboreFrame->getValuesPropertySet())
                    {
                        const std::string propertyVtkValidName = vtkDataAssembly::MakeValidNodeName((property->getXmlTag() + '_' + property->getTitle()).c_str());
                        int property_idNode = data_assembly->AddNode(("_" + property->getUuid()).c_str(), frame_idNode);
                        data_assembly->SetAttribute(property_idNode, "label", propertyVtkValidName.c_str());
                        data_assembly->SetAttribute(property_idNode, "traj", idNode);
                        data_assembly->SetAttribute(property_idNode, "type", std::to_string(static_cast<int>(TreeViewNodeType::WellboreChannel)).c_str());
                    }
                }
                else
                { // WellboreMarkerFrame
                    const std::string wellboreFrameVtkValidName = vtkDataAssembly::MakeValidNodeName((wellboreFrame->getXmlTag() + '_' + wellboreFrame->getTitle()).c_str());
                    int markerFrame_idNode = data_assembly->AddNode(("_" + wellboreFrame->getUuid()).c_str(), 0 /* add to root*/);
                    data_assembly->SetAttribute(markerFrame_idNode, "label", wellboreFrameVtkValidName.c_str());
                    data_assembly->SetAttribute(markerFrame_idNode, "traj", idNode);
                    data_assembly->SetAttribute(markerFrame_idNode, "type", std::to_string(static_cast<int>(TreeViewNodeType::WellboreFrame)).c_str());
                    for (auto *wellboreMarker : wellboreMarkerFrame->getWellboreMarkerSet())
                    {
                        const std::string wellboreMarkerVtkValidName = vtkDataAssembly::MakeValidNodeName((wellboreMarker->getXmlTag() + '_' + wellboreMarker->getTitle()).c_str());
                        int marker_idNode = data_assembly->AddNode(("_" + wellboreMarker->getUuid()).c_str(), markerFrame_idNode);
                        data_assembly->SetAttribute(marker_idNode, "label", wellboreMarkerVtkValidName.c_str());
                        data_assembly->SetAttribute(marker_idNode, "traj", idNode);
                        data_assembly->SetAttribute(marker_idNode, "type", std::to_string(static_cast<int>(TreeViewNodeType::WellboreMarker)).c_str());
                    }
                }
            }
        }
        // Wellbore Completion
        const auto *wellboreFeature = dynamic_cast<RESQML2_NS::WellboreFeature *>(wellboreTrajectory->getInterpretation()->getInterpretedFeature());
        if (wellboreFeature != nullptr)
        {
            const auto *witsmlWellbore = wellboreFeature->getWitsmlWellbore();
            if (witsmlWellbore != nullptr)
            {
                for (const auto *wellbore_completion : witsmlWellbore->getWellboreCompletionSet())
                {
                    const std::string wellboreCompletionVtkValidName = vtkDataAssembly::MakeValidNodeName((wellbore_completion->getXmlTag() + '_' + wellbore_completion->getTitle()).c_str());
                    int idNode_completion = data_assembly->AddNode(("_" + wellbore_completion->getUuid()).c_str(), 0);
                    data_assembly->SetAttribute(idNode_completion, "label", wellboreCompletionVtkValidName.c_str());
                    data_assembly->SetAttribute(idNode_completion, "type", std::to_string(static_cast<int>(TreeViewNodeType::WellboreCompletion)).c_str());
                    // Iterate over the perforations.
                    for (uint64_t perforationIndex = 0; perforationIndex < wellbore_completion->getConnectionCount(WITSML2_1_NS::WellboreCompletion::WellReservoirConnectionType::PERFORATION); ++perforationIndex)
                    {
                        std::string perforation_name = "Perfo";
                        auto extraMetadatas = wellbore_completion->getConnectionExtraMetadata(WITSML2_1_NS::WellboreCompletion::WellReservoirConnectionType::PERFORATION, perforationIndex, "Petrel:Name0");
                        // arbitrarily select the first event name as perforation name
                        if (extraMetadatas.size() > 0)
                        {
                            perforation_name += "_" + extraMetadatas[0];
                        }
                        else
                        {
                            extraMetadatas = wellbore_completion->getConnectionExtraMetadata(WITSML2_1_NS::WellboreCompletion::WellReservoirConnectionType::PERFORATION, perforationIndex, "Sismage-CIG:Name");
                            if (extraMetadatas.size() > 0)
                            {
                                perforation_name += "_" + extraMetadatas[0];
                            }
                            else
                            {
                                perforation_name += "_" + wellbore_completion->getConnectionUid(WITSML2_1_NS::WellboreCompletion::WellReservoirConnectionType::PERFORATION, perforationIndex);
                            }
                        }
                        const std::string perforation_VTK_validName = vtkDataAssembly::MakeValidNodeName((perforation_name).c_str());
                        int idNode_perfo = data_assembly->AddNode(vtkDataAssembly::MakeValidNodeName(("_" + wellbore_completion->getUuid() + "_" + wellbore_completion->getConnectionUid(WITSML2_1_NS::WellboreCompletion::WellReservoirConnectionType::PERFORATION, perforationIndex)).c_str()).c_str(), idNode_completion);
                        data_assembly->SetAttribute(idNode_perfo, "label", perforation_VTK_validName.c_str());
                        int nodeType = static_cast<int>(TreeViewNodeType::Perforation);
                        data_assembly->SetAttribute(idNode_perfo, "type", std::to_string(nodeType).c_str());
                        data_assembly->SetAttribute(idNode_perfo, "connection", wellbore_completion->getConnectionUid(WITSML2_1_NS::WellboreCompletion::WellReservoirConnectionType::PERFORATION, perforationIndex).c_str());
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
    auto *assembly = output->GetDataAssembly();
    std::vector<EML2_NS::TimeSeries *> timeSeriesSet;
    try
    {
        timeSeriesSet = repository->getTimeSeriesSet();
    }
    catch (const std::exception &e)
    {
        return_message = return_message + "Exception in FESAPI when calling getTimeSeriesSet with file: " + fileName + " : " + e.what();
    }

    /****
     *  change property parent to times serie parent
     ****/
    for (auto const *timeSeries : timeSeriesSet)
    {
        // get properties link to Times series
        try
        {
            auto propSeries = timeSeries->getPropertySet();

            std::map<std::string, std::vector<int>> propertyName_to_nodeSet;
            for (auto const *prop : propSeries)
            {
                if (prop->getXmlTag() == RESQML2_NS::ContinuousProperty::XML_TAG ||
                    prop->getXmlTag() == RESQML2_NS::DiscreteProperty::XML_TAG)
                {
                    auto node_id = (output->GetDataAssembly()->FindFirstNodeWithName(("_" + prop->getUuid()).c_str()));
                    if (node_id == -1)
                    {
                        return_message = return_message + "The property " + prop->getUuid() + " is not supported and consequently cannot be associated to its time series.\n";
                        continue;
                    }
                    else
                    {
                        // same node parent else not supported
                        const int nodeParent = assembly->GetParent(node_id);
                        if (nodeParent != -1)
                        {
                            propertyName_to_nodeSet[prop->getTitle()].push_back(node_id);
                            const size_t timeIndexInTimeSeries = timeSeries->getTimestampIndex(prop->getSingleTimestamp());
                            times_step.push_back(timeIndexInTimeSeries);
                            timeSeries_uuid_and_title_to_index_and_properties_uuid[timeSeries->getUuid()][vtkDataAssembly::MakeValidNodeName((timeSeries->getXmlTag() + '_' + prop->getTitle()).c_str())][timeIndexInTimeSeries] = prop->getUuid();
                        }
                        else
                        {
                            return_message = return_message + "The properties of time series " + timeSeries->getUuid() + " aren't parent and is not supported.\n";
                            continue;
                        }
                    }
                }
            }
            // erase duplicate Index
            sort(times_step.begin(), times_step.end());
            times_step.erase(unique(times_step.begin(), times_step.end()), times_step.end());

            for (const auto &myPair : propertyName_to_nodeSet)
            {
                std::vector<int> property_node_set = myPair.second;

                int nodeParent = -1;
                // erase property add to treeview for group by TimeSerie
                for (auto node : property_node_set)
                {
                    nodeParent = output->GetDataAssembly()->GetParent(node);
                    output->GetDataAssembly()->RemoveNode(node);
                }
                std::string name = vtkDataAssembly::MakeValidNodeName((timeSeries->getXmlTag() + '_' + myPair.first).c_str());
                auto times_serie_node_id = output->GetDataAssembly()->AddNode(("_" + timeSeries->getUuid() + name).c_str(), nodeParent);
                output->GetDataAssembly()->SetAttribute(times_serie_node_id, "label", name.c_str());
                output->GetDataAssembly()->SetAttribute(times_serie_node_id, "type", std::to_string(static_cast<int>(TreeViewNodeType::TimeSeries)).c_str());
            }
        }
        catch (const std::exception &e)
        {
            return_message = return_message + "Exception in FESAPI when calling getPropertySet with file: " + fileName + " : " + e.what();
        }
    }

    return return_message;
}

std::string ResqmlDataRepositoryToVtkPartitionedDataSetCollection::selectNodeId(int node)
{
    if (node != 0)
    {
        this->selectNodeIdParent(node);

        this->current_selection.insert(node);
        this->old_selection.erase(node);
    }
    this->selectNodeIdChildren(node);

    return "";
}

void ResqmlDataRepositoryToVtkPartitionedDataSetCollection::selectNodeIdParent(int node)
{
    auto const *assembly = this->output->GetDataAssembly();

    if (assembly->GetParent(node) > 0)
    {
        this->current_selection.insert(assembly->GetParent(node));
        this->old_selection.erase(assembly->GetParent(node));
        this->selectNodeIdParent(assembly->GetParent(node));
    }
}
void ResqmlDataRepositoryToVtkPartitionedDataSetCollection::selectNodeIdChildren(int node)
{
    auto const *assembly = this->output->GetDataAssembly();

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

void ResqmlDataRepositoryToVtkPartitionedDataSetCollection::initMapper(const TreeViewNodeType type, const int node_id /*const std::string& uuid*/, const int nbProcess, const int processId)
{
    CommonAbstractObjectToVtkPartitionedDataSet *rep = nullptr;
    const std::string uuid = std::string(output->GetDataAssembly()->GetNodeName(node_id)).substr(1);

    try
    {
        if (TreeViewNodeType::Representation == type || TreeViewNodeType::WellboreTrajectory == type || TreeViewNodeType::WellboreMarker == type || TreeViewNodeType::WellboreFrame == type || TreeViewNodeType::WellboreCompletion == type || TreeViewNodeType::Perforation == type)
        {

            COMMON_NS::AbstractObject *const result = repository->getDataObjectByUuid(uuid);
            if (TreeViewNodeType::Representation == type)
            {

                if (dynamic_cast<RESQML2_NS::AbstractIjkGridRepresentation *>(result) != nullptr)
                {
                    rep = new ResqmlIjkGridToVtkExplicitStructuredGrid(static_cast<RESQML2_NS::AbstractIjkGridRepresentation *>(result), processId, nbProcess);
                }
                else if (dynamic_cast<RESQML2_NS::Grid2dRepresentation *>(result) != nullptr)
                {
                    rep = new ResqmlGrid2dToVtkStructuredGrid(static_cast<RESQML2_NS::Grid2dRepresentation *>(result));
                }
                else if (dynamic_cast<RESQML2_NS::TriangulatedSetRepresentation *>(result) != nullptr)
                {
                    rep = new ResqmlTriangulatedSetToVtkPartitionedDataSet(static_cast<RESQML2_NS::TriangulatedSetRepresentation *>(result));
                }
                else if (dynamic_cast<RESQML2_NS::PolylineSetRepresentation *>(result) != nullptr)
                {
                    rep = new ResqmlPolylineToVtkPolyData(static_cast<RESQML2_NS::PolylineSetRepresentation *>(result));
                }
                else if (dynamic_cast<RESQML2_NS::UnstructuredGridRepresentation *>(result) != nullptr)
                {
                    rep = new ResqmlUnstructuredGridToVtkUnstructuredGrid(static_cast<RESQML2_NS::UnstructuredGridRepresentation *>(result));
                }
                else if (dynamic_cast<RESQML2_NS::SubRepresentation *>(result) != nullptr)
                {
                    RESQML2_NS::SubRepresentation *subRep = static_cast<RESQML2_NS::SubRepresentation *>(result);

                    if (dynamic_cast<RESQML2_NS::AbstractIjkGridRepresentation *>(subRep->getSupportingRepresentation(0)) != nullptr)
                    {
                        auto *supportingGrid = static_cast<RESQML2_NS::AbstractIjkGridRepresentation *>(subRep->getSupportingRepresentation(0));
                        if (this->nodeId_to_mapper.find(output->GetDataAssembly()->FindFirstNodeWithName(("_" + supportingGrid->getUuid()).c_str())) == this->nodeId_to_mapper.end())
                        {
                            this->nodeId_to_mapper[output->GetDataAssembly()->FindFirstNodeWithName(("_" + supportingGrid->getUuid()).c_str())] = new ResqmlIjkGridToVtkExplicitStructuredGrid(supportingGrid);
                        }
                        rep = new ResqmlIjkGridSubRepToVtkExplicitStructuredGrid(subRep, dynamic_cast<ResqmlIjkGridToVtkExplicitStructuredGrid *>(this->nodeId_to_mapper[output->GetDataAssembly()->FindFirstNodeWithName(("_" + supportingGrid->getUuid()).c_str())]));
                    }
                    else if (dynamic_cast<RESQML2_NS::UnstructuredGridRepresentation *>(subRep->getSupportingRepresentation(0)) != nullptr)
                    {
                        auto *supportingGrid = static_cast<RESQML2_NS::UnstructuredGridRepresentation *>(subRep->getSupportingRepresentation(0));
                        if (this->nodeId_to_mapper.find(output->GetDataAssembly()->FindFirstNodeWithName(("_" + supportingGrid->getUuid()).c_str())) == this->nodeId_to_mapper.end())
                        {
                            this->nodeId_to_mapper[output->GetDataAssembly()->FindFirstNodeWithName(("_" + supportingGrid->getUuid()).c_str())] = new ResqmlUnstructuredGridToVtkUnstructuredGrid(supportingGrid);
                        }
                        rep = new ResqmlUnstructuredGridSubRepToVtkUnstructuredGrid(subRep, dynamic_cast<ResqmlUnstructuredGridToVtkUnstructuredGrid *>(this->nodeId_to_mapper[output->GetDataAssembly()->FindFirstNodeWithName(("_" + supportingGrid->getUuid()).c_str())]));
                    }
                }
            }
            else if (TreeViewNodeType::WellboreTrajectory == type)
            {

                if (dynamic_cast<RESQML2_NS::WellboreTrajectoryRepresentation *>(result) != nullptr)
                {
                    rep = new ResqmlWellboreTrajectoryToVtkPolyData(static_cast<RESQML2_NS::WellboreTrajectoryRepresentation *>(result));
                }
            }
            else if (TreeViewNodeType::WellboreCompletion == type)
            {
                if (dynamic_cast<witsml2_1::WellboreCompletion *>(result) != nullptr)
                {
                    rep = new WitsmlWellboreCompletionToVtkPartitionedDataSet(static_cast<witsml2_1::WellboreCompletion *>(result));
                }
            }
            else if (TreeViewNodeType::Perforation == type)
            {
                const int node_parent = output->GetDataAssembly()->GetParent(node_id);
                if (dynamic_cast<WitsmlWellboreCompletionToVtkPartitionedDataSet*>(nodeId_to_mapper[node_parent]) != nullptr)
                {
                    const char* value;
                    output->GetDataAssembly()->GetAttribute(node_id, "connection", value);
                    const char* name;
                    output->GetDataAssembly()->GetAttribute(node_id, "label", name);
                    (static_cast<WitsmlWellboreCompletionToVtkPartitionedDataSet*>(nodeId_to_mapper[node_parent]))->addPerforation(value, name);
                    for (auto* perforation : (static_cast<WitsmlWellboreCompletionToVtkPartitionedDataSet*>(nodeId_to_mapper[node_parent]))->getPerforations()) {
                        if (perforation->getTitle() == std::string(name) && perforation->getConnectionuid() == std::string(value)) {
                            vtkOutputWindowDisplayText((std::string(name) + std::string(value)).c_str());
                            rep = perforation;
                        }
                    }
                }
            }
            else if (TreeViewNodeType::WellboreMarker == type)
            {
                if (dynamic_cast<RESQML2_NS::WellboreMarkerFrameRepresentation *>(result) != nullptr)
                {
                    rep = new ResqmlWellboreMarkerFrameToVtkPartitionedDataSet(static_cast<RESQML2_NS::WellboreMarkerFrameRepresentation *>(result));
                }
            }
            else if (TreeViewNodeType::WellboreFrame == type)
            {
                if (dynamic_cast<RESQML2_NS::WellboreFrameRepresentation *>(result) != nullptr)
                {
                    rep = new ResqmlWellboreFrameToVtkPartitionedDataSet(static_cast<RESQML2_NS::WellboreFrameRepresentation *>(result));
                }
                else
                {
                    return;
                }
            }
            else
            {
                return;
            }
            this->nodeId_to_mapper[node_id] = rep;
            return;
        }
    }
    catch (const std::exception &e)
    {
        vtkOutputWindowDisplayErrorText(("Error when initialize uuid: " + uuid + "\n" + e.what()).c_str());
    }
}

void ResqmlDataRepositoryToVtkPartitionedDataSetCollection::loadMapper(const TreeViewNodeType type, const int node_id /*const std::string& uuid*/, double time)
{
    auto const* assembly = this->output->GetDataAssembly();
    const std::string uuid = std::string(output->GetDataAssembly()->GetNodeName(node_id)).substr(1);
 /*   if (type == TreeViewNodeType::Perforation)
    {
        const int node_parent = assembly->GetParent(node_id);
        if (dynamic_cast<WitsmlWellboreCompletionToVtkPartitionedDataSet*>(nodeId_to_mapper[node_parent]) != nullptr)
        {
            const char* value;
            assembly->GetAttribute(node_id, "connection", value);
            const char* name;
            assembly->GetAttribute(node_id, "label", name);
            (static_cast<WitsmlWellboreCompletionToVtkPartitionedDataSet*>(nodeId_to_mapper[node_parent]))->addPerforation(value, name);
            for (auto* perforation : (static_cast<WitsmlWellboreCompletionToVtkPartitionedDataSet*>(nodeId_to_mapper[node_parent]))->getPerforations()) {
                if (perforation->getTitle() == std::string(name) && perforation->getConnectionuid() == std::string(value)) {
                    nodeId_to_mapper[node_id] = perforation; 
                    return;
                }
            }
        }
    }
    else */if (type == TreeViewNodeType::TimeSeries)
    {
        try
        {
            std::string ts_uuid = uuid.substr(0, 36);
            std::string node_name = uuid.substr(36);

            auto const *assembly = this->output->GetDataAssembly();
            const int node_parent = assembly->GetParent(output->GetDataAssembly()->FindFirstNodeWithName(("_" + uuid).c_str()));
            if (nodeId_to_mapper[node_parent])
            {
                static_cast<ResqmlAbstractRepresentationToVtkPartitionedDataSet *>(nodeId_to_mapper[node_parent])->addDataArray(timeSeries_uuid_and_title_to_index_and_properties_uuid[ts_uuid][node_name][time]);
            }
            return;
        }
        catch (const std::exception &e)
        {
            vtkOutputWindowDisplayErrorText(("Error when load Time Series property marker uuid: " + uuid + "\n" + e.what()).c_str());
            return;
        }
    }
    else
    {

        COMMON_NS::AbstractObject *const result = repository->getDataObjectByUuid(uuid);

        try
        { // load Wellbore Marker
            if (dynamic_cast<RESQML2_NS::WellboreMarker *>(result) != nullptr)
            {
                auto const *assembly = this->output->GetDataAssembly();
                const int node_parent = assembly->GetParent(output->GetDataAssembly()->FindFirstNodeWithName(("_" + uuid).c_str()));
                static_cast<ResqmlWellboreMarkerFrameToVtkPartitionedDataSet *>(nodeId_to_mapper[node_parent])->addMarker(uuid, markerOrientation, markerSize);
                return;
            }
        }
        catch (const std::exception &e)
        {
            vtkOutputWindowDisplayErrorText(("Error when load wellbore marker uuid: " + uuid + "\n" + e.what()).c_str());
            return;
        }
        try
        { // load property
            if (dynamic_cast<RESQML2_NS::AbstractValuesProperty *>(result) != nullptr)
            {
                auto const *assembly = this->output->GetDataAssembly();
                const int node_parent = assembly->GetParent(node_id); // node_id
                if (dynamic_cast<ResqmlWellboreFrameToVtkPartitionedDataSet *>(nodeId_to_mapper[node_parent]) != nullptr)
                {
                    static_cast<ResqmlWellboreFrameToVtkPartitionedDataSet *>(nodeId_to_mapper[node_parent])->addChannel(uuid, static_cast<resqml2::AbstractValuesProperty *>(result));
                }
                else if (dynamic_cast<ResqmlTriangulatedSetToVtkPartitionedDataSet *>(nodeId_to_mapper[node_parent]) != nullptr)
                {
                    static_cast<ResqmlTriangulatedSetToVtkPartitionedDataSet *>(nodeId_to_mapper[node_parent])->addDataArray(uuid);
                }
                else
                {
                    static_cast<ResqmlAbstractRepresentationToVtkPartitionedDataSet *>(nodeId_to_mapper[node_parent])->addDataArray(uuid);
                }
                return;
            }
        }
        catch (const std::exception &e)
        {
            vtkOutputWindowDisplayErrorText(("Error when load property uuid: " + uuid + "\n" + e.what()).c_str());
            return;
        }

        try
        { // load representation

            nodeId_to_mapper[node_id]->loadVtkObject(); // node_id

            return;
        }
        catch (const std::exception &e)
        {
            vtkOutputWindowDisplayErrorText(("Error when rendering uuid: " + uuid + "\n" + e.what()).c_str());
            return;
        }
    }
}

vtkPartitionedDataSetCollection *ResqmlDataRepositoryToVtkPartitionedDataSetCollection::getVtkPartitionedDatasSetCollection(const double time, const int nbProcess, const int processId)
{
    // initialization the output (VtkPartitionedDatasSetCollection)
    vtkSmartPointer<vtkDataAssembly> tmp_assembly = vtkSmartPointer<vtkDataAssembly>::New();
    tmp_assembly->DeepCopy(this->output->GetDataAssembly());
    this->output = vtkSmartPointer<vtkPartitionedDataSetCollection>::New();
    this->output->SetDataAssembly(tmp_assembly);

    unsigned int index = 0; // index for PartionedDatasSet

    // delete unchecked object
    for (const int selection : this->old_selection)
    {
        const std::string uuid_unselect = std::string(tmp_assembly->GetNodeName(selection)).substr(1);
        if (uuid_unselect.length() > 36) // uncheck Time Series
        {                                // TimeSerie properties deselection
            std::string ts_uuid = uuid_unselect.substr(0, 36);
            std::string node_name = uuid_unselect.substr(36);

            const int node_parent = tmp_assembly->GetParent(tmp_assembly->FindFirstNodeWithName(("_" + uuid_unselect).c_str()));
            if (this->nodeId_to_mapper.find(node_parent) != this->nodeId_to_mapper.end())
            {
                static_cast<ResqmlWellboreMarkerFrameToVtkPartitionedDataSet *>(nodeId_to_mapper[node_parent])->deleteDataArray(timeSeries_uuid_and_title_to_index_and_properties_uuid[ts_uuid][node_name][time]);
            }
        }
        else
        { // other deselection
            COMMON_NS::AbstractObject *const result = repository->getDataObjectByUuid(uuid_unselect);

            if (result == nullptr)
            {
                vtkOutputWindowDisplayErrorText(("Error in deselection for uuid: " + uuid_unselect + "\n").c_str());
            }
            else
            {
                if (dynamic_cast<RESQML2_NS::AbstractValuesProperty *>(result) != nullptr)
                {
                    const int node_parent = tmp_assembly->GetParent(selection);
                    try
                    {
                        if (this->nodeId_to_mapper.find(node_parent) != this->nodeId_to_mapper.end())
                        {
                            ResqmlAbstractRepresentationToVtkPartitionedDataSet *rep = static_cast<ResqmlWellboreMarkerFrameToVtkPartitionedDataSet *>(this->nodeId_to_mapper[node_parent]);
                            rep->deleteDataArray(std::string(tmp_assembly->GetNodeName(selection)).substr(1));
                        }
                    }
                    catch (const std::exception &e)
                    {
                        vtkOutputWindowDisplayErrorText(("Error in property unload for uuid: " + uuid_unselect + "\n" + e.what()).c_str());
                    }
                }
                else if (dynamic_cast<RESQML2_NS::WellboreMarker *>(result) != nullptr)
                {
                    try
                    {
                        const int node_parent = tmp_assembly->GetParent(selection);
                        if (this->nodeId_to_mapper.find(node_parent) != this->nodeId_to_mapper.end())
                        {
                            ResqmlWellboreMarkerFrameToVtkPartitionedDataSet *rep = dynamic_cast<ResqmlWellboreMarkerFrameToVtkPartitionedDataSet *>(this->nodeId_to_mapper[node_parent]);
                            rep->removeMarker(uuid_unselect);
                        }
                    }
                    catch (const std::exception &e)
                    {
                        vtkOutputWindowDisplayErrorText(("Error in wellboremarker unload for uuid: " + uuid_unselect + "\n" + e.what()).c_str());
                    }
                }
                else if (dynamic_cast<RESQML2_NS::SubRepresentation *>(result) != nullptr)
                {
                    try
                    {
                        if (this->nodeId_to_mapper.find(selection) != this->nodeId_to_mapper.end())
                        {
                            std::string uuid_supporting_grid = "";
                            if (dynamic_cast<ResqmlUnstructuredGridSubRepToVtkUnstructuredGrid *>(this->nodeId_to_mapper[selection]) != nullptr)
                            {
                                uuid_supporting_grid = static_cast<ResqmlUnstructuredGridSubRepToVtkUnstructuredGrid *>(this->nodeId_to_mapper[selection])->unregisterToMapperSupportingGrid();
                            }
                            else if (dynamic_cast<ResqmlIjkGridSubRepToVtkExplicitStructuredGrid *>(this->nodeId_to_mapper[selection]) != nullptr)
                            {
                                uuid_supporting_grid = static_cast<ResqmlIjkGridSubRepToVtkExplicitStructuredGrid *>(this->nodeId_to_mapper[selection])->unregisterToMapperSupportingGrid();
                            }
                            // erase representation
                            this->nodeId_to_mapper.erase(selection);
                            // erase supporting grid ??
                            const int node_parent = tmp_assembly->FindFirstNodeWithName(("_" + uuid_supporting_grid).c_str());
                            if ((this->nodeId_to_mapper.find(node_parent) != this->nodeId_to_mapper.end()))
                            {
                                if (this->current_selection.find(node_parent) == this->current_selection.end())
                                {
                                    if (static_cast<ResqmlWellboreMarkerFrameToVtkPartitionedDataSet *>(this->nodeId_to_mapper[node_parent])->subRepLinkedCount() == 0)
                                    {
                                        this->nodeId_to_mapper.erase(node_parent);
                                    }
                                }
                            }
                        }
                        else
                        {
                            vtkOutputWindowDisplayErrorText(("Error in deselection for uuid: " + uuid_unselect + "\n").c_str());
                        }
                    }
                    catch (const std::exception &e)
                    {
                        vtkOutputWindowDisplayErrorText(("Error in subrepresentation unload for uuid: " + uuid_unselect + "\n" + e.what()).c_str());
                    }
                }
                else
                {
                    if (this->nodeId_to_mapper.find(selection) != this->nodeId_to_mapper.end())
                    {
                        if (static_cast<ResqmlWellboreMarkerFrameToVtkPartitionedDataSet *>(this->nodeId_to_mapper[selection])->subRepLinkedCount() == 0)
                        {
                            this->nodeId_to_mapper.erase(selection);
                        }
                    }
                }
            }
        }
    }

    // foreach selection node
    for (const int node_selection : this->current_selection)
    {
        int value_type;
        tmp_assembly->GetAttribute(node_selection, "type", value_type);
        const std::string uuid = std::string(tmp_assembly->GetNodeName(node_selection)).substr(1);
        // initialize mapper
        if (this->nodeId_to_mapper.find(node_selection) == this->nodeId_to_mapper.end())
        {
            initMapper(static_cast<TreeViewNodeType>(value_type), node_selection /*uuid*/, nbProcess, processId);
        }

        // load mapper representation
        if (this->nodeId_to_mapper.find(node_selection) != this->nodeId_to_mapper.end())
        {
            if (this->nodeId_to_mapper[node_selection]->getOutput()->GetNumberOfPartitions() < 1)
            {
                loadMapper(static_cast<TreeViewNodeType>(value_type), node_selection /*uuid*/, time);
            }
            this->output->SetPartitionedDataSet(index, this->nodeId_to_mapper[node_selection]->getOutput());
            this->output->GetMetaData(index++)->Set(vtkCompositeDataSet::NAME(), this->nodeId_to_mapper[node_selection]->getTitle() + '(' + this->nodeId_to_mapper[node_selection]->getUuid() + ')');
            //                this->output->GetMetaData(index++)->Set(vtkCompositeDataSet::NAME(), this->output->GetDataAssembly()->GetNodeName(node_selection));
        }
        else
        {
            loadMapper(static_cast<TreeViewNodeType>(value_type), node_selection /*uuid*/, time);
        }
    }

    this->output->Modified();
    return this->output;
}

void ResqmlDataRepositoryToVtkPartitionedDataSetCollection::setMarkerOrientation(bool orientation)
{
    markerOrientation = orientation;
}

void ResqmlDataRepositoryToVtkPartitionedDataSetCollection::setMarkerSize(int size)
{
    markerSize = size;
}

