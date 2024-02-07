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
    - each node required 3 attributes:
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
#include <cstdlib>
#include <iostream>

// VTK includes
#include <vtkPartitionedDataSetCollection.h>
#include <vtkPartitionedDataSet.h>
#include <vtkInformation.h>
#include <vtkDataAssembly.h>
#include <vtkDataArraySelection.h>

// FESAPI includes
#include <fesapi/common/DataObjectRepository.h>
#include <fesapi/common/EpcDocument.h>
#include <fesapi/eml2/TimeSeries.h>
#include <fesapi/resqml2/Grid2dRepresentation.h>
#include <fesapi/resqml2/AbstractFeatureInterpretation.h>
#include <fesapi/resqml2/AbstractIjkGridRepresentation.h>
#include <fesapi/resqml2/PolylineSetRepresentation.h>
#include <fesapi/resqml2/SubRepresentation.h>
#include <fesapi/resqml2/TriangulatedSetRepresentation.h>
#include <fesapi/resqml2/UnstructuredGridRepresentation.h>
#include <fesapi/resqml2/WellboreMarkerFrameRepresentation.h>
#include <fesapi/resqml2/WellboreFrameRepresentation.h>
#include <fesapi/resqml2/WellboreMarker.h>
#include <fesapi/resqml2/WellboreTrajectoryRepresentation.h>
#include <fesapi/resqml2/AbstractFeatureInterpretation.h>
#include <fesapi/resqml2/ContinuousProperty.h>
#include <fesapi/resqml2/DiscreteProperty.h>
#include <fesapi/resqml2/WellboreFeature.h>
#include <fesapi/resqml2/RepresentationSetRepresentation.h>
#include <fesapi/resqml2_0_1/PropertySet.h>
#include <fesapi/witsml2_1/WellboreCompletion.h>
#include <fesapi/witsml2_1/WellCompletion.h>
#include <fesapi/witsml2_1/Well.h>

#ifdef WITH_ETP_SSL
#include <thread>

#include <fetpapi/etp/fesapi/FesapiHdfProxy.h>

#include <fetpapi/etp/ProtocolHandlers/DataspaceHandlers.h>
#include <fetpapi/etp/ProtocolHandlers/DiscoveryHandlers.h>
#include <fetpapi/etp/ProtocolHandlers/StoreHandlers.h>
#include <fetpapi/etp/ProtocolHandlers/DataArrayHandlers.h>
#endif

#include "Mapping/ResqmlIjkGridToVtkExplicitStructuredGrid.h"
#include "Mapping/ResqmlIjkGridSubRepToVtkExplicitStructuredGrid.h"
#include "Mapping/ResqmlGrid2dToVtkStructuredGrid.h"
#include "Mapping/ResqmlPolylineToVtkPolyData.h"
#include "Mapping/ResqmlTriangulatedSetToVtkPartitionedDataSet.h"
#include "Mapping/ResqmlUnstructuredGridToVtkUnstructuredGrid.h"
#include "Mapping/ResqmlUnstructuredGridSubRepToVtkUnstructuredGrid.h"
#include "Mapping/ResqmlWellboreTrajectoryToVtkPolyData.h"
#include "Mapping/ResqmlWellboreMarkerFrameToVtkPartitionedDataSet.h"
#include "Mapping/ResqmlWellboreFrameToVtkPartitionedDataSet.h"
#include "Mapping/WitsmlWellboreCompletionToVtkPartitionedDataSet.h"
#include "Mapping/WitsmlWellboreCompletionPerforationToVtkPolyData.h"
#include "Mapping/CommonAbstractObjectSetToVtkPartitionedDataSetSet.h"

ResqmlDataRepositoryToVtkPartitionedDataSetCollection::ResqmlDataRepositoryToVtkPartitionedDataSetCollection()
    : _markerOrientation(false),
      _markerSize(10),
      _repository(new common::DataObjectRepository()),
      _output(vtkSmartPointer<vtkPartitionedDataSetCollection>::New()),
      _nodeIdToMapper(),
      _currentSelection(),
      _oldSelection()
{
    auto w_assembly = vtkSmartPointer<vtkDataAssembly>::New();
    w_assembly->SetRootNodeName("data");

    _output->SetDataAssembly(w_assembly);
    _timesStep.clear();
}

ResqmlDataRepositoryToVtkPartitionedDataSetCollection::~ResqmlDataRepositoryToVtkPartitionedDataSetCollection()
{
    delete _repository;
    for (const auto &w_keyVal : _nodeIdToMapper)
    {
        delete w_keyVal.second;
    }
}

MapperType getMapperType(TreeViewNodeType p_type)
{
    switch (p_type) {
    case TreeViewNodeType::Properties:
    case TreeViewNodeType::Perforation:
    case TreeViewNodeType::WellboreChannel:
    case TreeViewNodeType::WellboreMarker:
    case TreeViewNodeType::TimeSeries:
        return MapperType::Data;
    case TreeViewNodeType::Unknown:
    case TreeViewNodeType::Collection:
    case TreeViewNodeType::Wellbore:
        return MapperType::Folder;
    case TreeViewNodeType::Representation:
    case TreeViewNodeType::SubRepresentation:
    case TreeViewNodeType::WellboreTrajectory:
        return MapperType::Mapper;
    case TreeViewNodeType::WellboreMarkerFrame:
    case TreeViewNodeType::WellboreFrame:
    case TreeViewNodeType::WellboreCompletion:
        return MapperType::MapperSet;
    default:
        return MapperType::Folder;
    }
}

// This function replaces the VTK function this->MakeValidNodeName(),
// which has a bug in the sorted_valid_chars array. The '.' character is placed
// before the '-' character, which is incorrect. This function uses a valid_chars
// array that correctly sorts the characters. The function checks if each character
// in the input string is valid, and adds it to the output string if it is valid.
// If the first character of the output string is not valid, an underscore is added
// to the beginning of the output string. This function is designed to create a valid
// node name from a given string.
std::string ResqmlDataRepositoryToVtkPartitionedDataSetCollection::MakeValidNodeName(const char *p_name)
{
    if (p_name == nullptr || p_name[0] == '\0')
    {
        return std::string();
    }

    const char w_sortedValidChars[] =
        "-.0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ_abcdefghijklmnopqrstuvwxyz";
    const auto w_sortedValidCharsLen = strlen(w_sortedValidChars);

    std::string w_result;
    w_result.reserve(strlen(p_name));
    for (size_t w_cc = 0, max = strlen(p_name); w_cc < max; ++w_cc)
    {
        if (std::binary_search(
                w_sortedValidChars, w_sortedValidChars + w_sortedValidCharsLen, p_name[w_cc]))
        {
            w_result += p_name[w_cc];
        }
    }

    if (w_result.empty() ||
        ((w_result[0] < 'a' || w_result[0] > 'z') && (w_result[0] < 'A' || w_result[0] > 'Z') &&
         w_result[0] != '_'))
    {
        return "_" + w_result;
    }
    return w_result;
}

std::string SimplifyXmlTag(std::string p_typeRepresentation)
{
    std::string w_suffix = "Representation";
    std::string w_prefix = "Wellbore";

    if (p_typeRepresentation.size() >= w_suffix.size() && p_typeRepresentation.substr(p_typeRepresentation.size() - w_suffix.size()) == w_suffix)
    {
        p_typeRepresentation = p_typeRepresentation.substr(0, p_typeRepresentation.size() - w_suffix.size());
    }

    if (p_typeRepresentation.size() >= w_prefix.size() && p_typeRepresentation.substr(0, w_prefix.size()) == w_prefix)
    {
        p_typeRepresentation = p_typeRepresentation.substr(w_prefix.size());
    }
    return p_typeRepresentation;
}

