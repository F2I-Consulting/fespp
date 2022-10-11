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
#include <regex>

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

#ifdef WITH_ETP_SSL
#include <thread>

#include <fetpapi/etp/fesapi/FesapiHdfProxy.h>

#include <fetpapi/etp/ProtocolHandlers/DataspaceHandlers.h>
#include <fetpapi/etp/ProtocolHandlers/DiscoveryHandlers.h>
#include <fetpapi/etp/ProtocolHandlers/StoreHandlers.h>
#include <fetpapi/etp/ProtocolHandlers/DataArrayHandlers.h>
#endif
#include <fesapi/common/EpcDocument.h>

#include "ResqmlIjkGridToVtkUnstructuredGrid.h"
#include "ResqmlIjkGridSubRepToVtkUnstructuredGrid.h"
#include "ResqmlGrid2dToVtkStructuredGrid.h"
#include "ResqmlPolylineToVtkPolyData.h"
#include "ResqmlTriangulatedSetToVtkPartitionedDataSet.h"
#include "ResqmlUnstructuredGridToVtkUnstructuredGrid.h"
#include "ResqmlUnstructuredGridSubRepToVtkUnstructuredGrid.h"
#include "ResqmlWellboreTrajectoryToVtkPolyData.h"
#include "ResqmlWellboreMarkerFrameToVtkPartitionedDataSet.h"
#include "ResqmlWellboreFrameToVtkPartitionedDataSet.h"

ResqmlDataRepositoryToVtkPartitionedDataSetCollection::ResqmlDataRepositoryToVtkPartitionedDataSetCollection() : markerOrientation(false),
                                                                                                                 markerSize(10),
                                                                                                                 repository(new common::DataObjectRepository()),
                                                                                                                 output(vtkSmartPointer<vtkPartitionedDataSetCollection>::New()),
                                                                                                                 nodeId_to_uuid(),
                                                                                                                 nodeId_to_resqml(),
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
    for (const auto &keyVal : nodeId_to_resqml)
    {
        delete keyVal.second;
    }
}