//----------------------------------------------------------------------------
std::vector<std::string> ResqmlDataRepositoryToVtkPartitionedDataSetCollection::connect(const std::string &p_etpUrl, const std::string &p_dataPartition, const std::string &p_authConnection)
{
    std::vector<std::string> w_result;
#ifdef WITH_ETP_SSL
    boost::uuids::random_generator w_gen;
    ETP_NS::InitializationParameters w_initializationParams(w_gen(), p_etpUrl);

    std::map<std::string, std::string> w_additionalHandshakeHeaderFields = {{"data-partition-id", p_dataPartition}};
    if (p_etpUrl.find("ws://") == 0)
    {
        _session = ETP_NS::ClientSessionLaunchers::createWsClientSession(&w_initializationParams, p_authConnection, w_additionalHandshakeHeaderFields);
    }
    else
    {
        _session = ETP_NS::ClientSessionLaunchers::createWssClientSession(&w_initializationParams, p_authConnection, w_additionalHandshakeHeaderFields);
    }
    try
    {
        _session->setDataspaceProtocolHandlers(std::make_shared<ETP_NS::DataspaceHandlers>(_session.get()));
    }
    catch (const std::exception &e)
    {
        vtkOutputWindowDisplayErrorText((std::string("fesapi error > ") + e.what()).c_str());
    }
    try
    {
        _session->setDiscoveryProtocolHandlers(std::make_shared<ETP_NS::DiscoveryHandlers>(_session.get()));
    }
    catch (const std::exception &e)
    {
        vtkOutputWindowDisplayErrorText((std::string("fesapi error > ") + e.what()).c_str());
    }
    try
    {
        _session->setStoreProtocolHandlers(std::make_shared<ETP_NS::StoreHandlers>(_session.get()));
    }
    catch (const std::exception &e)
    {
        vtkOutputWindowDisplayErrorText((std::string("fesapi error > ") + e.what()).c_str());
    }
    try
    {
        _session->setDataArrayProtocolHandlers(std::make_shared<ETP_NS::DataArrayHandlers>(_session.get()));
    }
    catch (const std::exception &e)
    {
        vtkOutputWindowDisplayErrorText((std::string("fesapi error > ") + e.what()).c_str());
    }

    _repository->setHdfProxyFactory(new ETP_NS::FesapiHdfProxyFactory(_session.get()));

    //
    if (p_etpUrl.find("ws://") == 0)
    {
        auto w_plainSession = std::dynamic_pointer_cast<ETP_NS::PlainClientSession>(_session);
        std::thread w_sessionThread(&ETP_NS::PlainClientSession::run, w_plainSession);
        w_sessionThread.detach();
    }
    else
    {
        auto w_sslSession = std::dynamic_pointer_cast<ETP_NS::SslClientSession>(_session);
        std::thread w_sessionThread(&ETP_NS::SslClientSession::run, w_sslSession);
        w_sessionThread.detach();
    }

    // Wait for the ETP session to be opened
    auto w_tStart = std::chrono::high_resolution_clock::now();
    while (_session->isEtpSessionClosed())
    {
        if (std::chrono::duration<double, std::milli>(std::chrono::high_resolution_clock::now() - w_tStart).count() > 5000)
        {
            throw std::invalid_argument("Did you forget to click apply button before to connect? Time out for websocket connection" +
                                        std::to_string(std::chrono::duration<double, std::milli>(std::chrono::high_resolution_clock::now() - w_tStart).count()) + "ms.\n");
        }
    }

    //************ LIST DATASPACES ************
    const auto w_dataspaces = _session->getDataspaces();

    std::transform(w_dataspaces.begin(), w_dataspaces.end(), std::back_inserter(w_result),
                   [](const Energistics::Etp::v12::Datatypes::Object::Dataspace &w_ds)
                   { return w_ds.uri; });

#endif
    return w_result;
}

//----------------------------------------------------------------------------
void ResqmlDataRepositoryToVtkPartitionedDataSetCollection::disconnect()
{
#ifdef WITH_ETP_SSL
    _session->close();
#endif
}

//----------------------------------------------------------------------------
std::string ResqmlDataRepositoryToVtkPartitionedDataSetCollection::addFile(const char *p_fileName)
{

    COMMON_NS::EpcDocument w_pck(p_fileName);
    std::string w_message = w_pck.deserializeInto(*_repository);
    w_pck.close();
    _files.insert(p_fileName);
    w_message += buildDataAssemblyFromDataObjectRepo(p_fileName);
    return w_message;
}

//----------------------------------------------------------------------------
void ResqmlDataRepositoryToVtkPartitionedDataSetCollection::closeFiles()
{
    for (const std::string w_filename : _files)
    {
        COMMON_NS::EpcDocument w_pck(w_filename);
        w_pck.close();
    }
}

//----------------------------------------------------------------------------
std::string ResqmlDataRepositoryToVtkPartitionedDataSetCollection::addDataspace(const char *p_dataspace)
{
#ifdef WITH_ETP_SSL
    //************ LIST RESOURCES ************
    Energistics::Etp::v12::Datatypes::Object::ContextInfo w_ctxInfo;
    w_ctxInfo.uri = p_dataspace;
    w_ctxInfo.depth = 0;
    w_ctxInfo.navigableEdges = Energistics::Etp::v12::Datatypes::Object::RelationshipKind::Both;
    w_ctxInfo.includeSecondaryTargets = false;
    w_ctxInfo.includeSecondarySources = false;
    const auto w_resources = _session->getResources(w_ctxInfo, Energistics::Etp::v12::Datatypes::Object::ContextScopeKind::targets);

    //************ GET ALL DATAOBJECTS ************
    _repository->setHdfProxyFactory(new ETP_NS::FesapiHdfProxyFactory(_session.get()));
    if (!w_resources.empty())
    {
        std::map<std::string, std::string> w_query;
        for (size_t w_i = 0; w_i < w_resources.size(); ++w_i)
        {
            w_query[std::to_string(w_i)] = w_resources[w_i].uri;
        }
        const auto w_dataobjects = _session->getDataObjects(w_query);
        for (auto &w_datoObject : w_dataobjects)
        {
            _repository->addOrReplaceGsoapProxy(w_datoObject.second.data,
                                                ETP_NS::EtpHelpers::getDataObjectType(w_datoObject.second.resource.uri),
                                                ETP_NS::EtpHelpers::getDataspaceUri(w_datoObject.second.resource.uri));
        }
    }
    else
    {
        vtkOutputWindowDisplayWarningText(("There is no dataobject in the dataspace : " + std::string(p_dataspace) + "\n").c_str());
    }
#endif
    return buildDataAssemblyFromDataObjectRepo("");
}

namespace
{
    auto lexicographicalComparison = [](const COMMON_NS::AbstractObject *p_a, const COMMON_NS::AbstractObject *p_b) -> bool
    {
        return p_a->getTitle().compare(p_b->getTitle()) < 0;
    };

    template <typename T>
    void sortAndAdd(std::vector<T> p_source, std::vector<RESQML2_NS::AbstractRepresentation const *> &p_dest)
    {
        std::sort(p_source.begin(), p_source.end(), lexicographicalComparison);
        std::move(p_source.begin(), p_source.end(), std::inserter(p_dest, p_dest.end()));
    }
}

std::string ResqmlDataRepositoryToVtkPartitionedDataSetCollection::buildDataAssemblyFromDataObjectRepo(const char *p_fileName)
{
    std::vector<RESQML2_NS::AbstractRepresentation const *> w_allReps;

    // create vtkDataAssembly: create treeView in property panel
    sortAndAdd(_repository->getHorizonGrid2dRepresentationSet(), w_allReps);
    sortAndAdd(_repository->getIjkGridRepresentationSet(), w_allReps);
    sortAndAdd(_repository->getAllPolylineSetRepresentationSet(), w_allReps);
    sortAndAdd(_repository->getAllTriangulatedSetRepresentationSet(), w_allReps);
    sortAndAdd(_repository->getUnstructuredGridRepresentationSet(), w_allReps);

    // See https://stackoverflow.com/questions/15347123/how-to-construct-a-stdstring-from-a-stdvectorstring
    std::string w_message = std::accumulate(std::begin(w_allReps), std::end(w_allReps), std::string{},
                                            [&](std::string &message, RESQML2_NS::AbstractRepresentation const *rep)
                                            {
                                                return message += searchRepresentations(rep);
                                            });
    // get WellboreTrajectory
    w_message += searchWellboreTrajectory(p_fileName);

    // get TimeSeries
    w_message += searchTimeSeries(p_fileName);

    return w_message;
}

std::string ResqmlDataRepositoryToVtkPartitionedDataSetCollection::searchRepresentations(resqml2::AbstractRepresentation const *p_representation, int p_NodeId)
{
    std::string w_result;

    if (p_representation->isPartial())
    {
        // check if it has already been added
        // not exist => not loaded
        if (_output->GetDataAssembly()->FindFirstNodeWithName(("_" + p_representation->getUuid()).c_str()) == -1)
        {
            return "Partial representation with UUID \"" + p_representation->getUuid() + "\" is not loaded.\n";
        } /******* TODO ********/ // exist but not the same type ?
    }
    else
    {
        // The leading underscore is forced by VTK which does not support a node name starting with a digit (probably because it is a QNAME).
        const std::string w_nodeName = "_" + p_representation->getUuid();
        const int w_existingNodeId = _output->GetDataAssembly()->FindFirstNodeWithName(w_nodeName.c_str());
        if (w_existingNodeId == -1)
        {
            p_NodeId = _output->GetDataAssembly()->AddNode(w_nodeName.c_str(), p_NodeId);

            auto const *w_subrep = dynamic_cast<RESQML2_NS::SubRepresentation const *>(p_representation);
            // To shorten the xmlTag by removing �Representation� from the end.

            std::string w_typeRepresentation = SimplifyXmlTag(p_representation->getXmlTag());

            const std::string w_representationVtkValidName = w_subrep == nullptr
                                                                 ? this->MakeValidNodeName((w_typeRepresentation + "_" + p_representation->getTitle()).c_str())
                                                                 : this->MakeValidNodeName((w_typeRepresentation + "_" + w_subrep->getSupportingRepresentation(0)->getTitle() + "_" + p_representation->getTitle()).c_str());

            const TreeViewNodeType w_type = w_subrep == nullptr
                                                ? TreeViewNodeType::Representation
                                                : TreeViewNodeType::SubRepresentation;

            _output->GetDataAssembly()->SetAttribute(p_NodeId, "label", w_representationVtkValidName.c_str());
            _output->GetDataAssembly()->SetAttribute(p_NodeId, "type", std::to_string(static_cast<int>(TreeViewNodeType::Representation)).c_str());
        }
        else
        {
            p_NodeId = w_existingNodeId;
        }
    }

    // add sub representation with properties (only for ijkGrid and unstructured grid)
    if (dynamic_cast<RESQML2_NS::AbstractIjkGridRepresentation const *>(p_representation) != nullptr ||
        dynamic_cast<RESQML2_NS::UnstructuredGridRepresentation const *>(p_representation) != nullptr)
    {
        w_result += searchSubRepresentation(p_representation, p_NodeId);
    }

    // add properties to representation
    w_result += searchProperties(p_representation, p_NodeId);

    return w_result;
}

std::string ResqmlDataRepositoryToVtkPartitionedDataSetCollection::searchSubRepresentation(RESQML2_NS::AbstractRepresentation const *p_representation, int p_nodeParent)
{
    std::string w_message = "";
    try
    {
        auto w_subRepresentationSet = p_representation->getSubRepresentationSet();
        std::sort(w_subRepresentationSet.begin(), w_subRepresentationSet.end(), lexicographicalComparison);

        w_message = std::accumulate(std::begin(w_subRepresentationSet), std::end(w_subRepresentationSet), std::string{},
                                    [&](std::string &message, RESQML2_NS::SubRepresentation *b)
                                    {
                                        return message += searchRepresentations(b, _output->GetDataAssembly()->GetParent(p_nodeParent));
                                    });
    }
    catch (const std::exception &e)
    {
        return "Exception in FESAPI when calling getSubRepresentationSet for uuid : " + p_representation->getUuid() + " : " + e.what() + ".\n";
    }

    return w_message;
}

int ResqmlDataRepositoryToVtkPartitionedDataSetCollection::searchPropertySet(resqml2_0_1::PropertySet const *p_propSet, int p_nodeId)
{
    if (_output->GetDataAssembly()->FindFirstNodeWithName(("_" + p_propSet->getUuid()).c_str()) == -1)
    { // verify uuid exist in treeview
      // To shorten the xmlTag by removing �Representation� from the end.
        resqml2_0_1::PropertySet *w_parent = p_propSet->getParent();
        if (w_parent != nullptr)
        {
            p_nodeId = searchPropertySet(w_parent, p_nodeId);
        }
        if (_output->GetDataAssembly()->FindFirstNodeWithName(("_" + p_propSet->getUuid()).c_str()) == -1)
        {
            const std::string w_vtkValidName = MakeValidNodeName(("Collection_" + p_propSet->getTitle()).c_str());
            p_nodeId = _output->GetDataAssembly()->AddNode(("_" + p_propSet->getUuid()).c_str(), p_nodeId);
            _output->GetDataAssembly()->SetAttribute(p_nodeId, "label", w_vtkValidName.c_str());
            _output->GetDataAssembly()->SetAttribute(p_nodeId, "type", std::to_string(static_cast<int>(TreeViewNodeType::Collection)).c_str());
        }
    }
    else
    {
        return _output->GetDataAssembly()->FindFirstNodeWithName(("_" + p_propSet->getUuid()).c_str());
    }

    return p_nodeId;
}

std::string ResqmlDataRepositoryToVtkPartitionedDataSetCollection::searchProperties(RESQML2_NS::AbstractRepresentation const *p_representation, int p_nodeParent)
{
    try
    {
        auto w_valuesPropertySet = p_representation->getValuesPropertySet();
        std::sort(w_valuesPropertySet.begin(), w_valuesPropertySet.end(), lexicographicalComparison);

        int w_propertySetNodeId = p_nodeParent;
        // property
        for (auto const *w_property : w_valuesPropertySet)
        {
            for (resqml2_0_1::PropertySet const *w_propertySet : w_property->getPropertySets())
            {
                w_propertySetNodeId = searchPropertySet(w_propertySet, p_nodeParent);
            }

            const std::string w_vtkValidName = MakeValidNodeName((w_property->getXmlTag() + '_' + w_property->getTitle()).c_str());

            if (_output->GetDataAssembly()->FindFirstNodeWithName(("_" + w_property->getUuid()).c_str()) == -1)
            { // verify uuid exist in treeview
                int w_propertyNodeId = _output->GetDataAssembly()->AddNode(("_" + w_property->getUuid()).c_str(), w_propertySetNodeId);
                _output->GetDataAssembly()->SetAttribute(w_propertyNodeId, "label", w_vtkValidName.c_str());
                _output->GetDataAssembly()->SetAttribute(w_propertyNodeId, "type", std::to_string(static_cast<int>(TreeViewNodeType::Properties)).c_str());
            }
        }
    }
    catch (const std::exception &e)
    {
        return "Exception in FESAPI when calling getValuesPropertySet with representation uuid: " + p_representation->getUuid() + " : " + e.what() + ".\n";
    }

    return "";
}

int ResqmlDataRepositoryToVtkPartitionedDataSetCollection::searchRepresentationSetRepresentation(resqml2::RepresentationSetRepresentation const *p_rsr, int p_nodeId)
{
    if (_output->GetDataAssembly()->FindFirstNodeWithName(("_" + p_rsr->getUuid()).c_str()) == -1)
    { // verify uuid exist in treeview
      // To shorten the xmlTag by removing �Representation� from the end.
        for (resqml2::RepresentationSetRepresentation *w_rsr : p_rsr->getRepresentationSetRepresentationSet())
        {
            p_nodeId = searchRepresentationSetRepresentation(w_rsr, p_nodeId);
        }
        if (_output->GetDataAssembly()->FindFirstNodeWithName(("_" + p_rsr->getUuid()).c_str()) == -1)
        {
            const std::string w_vtkValidName = this->MakeValidNodeName(("Collection_" + p_rsr->getTitle()).c_str());
            p_nodeId = _output->GetDataAssembly()->AddNode(("_" + p_rsr->getUuid()).c_str(), p_nodeId);
            _output->GetDataAssembly()->SetAttribute(p_nodeId, "label", w_vtkValidName.c_str());
            _output->GetDataAssembly()->SetAttribute(p_nodeId, "type", std::to_string(static_cast<int>(TreeViewNodeType::Collection)).c_str());
        }
    }
    else
    {
        return _output->GetDataAssembly()->FindFirstNodeWithName(("_" + p_rsr->getUuid()).c_str());
    }
    return p_nodeId;
}

std::string ResqmlDataRepositoryToVtkPartitionedDataSetCollection::searchWellboreTrajectory(const std::string &p_fileName)
{
    std::string w_result;

    int w_nodeId = 0;
    for (auto *w_wellboreTrajectory : _repository->getWellboreTrajectoryRepresentationSet())
    {
        const auto *w_wellboreFeature = dynamic_cast<RESQML2_NS::WellboreFeature *>(w_wellboreTrajectory->getInterpretation()->getInterpretedFeature());

        int w_nodeId = 0;
        int w_initNodeId = 0;
        if (_output->GetDataAssembly()->FindFirstNodeWithName(("_" + w_wellboreTrajectory->getUuid()).c_str()) == -1)
        { // verify uuid exist in treeview
          // To shorten the xmlTag by removing �Representation� from the end.
            for (resqml2::RepresentationSetRepresentation *w_rsr : w_wellboreTrajectory->getRepresentationSetRepresentationSet())
            {
                w_initNodeId = searchRepresentationSetRepresentation(w_rsr);
            }

            if (_output->GetDataAssembly()->FindFirstNodeWithName(("_" + w_wellboreFeature->getUuid()).c_str()) == -1)
            {
                const std::string w_vtkValidName = "Wellbore_" + MakeValidNodeName(w_wellboreFeature->getTitle().c_str());
                w_initNodeId = _output->GetDataAssembly()->AddNode(("_" + w_wellboreFeature->getUuid()).c_str(), w_initNodeId);
                _output->GetDataAssembly()->SetAttribute(w_initNodeId, "label", w_vtkValidName.c_str());
                _output->GetDataAssembly()->SetAttribute(w_initNodeId, "type", std::to_string(static_cast<int>(TreeViewNodeType::Wellbore)).c_str());
            }

            if (w_wellboreTrajectory->isPartial())
            {
                // check if it has already been added

                const std::string w_vtkValidName = MakeValidNodeName((SimplifyXmlTag(w_wellboreTrajectory->getXmlTag()) + "_" + w_wellboreTrajectory->getTitle()).c_str());
                // not exist => not loaded
                if (_output->GetDataAssembly()->FindFirstNodeWithName(("_" + w_vtkValidName).c_str()) == -1)
                {
                    w_result = w_result + " Partial UUID: (" + w_wellboreTrajectory->getUuid() + ") is not loaded \n";
                    continue;
                } /******* TODO ********/ // exist but not the same type ?
            }
            else
            {
                const std::string w_vtkValidName = MakeValidNodeName((SimplifyXmlTag(w_wellboreTrajectory->getXmlTag()) + '_' + w_wellboreTrajectory->getTitle()).c_str());
                w_nodeId = _output->GetDataAssembly()->AddNode(("_" + w_wellboreTrajectory->getUuid()).c_str(), w_initNodeId);
                _output->GetDataAssembly()->SetAttribute(w_nodeId, "label", w_vtkValidName.c_str());
                _output->GetDataAssembly()->SetAttribute(w_nodeId, "type", std::to_string(static_cast<int>(TreeViewNodeType::WellboreTrajectory)).c_str());
            }
        }
        w_result += searchWellboreFrame(w_wellboreTrajectory, w_initNodeId);
        w_result += searchWellboreCompletion(w_wellboreFeature, w_initNodeId);
    }
    return w_result;
}