//----------------------------------------------------------------------------
std::string ResqmlDataRepositoryToVtkPartitionedDataSetCollection::connect(const std::string etp_url, const std::string data_partition, const std::string auth_connection)
{
#ifdef WITH_ETP_SSL
    boost::uuids::random_generator gen;
    ETP_NS::InitializationParameters initializationParams(gen(), etp_url);

	std::map<std::string, std::string> additionalHandshakeHeaderFields = { {"data-partition-id" , data_partition} };
	if (initializationParams.getPort() == 80) {
		session = ETP_NS::ClientSessionLaunchers::createWsClientSession(&initializationParams, auth_connection, additionalHandshakeHeaderFields);
	}
	else {
		session = ETP_NS::ClientSessionLaunchers::createWssClientSession(&initializationParams, auth_connection, additionalHandshakeHeaderFields);
	}
	session->setDataspaceProtocolHandlers(std::make_shared<ETP_NS::DataspaceHandlers>(session.get()));
    session->setDiscoveryProtocolHandlers(std::make_shared<ETP_NS::DiscoveryHandlers>(session.get()));
	session->setStoreProtocolHandlers(std::make_shared<ETP_NS::StoreHandlers>(session.get()));
	session->setDataArrayProtocolHandlers(std::make_shared<ETP_NS::DataArrayHandlers>(session.get()));

    repository->setHdfProxyFactory(new ETP_NS::FesapiHdfProxyFactory(session.get()));

	auto plainSession = std::dynamic_pointer_cast<ETP_NS::PlainClientSession>(session);
	if (initializationParams.getPort() == 80) {
		std::thread sessionThread(&ETP_NS::PlainClientSession::run, plainSession);
		sessionThread.detach();
	}
	else {
		auto sslSession = std::dynamic_pointer_cast<ETP_NS::SslClientSession>(session);
		std::thread sessionThread(&ETP_NS::SslClientSession::run, sslSession);
		sessionThread.detach();
	}

	// Wait for the ETP session to be opened
	auto t_start = std::chrono::high_resolution_clock::now();
	// the work...
	auto t_end = std::chrono::high_resolution_clock::now();
	while (session->isEtpSessionClosed()) {
		if (std::chrono::duration<double, std::milli>(std::chrono::high_resolution_clock::now() - t_start).count() > 5000) {
			vtkOutputWindowDisplayWarningText(("Time out for websocket connection : " +
				std::to_string(std::chrono::duration<double, std::milli>(std::chrono::high_resolution_clock::now() - t_start).count()) + "ms\n").c_str());
			return "ETP time out";
		}
	}

	//************ LIST DATASPACES ************
	const auto dataspaces = session->getDataspaces();
	//************ LIST RESOURCES ************
	const auto dsIter = std::find_if(dataspaces.begin(), dataspaces.end(),
		[](const Energistics::Etp::v12::Datatypes::Object::Dataspace& ds) { return ds.uri == "eml:///dataspace('DANI/DEMO_F2F')"; });
	Energistics::Etp::v12::Datatypes::Object::ContextInfo ctxInfo;
	ctxInfo.uri = dsIter == dataspaces.end()
		? dataspaces[0].uri
		: dsIter->uri;
	ctxInfo.depth = 0;
	ctxInfo.navigableEdges = Energistics::Etp::v12::Datatypes::Object::RelationshipKind::Both;
	ctxInfo.includeSecondaryTargets = false;
	ctxInfo.includeSecondarySources = false;
	const auto resources = session->getResources(ctxInfo, Energistics::Etp::v12::Datatypes::Object::ContextScopeKind::targets);
	//************ GET ALL DATAOBJECTS ************
	repository->setHdfProxyFactory(new ETP_NS::FesapiHdfProxyFactory(session.get()));
	if (!resources.empty()) {
		std::map< std::string, std::string > query;
		for (size_t i = 0; i < resources.size(); ++i) {
			query[std::to_string(i)] = resources[i].uri;
		}
		const auto dataobjects = session->getDataObjects(query);
		for (auto& datoObject : dataobjects) {
			repository->addOrReplaceGsoapProxy(datoObject.second.data,
				ETP_NS::EtpHelpers::getDataObjectType(datoObject.second.resource.uri),
				ETP_NS::EtpHelpers::getDataspaceUri(datoObject.second.resource.uri));
		}
	}
	else {
		std::cout << "There is no dataobject in this dataspace" << std::endl;
	}
#endif
    vtkOutputWindowDisplayWarningText(("connection with paramater: " + etp_url + " data partition: " + data_partition + " authentication " + auth_connection + "\n").c_str());
    return buildDataAssemblyFromDataObjectRepo("");
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

std::string ResqmlDataRepositoryToVtkPartitionedDataSetCollection::buildDataAssemblyFromDataObjectRepo(const char *fileName)
{
    std::string message;

    // create vtkDataAssembly: create treeView in property panel
    // get grid2D + subrepresentation + property
    message += searchGrid2d(fileName);
    // get ijkGrid + subrepresentation + property
    message += searchIjkGrid(fileName);
    // get polylines + subrepresentation + property
    message += searchPolylines(fileName);
    // get triangulated + subrepresentation + property
    message += searchTriangulated(fileName);
    // get unstructuredGrid + subrepresentation + property
    message += searchUnstructuredGrid(fileName);
    // get WellboreTrajectory + subrepresentation + property
    message += searchWellboreTrajectory(fileName);

    // get TimeSeries
    message += searchTimeSeries(fileName);

    return message;
}

std::string ResqmlDataRepositoryToVtkPartitionedDataSetCollection::searchPolylines(const std::string &fileName)
{
    std::string result;

    try
    {
        auto polylineSetSet = repository->getAllPolylineSetRepresentationSet();
        std::sort(polylineSetSet.begin(), polylineSetSet.end(),
                  [](const RESQML2_NS::PolylineSetRepresentation *a, const RESQML2_NS::PolylineSetRepresentation *b) -> bool
                  {
                      return a->getTitle().compare(b->getTitle()) < 0;
                  });
        for (auto const *polylineSet : polylineSetSet)
        {
            // add full represnetation
            if (searchNodeByUuid(polylineSet->getUuid()) == -1)
            { // verify uuid exist in treeview
                result += searchRepresentations(
                    repository->getDataObjectByUuid<resqml2::AbstractRepresentation>(polylineSet->getUuid()));
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

    try
    {
        auto unstructuredGridSet = repository->getUnstructuredGridRepresentationSet();
        std::sort(unstructuredGridSet.begin(), unstructuredGridSet.end(),
                  [](const RESQML2_NS::UnstructuredGridRepresentation *a, const RESQML2_NS::UnstructuredGridRepresentation *b) -> bool
                  {
                      return a->getTitle().compare(b->getTitle()) < 0;
                  });
        for (auto const *unstructuredGrid : unstructuredGridSet)
        {
            if (searchNodeByUuid(unstructuredGrid->getUuid()) == -1)
            { // verify uuid exist in treeview
                result += searchRepresentations(
                    repository->getDataObjectByUuid<resqml2::AbstractRepresentation>(unstructuredGrid->getUuid()));
            }
        }
    }
    catch (const std::exception &e)
    {
        return "EXCEPTION in fesapi when calling getUnstructuredGridRepresentationSet with file: " + fileName + " : " + e.what();
    }
    return result;
}

std::string ResqmlDataRepositoryToVtkPartitionedDataSetCollection::searchTriangulated(const std::string &fileName)
{
    std::string result;

    try
    {
        auto triangulatedSet = repository->getAllTriangulatedSetRepresentationSet();
        std::sort(triangulatedSet.begin(), triangulatedSet.end(),
                  [](const RESQML2_NS::TriangulatedSetRepresentation *a, const RESQML2_NS::TriangulatedSetRepresentation *b) -> bool
                  {
                      return a->getTitle().compare(b->getTitle()) < 0;
                  });
        for (auto const *triangulated : triangulatedSet)
        {
            if (searchNodeByUuid(triangulated->getUuid()) == -1)
            { // verify uuid exist in treeview
                result += searchRepresentations(
                    repository->getDataObjectByUuid<resqml2::AbstractRepresentation>(triangulated->getUuid()));
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

    try
    {
        auto grid2DSet = repository->getHorizonGrid2dRepresentationSet();
        std::sort(grid2DSet.begin(), grid2DSet.end(),
                  [](const RESQML2_NS::Grid2dRepresentation *a, const RESQML2_NS::Grid2dRepresentation *b) -> bool
                  {
                      return a->getTitle().compare(b->getTitle()) < 0;
                  });
        for (auto const *grid2D : grid2DSet)
        {
            if (searchNodeByUuid(grid2D->getUuid()) == -1)
            { // verify uuid exist in treeview (generally when we load another epcfile in same repository)
                result += searchRepresentations(
                    repository->getDataObjectByUuid<resqml2::AbstractRepresentation>(grid2D->getUuid()));
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

    try
    {
        auto ijkGridSet = repository->getIjkGridRepresentationSet();
        std::sort(ijkGridSet.begin(), ijkGridSet.end(),
                  [](const RESQML2_NS::AbstractIjkGridRepresentation *a, const RESQML2_NS::AbstractIjkGridRepresentation *b) -> bool
                  {
                      return a->getTitle().compare(b->getTitle()) < 0;
                  });
        for (auto const *ijkGrid : ijkGridSet)
        {
            if (searchNodeByUuid(ijkGrid->getUuid()) == -1)
            { // verify uuid exist in treeview (generally when we load another epcfile in same repository)
                result += searchRepresentations(
                    repository->getDataObjectByUuid<resqml2::AbstractRepresentation>(ijkGrid->getUuid()));
            }
        }
    }
    catch (const std::exception &e)
    {
        return "EXCEPTION in fesapi when calling getIjkGridRepresentationSet with file: " + fileName + " : " + e.what();
    }
    return result;
}

std::string ResqmlDataRepositoryToVtkPartitionedDataSetCollection::searchRepresentations(resqml2::AbstractRepresentation *representation, int idNode)
{
    std::string result = "";
    vtkDataAssembly *data_assembly = output->GetDataAssembly();
    // idNode = addNodeGroup(representation->getXmlTag(), idNode);

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
        representation->setTitle(changeInvalidCharacter(representation->getXmlTag() + "_" + representation->getTitle()));
        idNode = data_assembly->AddNode(representation->getTitle().c_str(), idNode);
        nodeId_to_uuid[idNode] = representation->getUuid();
    }

    // add sub representation with properties (only for ijkGrid)
    if (dynamic_cast<RESQML2_NS::AbstractIjkGridRepresentation *>(representation) != nullptr ||
        dynamic_cast<RESQML2_NS::UnstructuredGridRepresentation *>(representation) != nullptr)
    {
        result += searchSubRepresentation(representation, data_assembly, idNode);
    }

    // add properties representation
    result += searchProperties(representation, data_assembly, idNode);

    return result;
}

int ResqmlDataRepositoryToVtkPartitionedDataSetCollection::addNodeGroup(std::string name, int idNode)
{
    if (idNode == 0)
    { // root
        vtkDataAssembly *data_assembly = output->GetDataAssembly();

        for (std::map<int, std::string>::iterator it = nodeId_to_uuid.begin(); it != nodeId_to_uuid.end(); ++it)
        {
            if (name == it->second)
            {
                return it->first;
            }
        }

        idNode = data_assembly->AddNode(name.c_str(), 0);
        nodeId_to_uuid[idNode] = name;
    }
    return idNode;
}

std::string ResqmlDataRepositoryToVtkPartitionedDataSetCollection::searchSubRepresentation(resqml2::AbstractRepresentation *representation, vtkDataAssembly *data_assembly, int node_parent)
{
    std::string result;

    try
    {
        auto subRepresentationSet = representation->getSubRepresentationSet();
        std::sort(subRepresentationSet.begin(), subRepresentationSet.end(),
                  [](const RESQML2_NS::SubRepresentation *a, const RESQML2_NS::SubRepresentation *b) -> bool
                  {
                      return a->getTitle().compare(b->getTitle()) < 0;
                  });
        for (auto const *subRepresentation : subRepresentationSet)
        {
            if (searchNodeByUuid(subRepresentation->getUuid()) == -1)
            { // verify uuid exist in treeview
                auto absRepresentation = repository->getDataObjectByUuid<resqml2::AbstractRepresentation>(subRepresentation->getUuid());
                absRepresentation->setTitle(changeInvalidCharacter(representation->getTitle() + "-" + subRepresentation->getTitle()));
                result += searchRepresentations(absRepresentation, data_assembly->GetParent(node_parent));
            }
        }
    }
    catch (const std::exception &e)
    {
        return "EXCEPTION in fesapi when calling getSubRepresentationSet for uuid : " + representation->getUuid() + " : " + e.what() + ".\n";
    }
    return result;
}

std::string ResqmlDataRepositoryToVtkPartitionedDataSetCollection::searchProperties(resqml2::AbstractRepresentation *representation, vtkDataAssembly *data_assembly, int node_parent)
{
    try
    {
        auto valuesPropertySet = representation->getValuesPropertySet();
        std::sort(valuesPropertySet.begin(), valuesPropertySet.end(),
                  [](const RESQML2_NS::AbstractValuesProperty *a, const RESQML2_NS::AbstractValuesProperty *b) -> bool
                  {
                      return a->getTitle().compare(b->getTitle()) < 0;
                  });
        // property
        for (auto *property : valuesPropertySet)
        {
            property->setTitle(changeInvalidCharacter(property->getXmlTag() + '.' + property->getTitle()));

            if (searchNodeByUuid(property->getUuid()) == -1)
            { // verify uuid exist in treeview
                int property_idNode = data_assembly->AddNode(property->getTitle().c_str(), node_parent);
                nodeId_to_uuid[property_idNode] = property->getUuid();
            }
        }
    }
    catch (const std::exception &e)
    {
        return "EXCEPTION in fesapi when calling getValuesPropertySet with representation uuid: " + nodeId_to_uuid[node_parent] + " : " + e.what() + ".\n";
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
                wellboreTrajectory->setTitle(changeInvalidCharacter(wellboreTrajectory->getXmlTag() + '.' + wellboreTrajectory->getTitle()));
                idNode = data_assembly->AddNode(wellboreTrajectory->getTitle().c_str(), 0 /* add to root*/);
                nodeId_to_uuid[idNode] = wellboreTrajectory->getUuid();
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
                    int frame_idNode = data_assembly->AddNode(wellboreFrame->getTitle().c_str(), 0 /* add to root*/);
                    nodeId_to_uuid[frame_idNode] = wellboreFrame->getUuid();
                    for (auto *property : wellboreFrame->getValuesPropertySet())
                    {
                        property->setTitle(changeInvalidCharacter(property->getXmlTag() + '.' + property->getTitle()));
                        int property_idNode = data_assembly->AddNode(property->getTitle().c_str(), frame_idNode);
                        nodeId_to_uuid[property_idNode] = property->getUuid();
                    }
                }
                else
                { // WellboreMarkerFrame
                    wellboreFrame->setTitle(changeInvalidCharacter(wellboreFrame->getXmlTag() + '.' + wellboreFrame->getTitle()));
                    int markerFrame_idNode = data_assembly->AddNode(wellboreFrame->getTitle().c_str(), 0 /* add to root*/);
                    nodeId_to_uuid[markerFrame_idNode] = wellboreFrame->getUuid();
                    for (auto *wellboreMarker : wellboreMarkerFrame->getWellboreMarkerSet())
                    {
                        wellboreMarker->setTitle(changeInvalidCharacter(wellboreMarker->getXmlTag() + '.' + wellboreMarker->getTitle()));
                        int marker_idNode = data_assembly->AddNode(wellboreMarker->getTitle().c_str(), markerFrame_idNode);
                        nodeId_to_uuid[marker_idNode] = wellboreMarker->getUuid();
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
    for (auto const *timeSerie : timeSeries)
    {
        bool valid = true;

        // get properties link to Times series
        try
        {
            auto propSeries = timeSerie->getPropertySet();

            std::vector<std::string> property_name_set;
            std::map<std::string, std::vector<int>> property_name_to_node_set;
            int node_parent = -1;
            for (auto const *prop : propSeries)
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
                        // nodeId_to_EntityType.erase(node);
                        nodeId_to_resqml.erase(node);
                        nodeId_to_uuid.erase(node);
                    }
                    std::string name = changeInvalidCharacter(timeSerie->getXmlTag() + '.' + myPair.first);
                    auto times_serie_node_id = output->GetDataAssembly()->AddNode(name.c_str(), node_parent);

                    // nodeId_to_EntityType[times_serie_node_id] = ResqmlDataRepositoryToVtkPartitionedDataSetCollection::EntityType::TIMES_SERIE;
                    //  for selection add node name to timeserie uuid
                    nodeId_to_uuid[times_serie_node_id] = timeSerie->getUuid() + name;
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

std::string ResqmlDataRepositoryToVtkPartitionedDataSetCollection::selectNodeId(int node)
{
    this->selectNodeIdParent(node);

    this->current_selection.insert(node);
    this->old_selection.erase(node);

    this->selectNodeIdChildren(node);

    return "";
}

void ResqmlDataRepositoryToVtkPartitionedDataSetCollection::selectNodeIdParent(int node)
{
    auto const *assembly = this->output->GetDataAssembly();

    if (assembly->GetParent(node) != -1)
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

ResqmlAbstractRepresentationToVtkDataset *ResqmlDataRepositoryToVtkPartitionedDataSetCollection::loadToVtk(std::string uuid, double time)
{

    if (uuid == "") // root
    {
        return nullptr;
    }
    if (uuid.length() > 36)
    { // Time Series Properties
        std::string ts_uuid = uuid.substr(0, 36);
        std::string node_name = uuid.substr(36);

        auto const *assembly = this->output->GetDataAssembly();
        const int node_parent = assembly->GetParent(searchNodeByUuid(uuid));
        if (nodeId_to_resqml[node_parent])
        {
            nodeId_to_resqml[node_parent]->addDataArray(timeSeries_uuid_and_title_to_index_and_properties_uuid[ts_uuid][node_name][time]);
        }
        return nullptr;
    }
    COMMON_NS::AbstractObject *const result = repository->getDataObjectByUuid(uuid);

    ResqmlAbstractRepresentationToVtkDataset *rep = nullptr;
    try
    {
        if (dynamic_cast<RESQML2_NS::AbstractIjkGridRepresentation *>(result) != nullptr)
        {
            rep = new ResqmlIjkGridToVtkUnstructuredGrid(static_cast<RESQML2_NS::AbstractIjkGridRepresentation *>(result));
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
                if (this->nodeId_to_resqml.find(searchNodeByUuid(supportingGrid->getUuid())) == this->nodeId_to_resqml.end())
                {
                    this->nodeId_to_resqml[searchNodeByUuid(supportingGrid->getUuid())] = new ResqmlIjkGridToVtkUnstructuredGrid(supportingGrid);
                }
                rep = new ResqmlIjkGridSubRepToVtkUnstructuredGrid(subRep, dynamic_cast<ResqmlIjkGridToVtkUnstructuredGrid *>(this->nodeId_to_resqml[searchNodeByUuid(supportingGrid->getUuid())]));
            }
            else if (dynamic_cast<RESQML2_NS::UnstructuredGridRepresentation *>(subRep->getSupportingRepresentation(0)) != nullptr)
            {
                auto *supportingGrid = static_cast<RESQML2_NS::UnstructuredGridRepresentation *>(subRep->getSupportingRepresentation(0));
                if (this->nodeId_to_resqml.find(searchNodeByUuid(supportingGrid->getUuid())) == this->nodeId_to_resqml.end())
                {
                    this->nodeId_to_resqml[searchNodeByUuid(supportingGrid->getUuid())] = new ResqmlUnstructuredGridToVtkUnstructuredGrid(supportingGrid);
                }
                rep = new ResqmlUnstructuredGridSubRepToVtkUnstructuredGrid(subRep, dynamic_cast<ResqmlUnstructuredGridToVtkUnstructuredGrid *>(this->nodeId_to_resqml[searchNodeByUuid(supportingGrid->getUuid())]));
            }
        }
        else if (dynamic_cast<RESQML2_NS::WellboreTrajectoryRepresentation *>(result) != nullptr)
        {
            rep = new ResqmlWellboreTrajectoryToVtkPolyData(static_cast<RESQML2_NS::WellboreTrajectoryRepresentation *>(result));
        }
        else if (dynamic_cast<RESQML2_NS::WellboreMarkerFrameRepresentation *>(result) != nullptr)
        {
            rep = new ResqmlWellboreMarkerFrameToVtkPartitionedDataSet(static_cast<RESQML2_NS::WellboreMarkerFrameRepresentation *>(result));
        }
        else if (dynamic_cast<RESQML2_NS::WellboreFrameRepresentation *>(result) != nullptr)
        {
            rep = new ResqmlWellboreFrameToVtkPartitionedDataSet(static_cast<RESQML2_NS::WellboreFrameRepresentation *>(result));
        }
        // add return rep ??
        else if (dynamic_cast<RESQML2_NS::WellboreMarker *>(result) != nullptr)
        {
            auto const *assembly = this->output->GetDataAssembly();
            const int node_parent = assembly->GetParent(searchNodeByUuid(uuid));
            static_cast<ResqmlWellboreMarkerFrameToVtkPartitionedDataSet *>(nodeId_to_resqml[node_parent])->addMarker(uuid, markerOrientation, markerSize);
            return nullptr;
        }
        else if (dynamic_cast<RESQML2_NS::AbstractValuesProperty *>(result) != nullptr)
        {
            auto const *assembly = this->output->GetDataAssembly();
            const int node_parent = assembly->GetParent(searchNodeByUuid(uuid));
            if (dynamic_cast<ResqmlWellboreFrameToVtkPartitionedDataSet *>(nodeId_to_resqml[node_parent]) != nullptr)
            {
                static_cast<ResqmlWellboreFrameToVtkPartitionedDataSet *>(nodeId_to_resqml[node_parent])->addChannel(uuid, static_cast<resqml2::AbstractValuesProperty *>(result));
            }
            else if (dynamic_cast<ResqmlTriangulatedSetToVtkPartitionedDataSet *>(nodeId_to_resqml[node_parent]) != nullptr)
            {
                static_cast<ResqmlTriangulatedSetToVtkPartitionedDataSet *>(nodeId_to_resqml[node_parent])->addDataArray(uuid);
            }
            else
            {
                static_cast<ResqmlAbstractRepresentationToVtkDataset *>(nodeId_to_resqml[node_parent])->addDataArray(uuid);
            }
            return nullptr;
        }
    }
    catch (const std::exception &e)
    {
        vtkOutputWindowDisplayErrorText(("Error when initialize uuid: " + uuid + "\n" + e.what()).c_str());
        return nullptr;
    }
    try
    {
        nodeId_to_resqml[searchNodeByUuid(uuid)] = rep;
        rep->loadVtkObject();
        return rep;
    }
    catch (const std::exception &e)
    {
        vtkOutputWindowDisplayErrorText(("Error when rendering uuid: " + uuid + "\n" + e.what()).c_str());
        return nullptr;
    }
    /*
    switch (entity)
    {
    case ResqmlDataRepositoryToVtkPartitionedDataSetCollection::EntityType::TIMES_SERIE:
    {
        try
        {
            // decompose uuid by Timeserie uuid + node name
            std::string ts_uuid = uuid.substr(0, 36);
            std::string node_name = uuid.substr(36);

            auto const *assembly = this->output->GetDataAssembly();
            const int node_parent = assembly->GetParent(searchNodeByUuid(uuid));
            if (nodeId_to_resqml[node_parent])
            {
                nodeId_to_resqml[node_parent]->addDataArray(timeSeries_uuid_and_title_to_index_and_properties_uuid[ts_uuid][node_name][time]);
            }
        }
        catch (const std::exception &e)
        {
            vtkOutputWindowDisplayErrorText(("Error in property load for uuid: " + uuid + "\n" + e.what()).c_str());
        }
        break;
    }
    }
    return nullptr;
    */
}

std::string ResqmlDataRepositoryToVtkPartitionedDataSetCollection::changeInvalidCharacter(std::string text)
{
    std::regex vowel_re("[^a-zA-Z0-9_.-]");

    return std::regex_replace(text, vowel_re, ".");
}

vtkPartitionedDataSetCollection *ResqmlDataRepositoryToVtkPartitionedDataSetCollection::getVtkPartitionedDatasSetCollection(const double time)
{
    // initialization the output (VtkPartitionedDatasSetCollection)
    vtkSmartPointer<vtkDataAssembly> tmp_assembly = vtkSmartPointer<vtkDataAssembly>::New();
    tmp_assembly->DeepCopy(this->output->GetDataAssembly());
    this->output = vtkSmartPointer<vtkPartitionedDataSetCollection>::New();
    this->output->SetDataAssembly(tmp_assembly);

    unsigned int index = 0; // index for PartionedDatasSet

    // delete Property or Wellbore marker
    for (const int selection : this->old_selection)
    {
        COMMON_NS::AbstractObject *const result = repository->getDataObjectByUuid(nodeId_to_uuid[selection]);

        if (result == nullptr)
        {
            vtkOutputWindowDisplayErrorText(("Error in deselection for uuid: " + nodeId_to_uuid[selection] + "\n").c_str());
        }
        else
        {
            if (dynamic_cast<RESQML2_NS::AbstractValuesProperty *>(result) != nullptr)
            {
                try
                {
                    auto const *assembly = this->output->GetDataAssembly();
                    const int node_parent = assembly->GetParent(searchNodeByUuid(nodeId_to_uuid[selection]));
                    if (this->nodeId_to_resqml.find(node_parent) != this->nodeId_to_resqml.end())
                    {
                        ResqmlAbstractRepresentationToVtkDataset *rep = this->nodeId_to_resqml[node_parent];
                        rep->deleteDataArray(nodeId_to_uuid[selection]);
                    }
                }
                catch (const std::exception &e)
                {
                    vtkOutputWindowDisplayErrorText(("Error in property load for uuid: " + nodeId_to_uuid[selection] + "\n" + e.what()).c_str());
                }
            }
            else if (dynamic_cast<RESQML2_NS::WellboreMarker *>(result) != nullptr)
            {
                try
                {
                    auto const *assembly = this->output->GetDataAssembly();
                    const int node_parent = assembly->GetParent(searchNodeByUuid(nodeId_to_uuid[selection]));
                    if (this->nodeId_to_resqml.find(node_parent) != this->nodeId_to_resqml.end())
                    {
                        ResqmlWellboreMarkerFrameToVtkPartitionedDataSet *rep = dynamic_cast<ResqmlWellboreMarkerFrameToVtkPartitionedDataSet *>(this->nodeId_to_resqml[node_parent]);
                        rep->removeMarker(nodeId_to_uuid[selection]);
                    }
                }
                catch (const std::exception &e)
                {
                    vtkOutputWindowDisplayErrorText(("Error in property load for uuid: " + nodeId_to_uuid[selection] + "\n" + e.what()).c_str());
                }
            }
            else
            {
                if (this->nodeId_to_resqml.find(selection) != this->nodeId_to_resqml.end())
                {
                    this->nodeId_to_resqml.erase(selection); // erase object
                }
            }
        }
    }

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
            rep = loadToVtk(nodeId_to_uuid[node_selection], time);
        }
        if (rep)
        {
            this->output->SetPartitionedDataSet(index, rep->getOutput());
            this->output->GetMetaData(index++)->Set(vtkCompositeDataSet::NAME(), this->output->GetDataAssembly()->GetNodeName(node_selection));
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

void ResqmlDataRepositoryToVtkPartitionedDataSetCollection::setMarkerOrientation(bool orientation)
{
    markerOrientation = orientation;
}

void ResqmlDataRepositoryToVtkPartitionedDataSetCollection::setMarkerSize(int size)
{
    markerSize = size;
}