std::string ResqmlDataRepositoryToVtkPartitionedDataSetCollection::searchWellboreFrame(const resqml2::WellboreTrajectoryRepresentation *p_wellboreTrajectory, int p_nodeId)
{
    std::string w_result = "";
    for (auto *w_wellboreFrame : p_wellboreTrajectory->getWellboreFrameRepresentationSet())
    {
        if (_output->GetDataAssembly()->FindFirstNodeWithName(("_" + w_wellboreFrame->getUuid()).c_str()) == -1)
        { // verify uuid exist in treeview
          // common with wellboreMarkerFrame & WellboreFrame
            const std::string w_vtkValidName = MakeValidNodeName((SimplifyXmlTag(w_wellboreFrame->getXmlTag()) + '_' + w_wellboreFrame->getTitle()).c_str());
            int w_frameNodeId = _output->GetDataAssembly()->AddNode(("_" + w_wellboreFrame->getUuid()).c_str(), p_nodeId);
            _output->GetDataAssembly()->SetAttribute(w_frameNodeId, "label", w_vtkValidName.c_str());

            auto *w_wellboreMarkerFrame = dynamic_cast<RESQML2_NS::WellboreMarkerFrameRepresentation const *>(w_wellboreFrame);
            if (w_wellboreMarkerFrame == nullptr)
            { // WellboreFrame
                _output->GetDataAssembly()->SetAttribute(w_frameNodeId, "type", std::to_string(static_cast<int>(TreeViewNodeType::WellboreFrame)).c_str());
                // chanel
                for (auto *w_property : w_wellboreFrame->getValuesPropertySet())
                {
                    const std::string w_vtkValidName = MakeValidNodeName((w_property->getXmlTag() + '_' + w_property->getTitle()).c_str());
                    int w_nodeId = _output->GetDataAssembly()->AddNode(("_" + w_property->getUuid()).c_str(), w_frameNodeId);
                    _output->GetDataAssembly()->SetAttribute(w_nodeId, "label", w_vtkValidName.c_str());
                    _output->GetDataAssembly()->SetAttribute(w_nodeId, "type", std::to_string(static_cast<int>(TreeViewNodeType::WellboreChannel)).c_str());
                }
            }
            else
            { // WellboreMarkerFrame
                _output->GetDataAssembly()->SetAttribute(w_frameNodeId, "type", std::to_string(static_cast<int>(TreeViewNodeType::WellboreMarkerFrame)).c_str());
                // marker
                for (auto *w_wellboreMarker : w_wellboreMarkerFrame->getWellboreMarkerSet())
                {
                    const std::string w_vtkValidName = MakeValidNodeName((w_wellboreMarker->getXmlTag() + '_' + w_wellboreMarker->getTitle()).c_str());
                    int w_nodeId = _output->GetDataAssembly()->AddNode(("_" + w_wellboreMarker->getUuid()).c_str(), w_frameNodeId);
                    _output->GetDataAssembly()->SetAttribute(w_nodeId, "label", w_vtkValidName.c_str());
                    _output->GetDataAssembly()->SetAttribute(w_nodeId, "type", std::to_string(static_cast<int>(TreeViewNodeType::WellboreMarker)).c_str());
                }
            }
        }
    }
    return w_result;
}

std::string ResqmlDataRepositoryToVtkPartitionedDataSetCollection::searchWellboreCompletion(const RESQML2_NS::WellboreFeature *p_wellboreFeature, int p_nodeId)
{
    std::string w_result = "";
    // witsml2::Wellbore *witsmlWellbore = nullptr;
    if (!p_wellboreFeature->isPartial())
    {
        if (const witsml2::Wellbore *w_witsmlWellbore = dynamic_cast<witsml2::Wellbore *>(p_wellboreFeature->getWitsmlWellbore()))
        {
            for (const auto *w_wellboreCompletion : w_witsmlWellbore->getWellboreCompletionSet())
            {
                const std::string w_vtkValidName = MakeValidNodeName((SimplifyXmlTag(w_wellboreCompletion->getXmlTag()) + '_' + w_wellboreCompletion->getTitle()).c_str());
                int w_completionNodeId = _output->GetDataAssembly()->AddNode(("_" + w_wellboreCompletion->getUuid()).c_str(), p_nodeId);
                _output->GetDataAssembly()->SetAttribute(w_completionNodeId, "label", w_vtkValidName.c_str());
                _output->GetDataAssembly()->SetAttribute(w_completionNodeId, "type", std::to_string(static_cast<int>(TreeViewNodeType::WellboreCompletion)).c_str());
                // Iterate over the perforations.
                for (uint64_t w_perforationIndex = 0; w_perforationIndex < w_wellboreCompletion->getConnectionCount(WITSML2_1_NS::WellboreCompletion::WellReservoirConnectionType::PERFORATION); ++w_perforationIndex)
                {
                    // init
                    std::string w_perforationName = "Perfo";
                    std::string w_perforationSkin = "";
                    std::string w_perforationDiameter = "";

                    // Test with Petrel rules
                    auto w_petrelName = w_wellboreCompletion->getConnectionExtraMetadata(WITSML2_1_NS::WellboreCompletion::WellReservoirConnectionType::PERFORATION, w_perforationIndex, "Petrel:Name0");
                    // arbitrarily select the first event name as perforation name
                    if (w_petrelName.size() > 0)
                    {
                        w_perforationName += "_" + w_petrelName[0];
                        // skin
                        auto w_petrelSkin = w_wellboreCompletion->getConnectionExtraMetadata(WITSML2_1_NS::WellboreCompletion::WellReservoirConnectionType::PERFORATION, w_perforationIndex, "Petrel:Skin0");
                        if (w_petrelSkin.size() > 0)
                        {
                            w_perforationSkin = w_petrelSkin[0];
                        }
                        // diameter
                        auto w_petrelDiam = w_wellboreCompletion->getConnectionExtraMetadata(WITSML2_1_NS::WellboreCompletion::WellReservoirConnectionType::PERFORATION, w_perforationIndex, "Petrel:BoreholePerforatedSection0");
                        if (w_petrelDiam.size() > 0)
                        {
                            w_perforationDiameter = w_petrelDiam[0];
                        }
                    }
                    else
                    {
                        // Test with Sismage rules
                        auto w_sismageName = w_wellboreCompletion->getConnectionExtraMetadata(WITSML2_1_NS::WellboreCompletion::WellReservoirConnectionType::PERFORATION, w_perforationIndex, "Sismage-CIG:Name");
                        if (w_sismageName.size() > 0)
                        {
                            w_perforationName += "_" + w_sismageName[0];
                            // skin
                            auto w_sismageSkin = w_wellboreCompletion->getConnectionExtraMetadata(WITSML2_1_NS::WellboreCompletion::WellReservoirConnectionType::PERFORATION, w_perforationIndex, "Sismage-CIG:Skin");
                            if (w_sismageSkin.size() > 0)
                            {
                                w_perforationSkin = w_sismageSkin[0];
                            }
                            // diameter
                            auto w_sismageDiam = w_wellboreCompletion->getConnectionExtraMetadata(WITSML2_1_NS::WellboreCompletion::WellReservoirConnectionType::PERFORATION, w_perforationIndex, "Petrel:CompletionDiameter");
                            if (w_sismageDiam.size() > 0)
                            {
                                w_perforationDiameter = w_sismageDiam[0];
                            }
                        }
                        else
                        {
                            // default
                            w_perforationName += "_" + w_wellboreCompletion->getConnectionUid(WITSML2_1_NS::WellboreCompletion::WellReservoirConnectionType::PERFORATION, w_perforationIndex);
                        }
                    }
                    w_perforationName += "__Skin_" + w_perforationSkin + "__Diam_" + w_perforationDiameter;

                    int w_nodeId = _output->GetDataAssembly()->AddNode(this->MakeValidNodeName(("_" + w_wellboreCompletion->getUuid() + "_" + w_wellboreCompletion->getConnectionUid(WITSML2_1_NS::WellboreCompletion::WellReservoirConnectionType::PERFORATION, w_perforationIndex)).c_str()).c_str(), w_completionNodeId);
                    _output->GetDataAssembly()->SetAttribute(w_nodeId, "label", MakeValidNodeName((w_perforationName).c_str()).c_str());
                    _output->GetDataAssembly()->SetAttribute(w_nodeId, "type", std::to_string(static_cast<int>(TreeViewNodeType::Perforation)).c_str());
                    _output->GetDataAssembly()->SetAttribute(w_nodeId, "connection", w_wellboreCompletion->getConnectionUid(WITSML2_1_NS::WellboreCompletion::WellReservoirConnectionType::PERFORATION, w_perforationIndex).c_str());
                    _output->GetDataAssembly()->SetAttribute(w_nodeId, "skin", w_perforationSkin.c_str());
                    _output->GetDataAssembly()->SetAttribute(w_nodeId, "diameter", w_perforationDiameter.c_str());
                }
            }
        }
    }
    return w_result;
}
std::string ResqmlDataRepositoryToVtkPartitionedDataSetCollection::searchTimeSeries(const std::string &p_fileName)
{
    _timesStep.clear();

    std::string w_message = "";
    std::vector<EML2_NS::TimeSeries *> w_timeSeriesSet;
    try
    {
        w_timeSeriesSet = _repository->getTimeSeriesSet();
    }
    catch (const std::exception &e)
    {
        w_message = w_message + "Exception in FESAPI when calling getTimeSeriesSet with file: " + p_fileName + " : " + e.what();
    }

    /****
     *  change property parent to times serie parent
     ****/
    for (auto const *w_timeSeries : w_timeSeriesSet)
    {
        // get properties link to Times series
        try
        {
            std::map<std::string, std::vector<int>> w_propertyNameToNodeIdSet;
            for (auto const *w_prop : w_timeSeries->getPropertySet())
            {
                if (w_prop->getXmlTag() == RESQML2_NS::ContinuousProperty::XML_TAG ||
                    w_prop->getXmlTag() == RESQML2_NS::DiscreteProperty::XML_TAG)
                {
                    auto w_nodeId = (_output->GetDataAssembly()->FindFirstNodeWithName(("_" + w_prop->getUuid()).c_str()));
                    if (w_nodeId == -1)
                    {
                        w_message = w_message + "The property " + w_prop->getUuid() + " is not supported and consequently cannot be associated to its time series.\n";
                        continue;
                    }
                    else
                    {
                        // same node parent else not supported
                        const int w_parentNodeId = _output->GetDataAssembly()->GetParent(w_nodeId);
                        if (w_parentNodeId != -1)
                        {
                            w_propertyNameToNodeIdSet[w_prop->getTitle()].push_back(w_nodeId);
                            const size_t w_timeIndexInTimeSeries = w_timeSeries->getTimestampIndex(w_prop->getSingleTimestamp());
                            _timesStep.push_back(w_timeIndexInTimeSeries);
                            _timeSeriesUuidAndTitleToIndexAndPropertiesUuid[w_timeSeries->getUuid()][MakeValidNodeName((w_timeSeries->getXmlTag() + '_' + w_prop->getTitle()).c_str())][w_timeIndexInTimeSeries] = w_prop->getUuid();
                        }
                        else
                        {
                            w_message = w_message + "The properties of time series " + w_timeSeries->getUuid() + " aren't parent and is not supported.\n";
                            continue;
                        }
                    }
                }
            }
            // erase duplicate Index
            sort(_timesStep.begin(), _timesStep.end());
            _timesStep.erase(unique(_timesStep.begin(), _timesStep.end()), _timesStep.end());

            for (const auto &w_myPair : w_propertyNameToNodeIdSet)
            {
                std::vector<int> w_propertyNodeSet = w_myPair.second;

                int w_parentNodeId = -1;
                // erase property add to treeview for group by TimeSerie
                for (auto node : w_propertyNodeSet)
                {
                    w_parentNodeId = _output->GetDataAssembly()->GetParent(node);
                    _output->GetDataAssembly()->RemoveNode(node);
                }
                std::string w_vtkValidName = MakeValidNodeName((w_timeSeries->getXmlTag() + '_' + w_myPair.first).c_str());
                auto w_nodeId = _output->GetDataAssembly()->AddNode(("_" + w_timeSeries->getUuid() + w_vtkValidName).c_str(), w_parentNodeId);
                _output->GetDataAssembly()->SetAttribute(w_nodeId, "label", w_vtkValidName.c_str());
                _output->GetDataAssembly()->SetAttribute(w_nodeId, "type", std::to_string(static_cast<int>(TreeViewNodeType::TimeSeries)).c_str());
            }
        }
        catch (const std::exception &e)
        {
            w_message = w_message + "Exception in FESAPI when calling getPropertySet with file: " + p_fileName + " : " + e.what();
        }
    }

    return w_message;
}

std::string ResqmlDataRepositoryToVtkPartitionedDataSetCollection::selectNodeId(int p_node)
{
    if (p_node != 0)
    {
        selectNodeIdParent(p_node);

        _currentSelection.insert(p_node);
        _oldSelection.erase(p_node);
    }
    selectNodeIdChildren(p_node);

    return "";
}

void ResqmlDataRepositoryToVtkPartitionedDataSetCollection::selectNodeIdParent(int p_node)
{
    if (_output->GetDataAssembly()->GetParent(p_node) > 0)
    {
        _currentSelection.insert(_output->GetDataAssembly()->GetParent(p_node));
        _oldSelection.erase(_output->GetDataAssembly()->GetParent(p_node));
        selectNodeIdParent(_output->GetDataAssembly()->GetParent(p_node));
    }
}
void ResqmlDataRepositoryToVtkPartitionedDataSetCollection::selectNodeIdChildren(int p_node)
{
    for (int w_nodeChild : _output->GetDataAssembly()->GetChildNodes(p_node))
    {
        _currentSelection.insert(w_nodeChild);
        _oldSelection.erase(w_nodeChild);
        selectNodeIdChildren(w_nodeChild);
    }
}

void ResqmlDataRepositoryToVtkPartitionedDataSetCollection::clearSelection()
{
    _oldSelection = _currentSelection;
    _currentSelection.clear();
}

void ResqmlDataRepositoryToVtkPartitionedDataSetCollection::initMapperSet(const TreeViewNodeType p_type, const int p_nodeId, const int p_nbProcess, const int p_processId)
{
    CommonAbstractObjectToVtkPartitionedDataSet *w_caotvpds = nullptr;
    const std::string w_uuid = std::string(_output->GetDataAssembly()->GetNodeName(p_nodeId)).substr(1);

    COMMON_NS::AbstractObject* const w_abstractObject = _repository->getDataObjectByUuid(w_uuid);

    if (p_type == TreeViewNodeType::WellboreCompletion)
    {
        try
        {
            if (dynamic_cast<witsml2_1::WellboreCompletion*>(w_abstractObject) != nullptr)
            {
                _nodeIdToMapperSet[p_nodeId] = new WitsmlWellboreCompletionToVtkPartitionedDataSet(static_cast<witsml2_1::WellboreCompletion*>(w_abstractObject), p_processId, p_nbProcess);
            }
            else
            {
                vtkOutputWindowDisplayErrorText(("Error object type in vtkDataAssembly for uuid: " + w_uuid + "\n").c_str());
            }
        }
        catch (const std::exception& e)
        {
            vtkOutputWindowDisplayErrorText(("Error when initialize uuid: " + w_uuid + "\n" + e.what()).c_str());
        }
    }
    else if (p_type == TreeViewNodeType::WellboreMarkerFrame)
    {
        try
        {
            if (dynamic_cast<RESQML2_NS::WellboreMarkerFrameRepresentation *>(w_abstractObject) != nullptr)
            {
                _nodeIdToMapperSet[p_nodeId] = new ResqmlWellboreMarkerFrameToVtkPartitionedDataSet(static_cast<RESQML2_NS::WellboreMarkerFrameRepresentation*>(w_abstractObject), p_processId, p_nbProcess);
            }
            else
            {
                vtkOutputWindowDisplayErrorText(("Error object type in vtkDataAssembly for uuid: " + w_uuid + "\n").c_str());
            }
        }
        catch (const std::exception& e)
        {
            vtkOutputWindowDisplayErrorText(("Error when initialize uuid: " + w_uuid + "\n" + e.what()).c_str());
        }
    }
    else if (p_type == TreeViewNodeType::WellboreFrame)
    {
        try
        {
            if (dynamic_cast<RESQML2_NS::WellboreFrameRepresentation *>(w_abstractObject) != nullptr)
            {
                _nodeIdToMapperSet[p_nodeId] = new ResqmlWellboreFrameToVtkPartitionedDataSet(static_cast<RESQML2_NS::WellboreFrameRepresentation*>(w_abstractObject), p_processId, p_nbProcess);
            }
            else
            {
                vtkOutputWindowDisplayErrorText(("Error object type in vtkDataAssembly for uuid: " + w_uuid + "\n").c_str());
            }
        }
        catch (const std::exception& e)
        {
            vtkOutputWindowDisplayErrorText(("Error when initialize uuid: " + w_uuid + "\n" + e.what()).c_str());
        }
    }

}

void ResqmlDataRepositoryToVtkPartitionedDataSetCollection::loadMapper(const TreeViewNodeType p_type, const int p_nodeId, const int p_nbProcess, const int p_processId)
{
    if (TreeViewNodeType::Representation == p_type)
    {
        loadRepresentationMapper(p_nodeId, p_nbProcess, p_processId);
    } 
    else if (TreeViewNodeType::WellboreTrajectory == p_type)
    {
        loadWellboreTrajectoryMapper(p_nodeId);
    }
    else if (TreeViewNodeType::SubRepresentation == p_type)
    {
        loadWellboreTrajectoryMapper(p_nodeId);
    }
}

void ResqmlDataRepositoryToVtkPartitionedDataSetCollection::loadRepresentationMapper(const int p_nodeId, const int p_nbProcess, const int p_processId)
{
    CommonAbstractObjectToVtkPartitionedDataSet* w_caotvpds = nullptr;
    const std::string w_uuid = std::string(_output->GetDataAssembly()->GetNodeName(p_nodeId)).substr(1);
    COMMON_NS::AbstractObject* const w_abstractObject = _repository->getDataObjectByUuid(w_uuid);

    if (dynamic_cast<RESQML2_NS::AbstractIjkGridRepresentation*>(w_abstractObject) != nullptr)
    {
        w_caotvpds = new ResqmlIjkGridToVtkExplicitStructuredGrid(static_cast<RESQML2_NS::AbstractIjkGridRepresentation*>(w_abstractObject), p_processId, p_nbProcess);
    }
    else if (dynamic_cast<RESQML2_NS::Grid2dRepresentation*>(w_abstractObject) != nullptr)
    {
        w_caotvpds = new ResqmlGrid2dToVtkStructuredGrid(static_cast<RESQML2_NS::Grid2dRepresentation*>(w_abstractObject));
    }
    else if (dynamic_cast<RESQML2_NS::TriangulatedSetRepresentation*>(w_abstractObject) != nullptr)
    {
        w_caotvpds = new ResqmlTriangulatedSetToVtkPartitionedDataSet(static_cast<RESQML2_NS::TriangulatedSetRepresentation*>(w_abstractObject));
    }
    else if (dynamic_cast<RESQML2_NS::PolylineSetRepresentation*>(w_abstractObject) != nullptr)
    {
        w_caotvpds = new ResqmlPolylineToVtkPolyData(static_cast<RESQML2_NS::PolylineSetRepresentation*>(w_abstractObject));
    }
    else if (dynamic_cast<RESQML2_NS::UnstructuredGridRepresentation*>(w_abstractObject) != nullptr)
    {
        w_caotvpds = new ResqmlUnstructuredGridToVtkUnstructuredGrid(static_cast<RESQML2_NS::UnstructuredGridRepresentation*>(w_abstractObject));
    }
    else if (dynamic_cast<RESQML2_NS::SubRepresentation*>(w_abstractObject) != nullptr)
    {
        RESQML2_NS::SubRepresentation* w_subRep = static_cast<RESQML2_NS::SubRepresentation*>(w_abstractObject);

        if (dynamic_cast<RESQML2_NS::AbstractIjkGridRepresentation*>(w_subRep->getSupportingRepresentation(0)) != nullptr)
        {
            auto* w_supportingGrid = static_cast<RESQML2_NS::AbstractIjkGridRepresentation*>(w_subRep->getSupportingRepresentation(0));
            if (_nodeIdToMapper.find(_output->GetDataAssembly()->FindFirstNodeWithName(("_" + w_supportingGrid->getUuid()).c_str())) == _nodeIdToMapper.end())
            {
                _nodeIdToMapper[_output->GetDataAssembly()->FindFirstNodeWithName(("_" + w_supportingGrid->getUuid()).c_str())] = new ResqmlIjkGridToVtkExplicitStructuredGrid(w_supportingGrid);
            }
            w_caotvpds = new ResqmlIjkGridSubRepToVtkExplicitStructuredGrid(w_subRep, dynamic_cast<ResqmlIjkGridToVtkExplicitStructuredGrid*>(_nodeIdToMapper[_output->GetDataAssembly()->FindFirstNodeWithName(("_" + w_supportingGrid->getUuid()).c_str())]));
        }
        else if (dynamic_cast<RESQML2_NS::UnstructuredGridRepresentation*>(w_subRep->getSupportingRepresentation(0)) != nullptr)
        {
            auto* w_supportingGrid = static_cast<RESQML2_NS::UnstructuredGridRepresentation*>(w_subRep->getSupportingRepresentation(0));
            if (_nodeIdToMapper.find(_output->GetDataAssembly()->FindFirstNodeWithName(("_" + w_supportingGrid->getUuid()).c_str())) == _nodeIdToMapper.end())
            {
                _nodeIdToMapper[_output->GetDataAssembly()->FindFirstNodeWithName(("_" + w_supportingGrid->getUuid()).c_str())] = new ResqmlUnstructuredGridToVtkUnstructuredGrid(w_supportingGrid);
            }
            w_caotvpds = new ResqmlUnstructuredGridSubRepToVtkUnstructuredGrid(w_subRep, dynamic_cast<ResqmlUnstructuredGridToVtkUnstructuredGrid*>(_nodeIdToMapper[_output->GetDataAssembly()->FindFirstNodeWithName(("_" + w_supportingGrid->getUuid()).c_str())]));
        }
        else {
            vtkOutputWindowDisplayWarningText(("FESPP only supports IJK Grid or UnstructuredGrid as supporting representation of subrepresentation  (for uuid: " + w_uuid +  ")\n").c_str());
        }
    }
    _nodeIdToMapper[p_nodeId] = w_caotvpds;
    try
    { // load representation
        _nodeIdToMapper[p_nodeId]->loadVtkObject();
        return;
    }
    catch (const std::exception& e)
    {
        vtkOutputWindowDisplayErrorText(("Error when rendering uuid: " + w_uuid + "\n" + e.what()).c_str());
        return;
    }
}

void ResqmlDataRepositoryToVtkPartitionedDataSetCollection::loadWellboreTrajectoryMapper(const int p_nodeId)
{
    const std::string w_uuid = std::string(_output->GetDataAssembly()->GetNodeName(p_nodeId)).substr(1);
    COMMON_NS::AbstractObject* const w_abstractObject = _repository->getDataObjectByUuid(w_uuid);


    if (dynamic_cast<RESQML2_NS::WellboreTrajectoryRepresentation*>(w_abstractObject) != nullptr)
    {
        _nodeIdToMapper[p_nodeId] = new ResqmlWellboreTrajectoryToVtkPolyData(static_cast<RESQML2_NS::WellboreTrajectoryRepresentation*>(w_abstractObject));
    } else {
        vtkOutputWindowDisplayErrorText(("Error object type in vtkDataAssembly for uuid: " + w_uuid + "\n").c_str());
        return;
    }
    try
    { // load representation
        _nodeIdToMapper[p_nodeId]->loadVtkObject();
        return;
    }
    catch (const std::exception& e)
    {
        vtkOutputWindowDisplayErrorText(("Error when rendering uuid: " + w_uuid + "\n" + e.what()).c_str());
        return;
    }
}

void ResqmlDataRepositoryToVtkPartitionedDataSetCollection::addDataToParent(const TreeViewNodeType p_type, const int p_nodeId, const int p_nbProcess, const int p_processId, const double p_time)
{
    const std::string w_uuid = std::string(_output->GetDataAssembly()->GetNodeName(p_nodeId)).substr(1);

    // search representation NodeId
    int w_nodeParent = _output->GetDataAssembly()->GetParent(p_nodeId);
    int value_type;
    _output->GetDataAssembly()->GetAttribute(w_nodeParent, "type", value_type);
    TreeViewNodeType w_typeParent = static_cast<TreeViewNodeType>(value_type);
    while (w_typeParent == TreeViewNodeType::Collection)
    {
        w_nodeParent = _output->GetDataAssembly()->GetParent(w_nodeParent);
        value_type;
        _output->GetDataAssembly()->GetAttribute(w_nodeParent, "type", value_type);
        w_typeParent = static_cast<TreeViewNodeType>(value_type);
    }

    if (TreeViewNodeType::Perforation == p_type)
    {
        try
        {
            if (static_cast<WitsmlWellboreCompletionToVtkPartitionedDataSet*>(_nodeIdToMapperSet[w_nodeParent]))
            {
                const char* w_connection;
                _output->GetDataAssembly()->GetAttribute(p_nodeId, "connection", w_connection);
                const char* w_name;
                _output->GetDataAssembly()->GetAttribute(p_nodeId, "label", w_name);
                const char* w_skin_s;
                _output->GetDataAssembly()->GetAttribute(p_nodeId, "skin", w_skin_s);
                const double w_skin = std::strtod(w_skin_s, nullptr);
                if (!_nodeIdToMapperSet[w_nodeParent]->existUuid(w_connection))
                {
                    (static_cast<WitsmlWellboreCompletionToVtkPartitionedDataSet*>(_nodeIdToMapperSet[w_nodeParent]))->addPerforation(w_connection, w_name, w_skin);
                }
                else
                {
                    vtkOutputWindowDisplayErrorText(("Error object type in vtkDataAssembly for uuid: " + w_uuid + "\n").c_str());
                }
            }
        }
        catch (const std::exception& e)
        {
            vtkOutputWindowDisplayErrorText(("Error when initialize uuid: " + w_uuid + "\n" + e.what()).c_str());
        }
        return;
    }
    else if (TreeViewNodeType::WellboreChannel == p_type)
    {
        try
        {
            COMMON_NS::AbstractObject *const w_result = _repository->getDataObjectByUuid(w_uuid);
            if (static_cast<ResqmlWellboreFrameToVtkPartitionedDataSet*>(_nodeIdToMapperSet[w_nodeParent]))
            {
                if (!_nodeIdToMapperSet[w_nodeParent]->existUuid(w_uuid))
                {
                    if (dynamic_cast<RESQML2_NS::AbstractValuesProperty*>(w_result) != nullptr)
                    {
                        (static_cast<ResqmlWellboreFrameToVtkPartitionedDataSet*>(_nodeIdToMapperSet[w_nodeParent]))->addChannel(w_uuid, static_cast<resqml2::AbstractValuesProperty*>(w_result));
                    }
                }
            }
            else
            {
                vtkOutputWindowDisplayErrorText(("Error object type in vtkDataAssembly for uuid: " + w_uuid + "\n").c_str());
            }
        }
        catch (const std::exception& e)
        {
            vtkOutputWindowDisplayErrorText(("Error when initialize uuid: " + w_uuid + "\n" + e.what()).c_str());
        }
        return;
    }
    else if (TreeViewNodeType::WellboreMarker == p_type)
    {
        try
        {
            if (static_cast<ResqmlWellboreMarkerFrameToVtkPartitionedDataSet*>(_nodeIdToMapperSet[w_nodeParent]))
            {
                const std::string w_uuidParent = std::string(_output->GetDataAssembly()->GetNodeName(w_nodeParent)).substr(1);
                resqml2::WellboreMarkerFrameRepresentation* w_markerFrame = _repository->getDataObjectByUuid<resqml2::WellboreMarkerFrameRepresentation>(w_uuidParent);
                if (!_nodeIdToMapperSet[w_nodeParent]->existUuid(w_uuid))
                {
                    resqml2::WellboreMarker* const w_marker = _repository->getDataObjectByUuid<resqml2::WellboreMarker>(w_uuid);
                    (static_cast<ResqmlWellboreMarkerFrameToVtkPartitionedDataSet*>(_nodeIdToMapperSet[w_nodeParent]))->addMarker(w_markerFrame, w_uuid, _markerOrientation, _markerSize);
                }
                else
                {
                    (static_cast<ResqmlWellboreMarkerFrameToVtkPartitionedDataSet*>(_nodeIdToMapperSet[w_nodeParent]))->changeOrientationAndSize(w_uuid, _markerOrientation, _markerSize);
                }
            }
            else
            {
                vtkOutputWindowDisplayErrorText(("Error object type in vtkDataAssembly for uuid: " + w_uuid + "\n").c_str());
            }
        }
        catch (const std::exception& e)
        {
            vtkOutputWindowDisplayErrorText(("Error when initialize uuid: " + w_uuid + "\n" + e.what()).c_str());
        }
        return;
    }
    else if (TreeViewNodeType::Properties == p_type)
    {
        try
        {
            if (static_cast<ResqmlAbstractRepresentationToVtkPartitionedDataSet*>(_nodeIdToMapper[w_nodeParent]))
            {
                    static_cast<ResqmlAbstractRepresentationToVtkPartitionedDataSet*>(_nodeIdToMapper[w_nodeParent])->addDataArray(w_uuid);
            }
            else
            {
                vtkOutputWindowDisplayErrorText(("Error representation for property uuid: " + w_uuid + " not exist\n").c_str());
            }
        }
        catch (const std::exception& e)
        {
            vtkOutputWindowDisplayErrorText(("Error when initialize uuid: " + w_uuid + "\n" + e.what()).c_str());
        }
    }
    else if (TreeViewNodeType::TimeSeries == p_type)
    {
        try
        {
            std::string w_tsUuid = w_uuid.substr(0, 36);
            std::string w_nodeName = w_uuid.substr(36);

            auto const* assembly = _output->GetDataAssembly();
            if (_nodeIdToMapper[w_nodeParent])
            {
                static_cast<ResqmlAbstractRepresentationToVtkPartitionedDataSet*>(_nodeIdToMapper[w_nodeParent])->addDataArray(_timeSeriesUuidAndTitleToIndexAndPropertiesUuid[w_tsUuid][w_nodeName][p_time]);
            }
        }
        catch (const std::exception& e)
        {
            vtkOutputWindowDisplayErrorText(("Error when load Time Series property marker uuid: " + w_uuid + "\n" + e.what()).c_str());
        }
        return;
    }
}

/**
 * delete oldSelection mapper
 */
void ResqmlDataRepositoryToVtkPartitionedDataSetCollection::deleteMapper(double p_time)
{
    // initialization the output (VtkPartitionedDatasSetCollection) with same vtkDataAssembly
    vtkSmartPointer<vtkDataAssembly> w_Assembly = vtkSmartPointer<vtkDataAssembly>::New();
    w_Assembly->DeepCopy(_output->GetDataAssembly());
    _output = vtkSmartPointer<vtkPartitionedDataSetCollection>::New();
    _output->SetDataAssembly(w_Assembly);

    // delete unchecked object
    for (const int w_nodeId : _oldSelection)
    {
        // retrieval of object type for nodeid
        int w_valueType;
        w_Assembly->GetAttribute(w_nodeId, "type", w_valueType);
        TreeViewNodeType valueType = static_cast<TreeViewNodeType>(w_valueType);

        // retrieval of object UUID for nodeId
        const std::string uuid_unselect = std::string(w_Assembly->GetNodeName(w_nodeId)).substr(1);

        if (valueType == TreeViewNodeType::TimeSeries)
        { // TimeSerie properties deselection
            std::string w_timeSeriesuuid = uuid_unselect.substr(0, 36);
            std::string w_nodeName = uuid_unselect.substr(36);

            const int w_nodeParent = w_Assembly->GetParent(w_Assembly->FindFirstNodeWithName(("_" + uuid_unselect).c_str()));
            if (_nodeIdToMapper.find(w_nodeParent) != _nodeIdToMapper.end())
            {
                static_cast<ResqmlAbstractRepresentationToVtkPartitionedDataSet*>(_nodeIdToMapper[w_nodeParent])->deleteDataArray(_timeSeriesUuidAndTitleToIndexAndPropertiesUuid[w_timeSeriesuuid][w_nodeName][p_time]);
            }
        }
        else if (valueType == TreeViewNodeType::Properties)
        {
            int w_nodeParent = _output->GetDataAssembly()->GetParent(w_nodeId);
            int w_typeValue;
            _output->GetDataAssembly()->GetAttribute(w_nodeParent, "type", w_typeValue);
            TreeViewNodeType w_typeParent = static_cast<TreeViewNodeType>(w_typeValue);
            while (w_typeParent == TreeViewNodeType::Collection)
            {
                w_nodeParent = _output->GetDataAssembly()->GetParent(w_nodeParent);
                w_typeValue;
                _output->GetDataAssembly()->GetAttribute(w_nodeParent, "type", w_typeValue);
                w_typeParent = static_cast<TreeViewNodeType>(w_typeValue);
            }

            try
            {
                if (_nodeIdToMapper.find(w_nodeParent) != _nodeIdToMapper.end())
                {
                    static_cast<ResqmlAbstractRepresentationToVtkPartitionedDataSet*>(_nodeIdToMapper[w_nodeParent])->deleteDataArray(std::string(w_Assembly->GetNodeName(w_nodeId)).substr(1));
                }
            }
            catch (const std::exception &e)
            {
                vtkOutputWindowDisplayErrorText(("Error in property unload for uuid: " + uuid_unselect + "\n" + e.what()).c_str());
            }
        }
        else if (valueType == TreeViewNodeType::WellboreMarker ||
            valueType == TreeViewNodeType::WellboreChannel)
        {
            const int w_nodeParent = w_Assembly->GetParent(w_nodeId);
            try
            {
                if (_nodeIdToMapperSet.find(w_nodeParent) != _nodeIdToMapperSet.end())
                {
                    _nodeIdToMapperSet[w_nodeParent]->removeCommonAbstractObjectToVtkPartitionedDataSet(std::string(w_Assembly->GetNodeName(w_nodeId)).substr(1));
                }
            }
            catch (const std::exception &e)
            {
                vtkOutputWindowDisplayErrorText(("Error in property unload for uuid: " + uuid_unselect + "\n" + e.what()).c_str());
            }
        }
        else if (valueType == TreeViewNodeType::SubRepresentation)
        {
            try
            {
                if (_nodeIdToMapper.find(w_nodeId) != _nodeIdToMapper.end())
                {
                    std::string uuid_supporting_grid = "";
                    if (dynamic_cast<ResqmlUnstructuredGridSubRepToVtkUnstructuredGrid *>(_nodeIdToMapper[w_nodeId]) != nullptr)
                    {
                        uuid_supporting_grid = static_cast<ResqmlUnstructuredGridSubRepToVtkUnstructuredGrid *>(_nodeIdToMapper[w_nodeId])->unregisterToMapperSupportingGrid();
                    }
                    else if (dynamic_cast<ResqmlIjkGridSubRepToVtkExplicitStructuredGrid *>(_nodeIdToMapper[w_nodeId]) != nullptr)
                    {
                        uuid_supporting_grid = static_cast<ResqmlIjkGridSubRepToVtkExplicitStructuredGrid *>(_nodeIdToMapper[w_nodeId])->unregisterToMapperSupportingGrid();
                    }
                    delete _nodeIdToMapper[w_nodeId];
                    _nodeIdToMapper.erase(w_nodeId);
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
        else if (valueType == TreeViewNodeType::Representation ||
                 valueType == TreeViewNodeType::WellboreTrajectory)
        {
            try
            {
                if (_nodeIdToMapper.find(w_nodeId) != _nodeIdToMapper.end())
                {
                    delete _nodeIdToMapper[w_nodeId];
                    _nodeIdToMapper.erase(w_nodeId);
                }
            }
            catch (const std::exception &e)
            {
                vtkOutputWindowDisplayErrorText(("Error for unload uuid: " + uuid_unselect + "\n" + e.what()).c_str());
            }
        }
        else if (valueType == TreeViewNodeType::Perforation)
        // delete child of CommonAbstractObjectSetToVtkPartitionedDataSetSet
        {
            const int w_nodeParent = w_Assembly->GetParent(w_nodeId);
            try
            {
                if (_nodeIdToMapperSet.find(w_nodeParent) != _nodeIdToMapperSet.end())
                {
                    const char *w_connection;
                    _output->GetDataAssembly()->GetAttribute(w_nodeId, "connection", w_connection);
                    _nodeIdToMapperSet[w_nodeParent]->removeCommonAbstractObjectToVtkPartitionedDataSet(w_connection);
                }
            }
            catch (const std::exception &e)
            {
                vtkOutputWindowDisplayErrorText(("Error in WellboreCompletion unload for uuid: " + uuid_unselect + "\n" + e.what()).c_str());
            }
        }
        else if (valueType == TreeViewNodeType::WellboreCompletion ||
            valueType == TreeViewNodeType::WellboreMarkerFrame ||
            valueType == TreeViewNodeType::WellboreFrame)
        {
            try
            {
                if (_nodeIdToMapperSet.find(w_nodeId) != _nodeIdToMapperSet.end())
                {
                    delete _nodeIdToMapperSet[w_nodeId];
                    _nodeIdToMapperSet.erase(w_nodeId);
                }
            }
            catch (const std::exception &e)
            {
                vtkOutputWindowDisplayErrorText(("Error in WellboreCompletion unload for uuid: " + uuid_unselect + "\n" + e.what()).c_str());
            }
        }
    }
}

vtkPartitionedDataSetCollection *ResqmlDataRepositoryToVtkPartitionedDataSetCollection::getVtkPartitionedDatasSetCollection(const double p_time, const int p_nbProcess, const int p_processId)
{
    deleteMapper(p_time);

    // vtkParitionedDataSetCollection - hierarchy - build
    // foreach selection node init object
    auto w_it = _currentSelection.begin();
    while (w_it != _currentSelection.end())
    {
        int w_typeValue;
        _output->GetDataAssembly()->GetAttribute(*w_it, "type", w_typeValue);
        TreeViewNodeType w_type = static_cast<TreeViewNodeType>(w_typeValue);

        // init MapperSet && save nodeId for attach to vtkPartitionedDataSetcollection
        if (getMapperType(w_type) == MapperType::MapperSet)
        {
            // initialize mapperSet with nodeId
            if (_nodeIdToMapperSet.find(*w_it) == _nodeIdToMapperSet.end())
            {
                initMapperSet(w_type, *w_it, p_nbProcess, p_processId);
            }
            ++w_it;
        }
        else if (getMapperType(w_type) == MapperType::Mapper)
        {
            // load mapper with nodeId
            if (_nodeIdToMapper.find(*w_it) == _nodeIdToMapper.end())
            {
                loadMapper(w_type, *w_it, p_nbProcess, p_processId);
            }
            ++w_it;
        }
        else if (getMapperType(w_type) == MapperType::Folder)
        {
            ++w_it;
        }
        else if (getMapperType(w_type) == MapperType::Data)
        {
            addDataToParent(w_type, *w_it, p_nbProcess, p_processId, p_time);
            ++w_it;
        }
    }

    unsigned int w_PartitionIndex = 0; 
    // foreach selection node load object
    for (const int w_nodeSelection : _currentSelection)
    {
        int w_typeValue;
        _output->GetDataAssembly()->GetAttribute(w_nodeSelection, "type", w_typeValue);
        TreeViewNodeType w_type = static_cast<TreeViewNodeType>(w_typeValue);

        if (getMapperType(w_type) == MapperType::MapperSet)
        {
            // load mapper representation
            if (_nodeIdToMapperSet.find(w_nodeSelection) != _nodeIdToMapperSet.end())
            {
                try
                {
                    _nodeIdToMapperSet[w_nodeSelection]->loadVtkObject();
                }
                catch (const std::exception& e)
                    {
                        vtkOutputWindowDisplayErrorText(("Fesapi Error for uuid : " + std::string(_output->GetDataAssembly()->GetNodeName(w_nodeSelection)).substr(1) + "\n" + e.what()).c_str());
                    }
                for (auto partition : _nodeIdToMapperSet[w_nodeSelection]->getMapperSet())
                {
                    _output->SetPartitionedDataSet(w_PartitionIndex, partition->getOutput());
                    _output->GetMetaData(w_PartitionIndex)->Set(vtkCompositeDataSet::NAME(), partition->getTitle() + '(' + partition->getUuid() + ')');
                    GetAssembly()->AddDataSetIndex(w_nodeSelection, w_PartitionIndex + 1); // attach hierarchy to assembly
                    w_PartitionIndex++;
                }
            }
        }
        else if (getMapperType(w_type) == MapperType::Mapper)
        {
            // load mapper representation
            if (_nodeIdToMapper.find(w_nodeSelection) != _nodeIdToMapper.end())
            {
                _output->SetPartitionedDataSet(w_PartitionIndex, _nodeIdToMapper[w_nodeSelection]->getOutput());
                _output->GetMetaData(w_PartitionIndex)->Set(vtkCompositeDataSet::NAME(), _nodeIdToMapper[w_nodeSelection]->getTitle() + '(' + _nodeIdToMapper[w_nodeSelection]->getUuid() + ')');
                GetAssembly()->AddDataSetIndex(w_nodeSelection, w_PartitionIndex + 1); // attach hierarchy to assembly
                w_PartitionIndex++;
            }
        }
    }

    _output->Modified();
    return _output;
}

void ResqmlDataRepositoryToVtkPartitionedDataSetCollection::setMarkerOrientation(bool orientation)
{
    _markerOrientation = orientation;
}

void ResqmlDataRepositoryToVtkPartitionedDataSetCollection::setMarkerSize(int size)
{
    _markerSize = size;
}
