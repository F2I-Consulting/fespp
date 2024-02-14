/*---------------------vtkEPCReader--------------------------------------------------
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
#include "vtkEPCWriter.h"

#include <array>
#include <vector>

#include <vtkCellArray.h>
#include <vtkCell.h>
#include <vtkCellData.h>
#include <vtkCellType.h>
#include <vtkDataArray.h>
#include <vtkDoubleArray.h>
#include <vtkIdList.h>
#include <vtkInformation.h>
#include <vtkObjectFactory.h>
#include <vtkPartitionedDataSetCollection.h>
#include <vtkPointData.h>
#include <vtkUnstructuredGrid.h>

#include "fesapi/common/EpcDocument.h"
#include "fesapi/eml2/AbstractHdfProxy.h"
#include "fesapi/common/DataObjectRepository.h"
#include "fesapi/resqml2_0_1/DiscreteProperty.h"
#include "fesapi/resqml2/LocalDepth3dCrs.h"
#include "fesapi/resqml2_0_1/ContinuousProperty.h"
#include "fesapi/resqml2_0_1/PropertyKind.h"
#include "fesapi/resqml2_0_1/UnstructuredGridRepresentation.h"

/**
 * The FESPP version as a string
 */
#define FESPP_VERSION_STR "3.2.0"


 //-----------------------------------------------------------------------------
struct EPCWriterInternal
{
	vtkDataObject* input = nullptr;
	vtkUnstructuredGrid* inputUnstructuredGrid = nullptr;

	std::string inputUnstructuredGridName = "";

	RESQML2_0_1_NS::PropertyKind* discretePropertyKind = nullptr;
	RESQML2_0_1_NS::PropertyKind* continousPropertyKind = nullptr;

	int dataType;
};

//-----------------------------------------------------------------------------
vtkStandardNewMacro(vtkEPCWriter);

//-----------------------------------------------------------------------------
vtkEPCWriter::vtkEPCWriter()
	: Internal(new EPCWriterInternal)
{
	SetNumberOfInputPorts(1);
}

//-----------------------------------------------------------------------------
vtkEPCWriter::~vtkEPCWriter()
{
	SetFileName(nullptr);
}

//-----------------------------------------------------------------------------
int vtkEPCWriter::FillInputPortInformation(int port, vtkInformation* info)
{
	info->Remove(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE());
	info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkUnstructuredGrid");
	info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkPartitionedDataSetCollection");
	return 1;
}

//-----------------------------------------------------------------------------
int vtkEPCWriter::RequestData(
	vtkInformation* request, vtkInformationVector** inputVector, vtkInformationVector*)
{
	// make sure the user specified a FileName
	if (!FileName)
	{
		vtkErrorMacro(<< "Please specify FileName to use");
		return 0;
	}

	Internal->dataType = vtkDataObject::GetData(inputVector[0])->GetDataObjectType();
	Internal->input = vtkDataObject::GetData(inputVector[0]);
	WriteData();

	return 1;
}

//------------------------------------------------------------------------------
void vtkEPCWriter::WriteData()
{
	common::EpcDocument epcDoc(FileName);
	common::DataObjectRepository repo;

	common::AbstractObject::setFormat("F2I-CONSULTING", "FESPP", FESPP_VERSION_STR);
	eml2::AbstractHdfProxy* hdfProxy = repo.createHdfProxy("", "Hdf Proxy", epcDoc.getStorageDirectory(), epcDoc.getName() + ".h5", COMMON_NS::DataObjectRepository::openingMode::OVERWRITE);

	resqml2::LocalDepth3dCrs* local3dCrs = repo.createLocalDepth3dCrs("", "Default local CRS", .0, .0, .0, .0, gsoap_resqml2_0_1::eml20__LengthUom::m, "ParaView does not support CRS", gsoap_resqml2_0_1::eml20__LengthUom::m, "ParaView does not support CRS", true);
	repo.setDefaultCrs(local3dCrs);

	if (!Internal->discretePropertyKind)
	{
		Internal->discretePropertyKind = repo.createPropertyKind("", "Discrete PropKind", "Fespp", gsoap_resqml2_0_1::resqml20__ResqmlUom::Euc, gsoap_resqml2_0_1::resqml20__ResqmlPropertyKind::discrete);
	}
	if (!Internal->continousPropertyKind)
	{
		Internal->continousPropertyKind = repo.createPropertyKind("", "Continous PropKind", "Fespp", gsoap_resqml2_0_1::resqml20__ResqmlUom::Euc, gsoap_resqml2_0_1::resqml20__ResqmlPropertyKind::continuous);
	}

	if (Internal->dataType == VTK_UNSTRUCTURED_GRID)
	{
		/**
		* UNSTRUCTUREDGRID
		*/
		Internal->inputUnstructuredGrid = vtkUnstructuredGrid::SafeDownCast(Internal->input);

		RESQML2_NS::UnstructuredGridRepresentation* w_unstructuredGridRepresentation = writeUnstructuredGrid(repo, hdfProxy, local3dCrs);
		writeProperties(repo, hdfProxy, w_unstructuredGridRepresentation);
	}
	else if (Internal->dataType == VTK_PARTITIONED_DATA_SET_COLLECTION)
	{
		vtkPartitionedDataSetCollection* w_vPDSC = vtkPartitionedDataSetCollection::SafeDownCast(Internal->input);
		for (vtkIdType index = 0; index < w_vPDSC->GetNumberOfPartitionedDataSets(); ++index)
		{
			if (w_vPDSC->GetPartitionAsDataObject(index, 0)->GetDataObjectType() == VTK_UNSTRUCTURED_GRID)
			{
				Internal->inputUnstructuredGridName = w_vPDSC->GetMetaData(index)->Get(vtkCompositeDataSet::NAME());
				Internal->inputUnstructuredGrid = vtkUnstructuredGrid::SafeDownCast(w_vPDSC->GetPartitionAsDataObject(index, 0));
				RESQML2_NS::UnstructuredGridRepresentation* w_unstructuredGridRepresentation = writeUnstructuredGrid(repo, hdfProxy, local3dCrs);
				writeProperties(repo, hdfProxy, w_unstructuredGridRepresentation);
			}
		}
	}

	hdfProxy->close();
	epcDoc.serializeFrom(repo);
}

void vtkEPCWriter::writeProperties(COMMON_NS::DataObjectRepository& repo, EML2_NS::AbstractHdfProxy* hdfProxy, RESQML2_NS::UnstructuredGridRepresentation* p_resqmlUnstructuredGrid)
{

	// passer en vtkGenericDataArray pour le type
	vtkPointData* w_pointData = Internal->inputUnstructuredGrid->GetPointData();
	for (int i = 0; i < w_pointData->GetNumberOfArrays(); ++i)
	{
		vtkDataArray* w_array = Internal->inputUnstructuredGrid->GetCellData()->GetArray(i);
		vtkIntArray* w_intArray = vtkIntArray::SafeDownCast(Internal->inputUnstructuredGrid->GetPointData()->GetArray(i));
		if (w_intArray)
		{
			RESQML2_NS::DiscreteProperty* w_prop = repo.createDiscreteProperty(p_resqmlUnstructuredGrid, "", w_intArray->GetName(), 1, gsoap_eml2_3::eml23__IndexableElement::nodes, Internal->discretePropertyKind);
			w_prop->pushBackInt32Hdf5Array1dOfValues(static_cast<int32_t*>(w_intArray->GetVoidPointer(0)), w_intArray->GetNumberOfValues(), hdfProxy, (std::numeric_limits<int32_t>::max)());
		}
		else {
			vtkDoubleArray* w_doubleArray = vtkDoubleArray::SafeDownCast(Internal->inputUnstructuredGrid->GetPointData()->GetArray(i));
			if (w_doubleArray)
			{
				RESQML2_NS::ContinuousProperty* w_prop = repo.createContinuousProperty(p_resqmlUnstructuredGrid, "", w_doubleArray->GetName(), 1, gsoap_eml2_3::eml23__IndexableElement::nodes, gsoap_resqml2_0_1::resqml20__ResqmlUom::Euc, Internal->continousPropertyKind);
				w_prop->pushBackDoubleHdf5Array1dOfValues(static_cast<double*>(w_doubleArray->GetVoidPointer(0)), w_doubleArray->GetNumberOfValues());
			}
			else
			{
				vtkOutputWindowDisplayErrorText(("Le type " + std::string(w_array->GetClassName()) + "de la propriété " + std::string(w_array->GetName()) + " n'est pas implémenté").c_str());
			}
		}
	}

	vtkCellData* w_cellData = Internal->inputUnstructuredGrid->GetCellData();
	for (int i = 0; i < w_cellData->GetNumberOfArrays(); i++)
	{
		vtkDataArray* w_array= Internal->inputUnstructuredGrid->GetCellData()->GetArray(i);
		vtkIntArray* w_intArray = vtkIntArray::SafeDownCast(Internal->inputUnstructuredGrid->GetCellData()->GetArray(i));
		if (w_intArray)
		{
			RESQML2_NS::DiscreteProperty* w_prop = repo.createDiscreteProperty(p_resqmlUnstructuredGrid, "", w_intArray->GetName(), 1, gsoap_eml2_3::eml23__IndexableElement::cells, Internal->discretePropertyKind);
			w_prop->pushBackInt32Hdf5Array1dOfValues(static_cast<int32_t*>(w_intArray->GetVoidPointer(0)), w_intArray->GetNumberOfValues(), hdfProxy, (std::numeric_limits<int32_t>::max)());
		}
		else {
			vtkDoubleArray* w_doubleArray = vtkDoubleArray::SafeDownCast(Internal->inputUnstructuredGrid->GetCellData()->GetArray(i));
			if (w_doubleArray)
			{
				RESQML2_NS::ContinuousProperty* w_prop = repo.createContinuousProperty(p_resqmlUnstructuredGrid, "", w_doubleArray->GetName(), 1, gsoap_eml2_3::eml23__IndexableElement::cells, gsoap_resqml2_0_1::resqml20__ResqmlUom::Euc, Internal->continousPropertyKind);
				w_prop->pushBackDoubleHdf5Array1dOfValues(static_cast<double*>(w_doubleArray->GetVoidPointer(0)), w_doubleArray->GetNumberOfValues());
			}
			else
			{
				vtkOutputWindowDisplayErrorText(("Le type " + std::string(w_array->GetClassName()) + "de la propriété " + std::string(w_array->GetName()) + " n'est pas implémenté").c_str());
			}
		}

	}
}


RESQML2_NS::UnstructuredGridRepresentation* vtkEPCWriter::writeUnstructuredGrid(COMMON_NS::DataObjectRepository& repo, EML2_NS::AbstractHdfProxy* hdfProxy, RESQML2_NS::LocalDepth3dCrs* local3dCrs)
{

	std::vector<double> w_points = getUnstructuredGridPoints();

	std::vector<uint64_t> w_nodeIndicesPerFace;
	std::vector<uint64_t> w_nodeIndicesCumulativeCountPerFace;
	std::vector<uint64_t> faceIndicesPerCell;
	std::vector<uint64_t> faceIndicesCumulativeCountPerCell;
	std::vector<uint8_t> faceRightHandness;



	uint64_t numberOfCells = Internal->inputUnstructuredGrid->GetNumberOfCells();

	for (vtkIdType cellId = 0; cellId < numberOfCells; ++cellId)
	{

		vtkCell* const cell = Internal->inputUnstructuredGrid->GetCell(cellId);
		int32_t const cellType = cell->GetCellType();
		vtkIdType const* const ptsIds = cell->GetPointIds()->begin();
		if (cellType == VTK_TETRA)
		{
			loadFacesForVTK_TETRA(ptsIds, w_nodeIndicesPerFace, w_nodeIndicesCumulativeCountPerFace, faceIndicesPerCell, faceIndicesCumulativeCountPerCell, faceRightHandness);
		}
		else if (cellType == VTK_HEXAHEDRON)
		{
			loadFacesForVTK_HEXAHEDRON(ptsIds, w_nodeIndicesPerFace, w_nodeIndicesCumulativeCountPerFace, faceIndicesPerCell, faceIndicesCumulativeCountPerCell, faceRightHandness);
		}
		else if (cellType == VTK_WEDGE)
		{
			loadFacesForVTK_WEDGE(ptsIds, w_nodeIndicesPerFace, w_nodeIndicesCumulativeCountPerFace, faceIndicesPerCell, faceIndicesCumulativeCountPerCell, faceRightHandness);
		}
		else if (cellType == VTK_PYRAMID)
		{
			loadFacesForVTK_PYRAMID(ptsIds, w_nodeIndicesPerFace, w_nodeIndicesCumulativeCountPerFace, faceIndicesPerCell, faceIndicesCumulativeCountPerCell, faceRightHandness);
		}
		else if (cellType == VTK_PENTAGONAL_PRISM)
		{
			loadFacesForVTK_PENTAGONAL_PRISM(ptsIds, w_nodeIndicesPerFace, w_nodeIndicesCumulativeCountPerFace, faceIndicesPerCell, faceIndicesCumulativeCountPerCell, faceRightHandness);
		}
		else if (cellType == VTK_HEXAGONAL_PRISM)
		{
			loadFacesForVTK_HEXAGONAL_PRISM(ptsIds, w_nodeIndicesPerFace, w_nodeIndicesCumulativeCountPerFace, faceIndicesPerCell, faceIndicesCumulativeCountPerCell, faceRightHandness);
		}
		else if (cellType == VTK_VOXEL)
		{
			vtkOutputWindowDisplayErrorText("VTK_VOXEL in VtkUnstructuredGrid is not supported");
			return nullptr;
		}
		else // vtkPolyhedron
		{
			loadFacesForVTK_POLYHEDRON(cellId, w_nodeIndicesPerFace, w_nodeIndicesCumulativeCountPerFace, faceIndicesPerCell, faceIndicesCumulativeCountPerCell, faceRightHandness);
		}
	}

	// creating the unstructured grid
	vtkOutputWindowDisplayDebugText((Internal->inputUnstructuredGrid->GetObjectName()).c_str());
	RESQML2_NS::UnstructuredGridRepresentation* unstructuredGrid = repo.createUnstructuredGridRepresentation("", Internal->inputUnstructuredGridName, numberOfCells);
	unstructuredGrid->setGeometry(faceRightHandness.data(), w_points.data(), w_points.size() / 3, nullptr, faceIndicesPerCell.data(), faceIndicesCumulativeCountPerCell.data(), faceIndicesPerCell.size() /* warning shared faces */, w_nodeIndicesPerFace.data(), w_nodeIndicesCumulativeCountPerFace.data(), gsoap_resqml2_0_1::resqml20__CellShape::polyhedral /*local3dCrs*/);

	return unstructuredGrid;
}

//-----------------------------------------------------------------------------
std::vector<double> vtkEPCWriter::getUnstructuredGridPoints()
{
	vtkSmartPointer<vtkPoints> w_points = Internal->inputUnstructuredGrid->GetPoints();
	vtkIdType numberPoints = w_points->GetNumberOfPoints();
	std::vector<double> unstructuredGridPoints;

	for (vtkIdType i = 0; i < numberPoints; ++i)
	{
		double p[3];
		w_points->GetPoint(i, p);
		unstructuredGridPoints.push_back(p[0]);
		unstructuredGridPoints.push_back(p[1]);
		unstructuredGridPoints.push_back(p[2]);
	}
	return unstructuredGridPoints;
}

/***
* VTK_TETRA(:
*
* https://examples.vtk.org/site/VTKBook/05Chapter5/#Figure%205-2
*
*	  3
*	 /|\
*   / | \
*  /  |  \
* /   |   \
*0----|----2
* \   |   /
*  \  |  /
*   \ | /
*	 \|/
*	  1
*
* F0: 0 - 1 - 2
* F1: 0 - 1 - 3
* F2: 1 - 2 - 3
* F3: 0 - 2 - 3
*/
void vtkEPCWriter::loadFacesForVTK_TETRA(vtkIdType const* p_ptsIds, std::vector<uint64_t>& p_nodeIndicesPerFace, std::vector<uint64_t>& p_nodeIndicesCumulativeCountPerFace, std::vector<uint64_t>& faceIndicesPerCell, std::vector<uint64_t>& faceIndicesCumulativeCountPerCell, std::vector<uint8_t>& faceRightHandness)
{
	/*
	* Nodes per face
	*/
	//auto p_ptsIds = cell->GetPointIds();
	uint64_t w_nodeIndicesForWedgeFaces[12] = {
		//Face 0
		static_cast<uint64_t>(p_ptsIds[0]),
		static_cast<uint64_t>(p_ptsIds[1]),
		static_cast<uint64_t>(p_ptsIds[2]),
		//Face 1
		static_cast<uint64_t>(p_ptsIds[0]),
		static_cast<uint64_t>(p_ptsIds[1]),
		static_cast<uint64_t>(p_ptsIds[3]),
		//Face 2
		static_cast<uint64_t>(p_ptsIds[1]),
		static_cast<uint64_t>(p_ptsIds[2]),
		static_cast<uint64_t>(p_ptsIds[3]),
		//Face 3
		static_cast<uint64_t>(p_ptsIds[0]),
		static_cast<uint64_t>(p_ptsIds[2]),
		static_cast<uint64_t>(p_ptsIds[3])
	};
	p_nodeIndicesPerFace.insert(std::end(p_nodeIndicesPerFace), std::begin(w_nodeIndicesForWedgeFaces), std::end(w_nodeIndicesForWedgeFaces));

	/*
	* Node cumulative count
	*/

	// .reserve + .push_back
	uint64_t initNodeIndicesCumulativeCountPerFace = p_nodeIndicesCumulativeCountPerFace.empty() ? 0 : p_nodeIndicesCumulativeCountPerFace.back();
	uint64_t w_nodeIndicesCumulativeCountPerFace[4] = { initNodeIndicesCumulativeCountPerFace + 3, initNodeIndicesCumulativeCountPerFace + 6, initNodeIndicesCumulativeCountPerFace + 9, initNodeIndicesCumulativeCountPerFace + 12 };
	p_nodeIndicesCumulativeCountPerFace.insert(std::end(p_nodeIndicesCumulativeCountPerFace), std::begin(w_nodeIndicesCumulativeCountPerFace), std::end(w_nodeIndicesCumulativeCountPerFace));

	/*
	* face indices per cell
	*/
	uint64_t initFaceId = faceIndicesPerCell.empty() ? 0 : faceIndicesPerCell.back() + 1;
	uint64_t w_faceIndicesPerCell[4] = { initFaceId, initFaceId + 1, initFaceId + 2, initFaceId + 3 };
	faceIndicesPerCell.insert(std::end(faceIndicesPerCell), std::begin(w_faceIndicesPerCell), std::end(w_faceIndicesPerCell));

	/*
	* face right handness
	*/
	uint8_t w_faceRightHandness[4] = { 1, 1, 1, 1 };
	faceRightHandness.insert(std::end(faceRightHandness), std::begin(w_faceRightHandness), std::end(w_faceRightHandness));

	/*
	* Face indices cumulative
	*/
	uint64_t initFaceIndicesCumulative = faceIndicesCumulativeCountPerCell.empty() ? 0 : faceIndicesCumulativeCountPerCell.back();
	faceIndicesCumulativeCountPerCell.push_back(initFaceIndicesCumulative + 4);
}

/***
* VTK_HEXAHEDRON:
*
* https://examples.vtk.org/site/VTKBook/05Chapter5/#Figure%205-2
*
*       3--------2
*       |\       |\
*       | \      | \
*       |  \     |  \
*       |   7--------6
*       |   |    |   |
*       0---|----1   |
*        \  |     \  |
*	      \ |      \ |
*          \|       \|
*           4--------5
*
* F0: 0 - 1 - 2 - 3
* F1: 4 - 5 - 6 - 7
* F2: 0 - 4 - 5 - 1
* F3: 1 - 5 - 6 - 2
* F4: 2 - 6 - 7 - 3
* F5: 3 - 7 - 4 - 0
*/
void vtkEPCWriter::loadFacesForVTK_HEXAHEDRON(vtkIdType const* p_ptsIds, std::vector<uint64_t>& p_nodeIndicesPerFace, std::vector<uint64_t>& p_nodeIndicesCumulativeCountPerFace, std::vector<uint64_t>& faceIndicesPerCell, std::vector<uint64_t>& faceIndicesCumulativeCountPerCell, std::vector<uint8_t>& faceRightHandness)
{
	/*
	* Nodes per face
	*/
	uint64_t w_nodeIndicesForHexahedronFaces[24] = {
		//Face 0
		static_cast<uint64_t>(p_ptsIds[0]),
		static_cast<uint64_t>(p_ptsIds[1]),
		static_cast<uint64_t>(p_ptsIds[2]),
		static_cast<uint64_t>(p_ptsIds[3]),
		//Face 1
		static_cast<uint64_t>(p_ptsIds[4]),
		static_cast<uint64_t>(p_ptsIds[5]),
		static_cast<uint64_t>(p_ptsIds[6]),
		static_cast<uint64_t>(p_ptsIds[7]),
		//Face 2
		static_cast<uint64_t>(p_ptsIds[0]),
		static_cast<uint64_t>(p_ptsIds[4]),
		static_cast<uint64_t>(p_ptsIds[5]),
		static_cast<uint64_t>(p_ptsIds[1]),
		//Face 3
		static_cast<uint64_t>(p_ptsIds[1]),
		static_cast<uint64_t>(p_ptsIds[5]),
		static_cast<uint64_t>(p_ptsIds[6]),
		static_cast<uint64_t>(p_ptsIds[2]),
		//Face 4
		static_cast<uint64_t>(p_ptsIds[2]),
		static_cast<uint64_t>(p_ptsIds[6]),
		static_cast<uint64_t>(p_ptsIds[7]),
		static_cast<uint64_t>(p_ptsIds[3]),
		//Face 5
		static_cast<uint64_t>(p_ptsIds[3]),
		static_cast<uint64_t>(p_ptsIds[7]),
		static_cast<uint64_t>(p_ptsIds[4]),
		static_cast<uint64_t>(p_ptsIds[0])
	};
	p_nodeIndicesPerFace.insert(std::end(p_nodeIndicesPerFace), std::begin(w_nodeIndicesForHexahedronFaces), std::end(w_nodeIndicesForHexahedronFaces));

	/*
	* Node cumulative count
	*/
	uint64_t initNodeIndicesCumulativeCountPerFace = p_nodeIndicesCumulativeCountPerFace.empty() ? 0 : p_nodeIndicesCumulativeCountPerFace.back();
	uint64_t w_nodeIndicesCumulativeCountPerFace[6] = { initNodeIndicesCumulativeCountPerFace + 4, initNodeIndicesCumulativeCountPerFace + 8, initNodeIndicesCumulativeCountPerFace + 12, initNodeIndicesCumulativeCountPerFace + 16, initNodeIndicesCumulativeCountPerFace + 20, initNodeIndicesCumulativeCountPerFace + 24 };
	p_nodeIndicesCumulativeCountPerFace.insert(std::end(p_nodeIndicesCumulativeCountPerFace), std::begin(w_nodeIndicesCumulativeCountPerFace), std::end(w_nodeIndicesCumulativeCountPerFace));

	/*
	* face indices per cell
	*/
	uint64_t initFaceId = faceIndicesPerCell.empty() ? 0 : faceIndicesPerCell.back() + 1;
	uint64_t w_faceIndicesPerCell[6] = { initFaceId, initFaceId + 1, initFaceId + 2, initFaceId + 3, initFaceId + 4, initFaceId + 5 };
	faceIndicesPerCell.insert(std::end(faceIndicesPerCell), std::begin(w_faceIndicesPerCell), std::end(w_faceIndicesPerCell));

	/*
	* face right handness
	*/
	uint8_t w_faceRightHandness[6] = { 0, 0, 1, 1, 1, 1 };
	faceRightHandness.insert(std::end(faceRightHandness), std::begin(w_faceRightHandness), std::end(w_faceRightHandness));

	/*
	* Face indices cumulative
	*/
	uint64_t initFaceIndicesCumulative = faceIndicesCumulativeCountPerCell.empty() ? 0 : faceIndicesCumulativeCountPerCell.back();
	faceIndicesCumulativeCountPerCell.push_back(initFaceIndicesCumulative + 6);
}

/***
* VTK_WEDGE:
*
* https://examples.vtk.org/site/VTKBook/05Chapter5/#Figure%205-2
*
*       0--------3
*       |\       |\
*       | \      | \
*       |  2--------5
*       | /      | /
*       |/       |/
*       1--------4
*
* F0: 2 - 1 - 0
* F1: 5 - 3 - 4
* F2: 0 - 1 - 4 - 3
* F3: 1 - 4 - 5 - 2
* F4: 2 - 5 - 3 - 0
*/
void vtkEPCWriter::loadFacesForVTK_WEDGE(vtkIdType const* p_ptsIds, std::vector<uint64_t>& p_nodeIndicesPerFace, std::vector<uint64_t>& nodeIndicesCumulativeCountPerFace, std::vector<uint64_t>& faceIndicesPerCell, std::vector<uint64_t>& faceIndicesCumulativeCountPerCell, std::vector<uint8_t>& faceRightHandness)
{
	/*
	* Nodes per face
	*/
	uint64_t w_nodeIndicesForWedgeFaces[18] = {
		//Face 0
		static_cast<uint64_t>(p_ptsIds[2]),
		static_cast<uint64_t>(p_ptsIds[1]),
		static_cast<uint64_t>(p_ptsIds[0]),
		//Face 1
		static_cast<uint64_t>(p_ptsIds[5]),
		static_cast<uint64_t>(p_ptsIds[3]),
		static_cast<uint64_t>(p_ptsIds[4]),
		//Face 2
		static_cast<uint64_t>(p_ptsIds[0]),
		static_cast<uint64_t>(p_ptsIds[1]),
		static_cast<uint64_t>(p_ptsIds[4]),
		static_cast<uint64_t>(p_ptsIds[3]),
		//Face 3
		static_cast<uint64_t>(p_ptsIds[1]),
		static_cast<uint64_t>(p_ptsIds[4]),
		static_cast<uint64_t>(p_ptsIds[5]),
		static_cast<uint64_t>(p_ptsIds[2]),
		//Face 4
		static_cast<uint64_t>(p_ptsIds[2]),
		static_cast<uint64_t>(p_ptsIds[5]),
		static_cast<uint64_t>(p_ptsIds[3]),
		static_cast<uint64_t>(p_ptsIds[0])
	};
	p_nodeIndicesPerFace.insert(std::end(p_nodeIndicesPerFace), std::begin(w_nodeIndicesForWedgeFaces), std::end(w_nodeIndicesForWedgeFaces));

	/*
	* Node cumulative count
	*/
	uint64_t initNodeIndicesCumulativeCountPerFace = nodeIndicesCumulativeCountPerFace.empty() ? 0 : nodeIndicesCumulativeCountPerFace.back();
	uint64_t w_nodeIndicesCumulativeCountPerFace[5] = { initNodeIndicesCumulativeCountPerFace + 3, initNodeIndicesCumulativeCountPerFace + 6, initNodeIndicesCumulativeCountPerFace + 10, initNodeIndicesCumulativeCountPerFace + 14 , initNodeIndicesCumulativeCountPerFace + 18 };
	nodeIndicesCumulativeCountPerFace.insert(std::end(nodeIndicesCumulativeCountPerFace), std::begin(w_nodeIndicesCumulativeCountPerFace), std::end(w_nodeIndicesCumulativeCountPerFace));

	/*
	* face indices per cell
	*/
	uint64_t initFaceId = faceIndicesPerCell.empty() ? 0 : faceIndicesPerCell.back() + 1;
	uint64_t w_faceIndicesPerCell[5] = { initFaceId, initFaceId + 1, initFaceId + 2, initFaceId + 3 , initFaceId + 4 };
	faceIndicesPerCell.insert(std::end(faceIndicesPerCell), std::begin(w_faceIndicesPerCell), std::end(w_faceIndicesPerCell));

	/*
	* face right handness
	*/
	uint8_t w_faceRightHandness[5] = { 0, 0, 1, 1, 1 };
	faceRightHandness.insert(std::end(faceRightHandness), std::begin(w_faceRightHandness), std::end(w_faceRightHandness));

	/*
	* Face indices cumulative
	*/
	uint64_t initFaceIndicesCumulative = faceIndicesCumulativeCountPerCell.empty() ? 0 : faceIndicesCumulativeCountPerCell.back();
	faceIndicesCumulativeCountPerCell.push_back(initFaceIndicesCumulative + 5);
}

/***
* VTK_PYRAMID:
*
* https://examples.vtk.org/site/VTKBook/05Chapter5/#Figure%205-2
*
*       1-----0
*       |\   /|
*       | \ / |
*       |  4  |
*       | / \ |
*       |/   \|
*       2-----3
*
* F0: 0 - 1 - 2 - 3
* F1: 0 - 1 - 4
* F2: 1 - 2 - 4
* F3: 2 - 3 - 4
* F4: 3 - 0 - 4
*/
void vtkEPCWriter::loadFacesForVTK_PYRAMID(vtkIdType const* p_ptsIds, std::vector<uint64_t>& p_nodeIndicesPerFace, std::vector<uint64_t>& nodeIndicesCumulativeCountPerFace, std::vector<uint64_t>& faceIndicesPerCell, std::vector<uint64_t>& faceIndicesCumulativeCountPerCell, std::vector<uint8_t>& faceRightHandness)
{
	/*
	* Nodes per face
	*/
	uint64_t w_nodeIndicesForWedgeFaces[16] = {
		//Face 0
		static_cast<uint64_t>(p_ptsIds[0]),
		static_cast<uint64_t>(p_ptsIds[1]),
		static_cast<uint64_t>(p_ptsIds[2]),
		static_cast<uint64_t>(p_ptsIds[3]),
		//Face 1
		static_cast<uint64_t>(p_ptsIds[0]),
		static_cast<uint64_t>(p_ptsIds[1]),
		static_cast<uint64_t>(p_ptsIds[4]),
		//Face 2
		static_cast<uint64_t>(p_ptsIds[1]),
		static_cast<uint64_t>(p_ptsIds[2]),
		static_cast<uint64_t>(p_ptsIds[4]),
		//Face 3
		static_cast<uint64_t>(p_ptsIds[2]),
		static_cast<uint64_t>(p_ptsIds[3]),
		static_cast<uint64_t>(p_ptsIds[4]),
		//Face 4
		static_cast<uint64_t>(p_ptsIds[3]),
		static_cast<uint64_t>(p_ptsIds[0]),
		static_cast<uint64_t>(p_ptsIds[4])
	};
	p_nodeIndicesPerFace.insert(std::end(p_nodeIndicesPerFace), std::begin(w_nodeIndicesForWedgeFaces), std::end(w_nodeIndicesForWedgeFaces));

	/*
	* Node cumulative count
	*/
	uint64_t initNodeIndicesCumulativeCountPerFace = nodeIndicesCumulativeCountPerFace.empty() ? 0 : nodeIndicesCumulativeCountPerFace.back();
	uint64_t w_nodeIndicesCumulativeCountPerFace[5] = { initNodeIndicesCumulativeCountPerFace + 4, initNodeIndicesCumulativeCountPerFace + 7, initNodeIndicesCumulativeCountPerFace + 10, initNodeIndicesCumulativeCountPerFace + 13 , initNodeIndicesCumulativeCountPerFace + 16 };
	nodeIndicesCumulativeCountPerFace.insert(std::end(nodeIndicesCumulativeCountPerFace), std::begin(w_nodeIndicesCumulativeCountPerFace), std::end(w_nodeIndicesCumulativeCountPerFace));

	/*
	* face indices per cell
	*/
	uint64_t initFaceId = faceIndicesPerCell.empty() ? 0 : faceIndicesPerCell.back() + 1;
	uint64_t w_faceIndicesPerCell[5] = { initFaceId, initFaceId + 1, initFaceId + 2, initFaceId + 3 , initFaceId + 4 };
	faceIndicesPerCell.insert(std::end(faceIndicesPerCell), std::begin(w_faceIndicesPerCell), std::end(w_faceIndicesPerCell));

	/*
	* face right handness
	*/
	uint8_t w_faceRightHandness[5] = { 0, 1, 1, 1, 1 };
	faceRightHandness.insert(std::end(faceRightHandness), std::begin(w_faceRightHandness), std::end(w_faceRightHandness));

	/*
	* Face indices cumulative
	*/
	uint64_t initFaceIndicesCumulative = faceIndicesCumulativeCountPerCell.empty() ? 0 : faceIndicesCumulativeCountPerCell.back();
	faceIndicesCumulativeCountPerCell.push_back(initFaceIndicesCumulative + 5);
}

/***
* VTK_PENTAGONAL_PRISM:
*
* https://examples.vtk.org/site/VTKBook/05Chapter5/#Figure%205-2
*
*/
void vtkEPCWriter::loadFacesForVTK_PENTAGONAL_PRISM(vtkIdType const* p_ptsIds, std::vector<uint64_t>& p_nodeIndicesPerFace, std::vector<uint64_t>& nodeIndicesCumulativeCountPerFace, std::vector<uint64_t>& faceIndicesPerCell, std::vector<uint64_t>& faceIndicesCumulativeCountPerCell, std::vector<uint8_t>& faceRightHandness)
{
	/*
* Nodes per face
*/
	uint64_t w_nodeIndicesForWedgeFaces[30] = {
		//Face 0
		static_cast<uint64_t>(p_ptsIds[0]),
		static_cast<uint64_t>(p_ptsIds[1]),
		static_cast<uint64_t>(p_ptsIds[2]),
		static_cast<uint64_t>(p_ptsIds[3]),
		static_cast<uint64_t>(p_ptsIds[4]),
		//Face 1
		static_cast<uint64_t>(p_ptsIds[5]),
		static_cast<uint64_t>(p_ptsIds[6]),
		static_cast<uint64_t>(p_ptsIds[7]),
		static_cast<uint64_t>(p_ptsIds[8]),
		static_cast<uint64_t>(p_ptsIds[9]),
		//Face 2
		static_cast<uint64_t>(p_ptsIds[0]),
		static_cast<uint64_t>(p_ptsIds[5]),
		static_cast<uint64_t>(p_ptsIds[6]),
		static_cast<uint64_t>(p_ptsIds[1]),
		//Face 3
		static_cast<uint64_t>(p_ptsIds[1]),
		static_cast<uint64_t>(p_ptsIds[6]),
		static_cast<uint64_t>(p_ptsIds[7]),
		static_cast<uint64_t>(p_ptsIds[2]),
		//Face 4
		static_cast<uint64_t>(p_ptsIds[2]),
		static_cast<uint64_t>(p_ptsIds[7]),
		static_cast<uint64_t>(p_ptsIds[8]),
		static_cast<uint64_t>(p_ptsIds[3]),
		//Face 5
		static_cast<uint64_t>(p_ptsIds[3]),
		static_cast<uint64_t>(p_ptsIds[8]),
		static_cast<uint64_t>(p_ptsIds[9]),
		static_cast<uint64_t>(p_ptsIds[4]),
		//Face 6
		static_cast<uint64_t>(p_ptsIds[4]),
		static_cast<uint64_t>(p_ptsIds[9]),
		static_cast<uint64_t>(p_ptsIds[5]),
		static_cast<uint64_t>(p_ptsIds[0])
	};
	p_nodeIndicesPerFace.insert(std::end(p_nodeIndicesPerFace), std::begin(w_nodeIndicesForWedgeFaces), std::end(w_nodeIndicesForWedgeFaces));

	/*
	* Node cumulative count
	*/
	uint64_t initNodeIndicesCumulativeCountPerFace = nodeIndicesCumulativeCountPerFace.empty() ? 0 : nodeIndicesCumulativeCountPerFace.back();
	uint64_t w_nodeIndicesCumulativeCountPerFace[7] = { initNodeIndicesCumulativeCountPerFace + 5, initNodeIndicesCumulativeCountPerFace + 10, initNodeIndicesCumulativeCountPerFace + 14, initNodeIndicesCumulativeCountPerFace + 18, initNodeIndicesCumulativeCountPerFace + 22, initNodeIndicesCumulativeCountPerFace + 26, initNodeIndicesCumulativeCountPerFace + 30 };
	nodeIndicesCumulativeCountPerFace.insert(std::end(nodeIndicesCumulativeCountPerFace), std::begin(w_nodeIndicesCumulativeCountPerFace), std::end(w_nodeIndicesCumulativeCountPerFace));

	/*
	* face indices per cell
	*/
	uint64_t initFaceId = faceIndicesPerCell.empty() ? 0 : faceIndicesPerCell.back() + 1;
	uint64_t w_faceIndicesPerCell[7] = { initFaceId, initFaceId + 1, initFaceId + 2, initFaceId + 3, initFaceId + 4, initFaceId + 5, initFaceId + 6 };
	faceIndicesPerCell.insert(std::end(faceIndicesPerCell), std::begin(w_faceIndicesPerCell), std::end(w_faceIndicesPerCell));

	/*
	* face right handness
	*/
	uint8_t w_faceRightHandness[7] = { 0,0,1,1,1,1,1 };
	faceRightHandness.insert(std::end(faceRightHandness), std::begin(w_faceRightHandness), std::end(w_faceRightHandness));

	/*
	* Face indices cumulative
	*/
	uint64_t initFaceIndicesCumulative = faceIndicesCumulativeCountPerCell.empty() ? 0 : faceIndicesCumulativeCountPerCell.back();
	faceIndicesCumulativeCountPerCell.push_back(initFaceIndicesCumulative + 7);
}

/***
* VTK_PENTAGONAL_PRISM:
*
* https://examples.vtk.org/site/VTKBook/05Chapter5/#Figure%205-2
*
*/
void vtkEPCWriter::loadFacesForVTK_HEXAGONAL_PRISM(vtkIdType const* p_ptsIds, std::vector<uint64_t>& p_nodeIndicesPerFace, std::vector<uint64_t>& nodeIndicesCumulativeCountPerFace, std::vector<uint64_t>& faceIndicesPerCell, std::vector<uint64_t>& faceIndicesCumulativeCountPerCell, std::vector<uint8_t>& faceRightHandness)
{
	/*
* Nodes per face
*/
	uint64_t w_nodeIndicesForWedgeFaces[36] = {
		//Face 0
		static_cast<uint64_t>(p_ptsIds[0]),
		static_cast<uint64_t>(p_ptsIds[1]),
		static_cast<uint64_t>(p_ptsIds[2]),
		static_cast<uint64_t>(p_ptsIds[3]),
		static_cast<uint64_t>(p_ptsIds[4]),
		static_cast<uint64_t>(p_ptsIds[5]),
		//Face 1
		static_cast<uint64_t>(p_ptsIds[6]),
		static_cast<uint64_t>(p_ptsIds[7]),
		static_cast<uint64_t>(p_ptsIds[8]),
		static_cast<uint64_t>(p_ptsIds[9]),
		static_cast<uint64_t>(p_ptsIds[10]),
		static_cast<uint64_t>(p_ptsIds[11]),
		//Face 2
		static_cast<uint64_t>(p_ptsIds[0]),
		static_cast<uint64_t>(p_ptsIds[6]),
		static_cast<uint64_t>(p_ptsIds[7]),
		static_cast<uint64_t>(p_ptsIds[1]),
		//Face 3
		static_cast<uint64_t>(p_ptsIds[1]),
		static_cast<uint64_t>(p_ptsIds[7]),
		static_cast<uint64_t>(p_ptsIds[8]),
		static_cast<uint64_t>(p_ptsIds[2]),
		//Face 4
		static_cast<uint64_t>(p_ptsIds[2]),
		static_cast<uint64_t>(p_ptsIds[8]),
		static_cast<uint64_t>(p_ptsIds[9]),
		static_cast<uint64_t>(p_ptsIds[3]),
		//Face 5
		static_cast<uint64_t>(p_ptsIds[3]),
		static_cast<uint64_t>(p_ptsIds[9]),
		static_cast<uint64_t>(p_ptsIds[10]),
		static_cast<uint64_t>(p_ptsIds[4]),
		//Face 6
		static_cast<uint64_t>(p_ptsIds[4]),
		static_cast<uint64_t>(p_ptsIds[10]),
		static_cast<uint64_t>(p_ptsIds[11]),
		static_cast<uint64_t>(p_ptsIds[5]),
		//Face 7
		static_cast<uint64_t>(p_ptsIds[5]),
		static_cast<uint64_t>(p_ptsIds[11]),
		static_cast<uint64_t>(p_ptsIds[6]),
		static_cast<uint64_t>(p_ptsIds[0])
	};
	p_nodeIndicesPerFace.insert(std::end(p_nodeIndicesPerFace), std::begin(w_nodeIndicesForWedgeFaces), std::end(w_nodeIndicesForWedgeFaces));

	/*
	* Node cumulative count
	*/
	uint64_t initNodeIndicesCumulativeCountPerFace = nodeIndicesCumulativeCountPerFace.empty() ? 0 : nodeIndicesCumulativeCountPerFace.back();
	uint64_t w_nodeIndicesCumulativeCountPerFace[8] = { initNodeIndicesCumulativeCountPerFace + 6, initNodeIndicesCumulativeCountPerFace + 12, initNodeIndicesCumulativeCountPerFace + 16, initNodeIndicesCumulativeCountPerFace + 20, initNodeIndicesCumulativeCountPerFace + 24, initNodeIndicesCumulativeCountPerFace + 28, initNodeIndicesCumulativeCountPerFace + 32, initNodeIndicesCumulativeCountPerFace + 36 };
	nodeIndicesCumulativeCountPerFace.insert(std::end(nodeIndicesCumulativeCountPerFace), std::begin(w_nodeIndicesCumulativeCountPerFace), std::end(w_nodeIndicesCumulativeCountPerFace));

	/*
	* face indices per cell
	*/
	uint64_t initFaceId = faceIndicesPerCell.empty() ? 0 : faceIndicesPerCell.back() + 1;
	uint64_t w_faceIndicesPerCell[8] = { initFaceId, initFaceId + 1, initFaceId + 2, initFaceId + 3, initFaceId + 4, initFaceId + 5, initFaceId + 6, initFaceId + 7 };
	faceIndicesPerCell.insert(std::end(faceIndicesPerCell), std::begin(w_faceIndicesPerCell), std::end(w_faceIndicesPerCell));

	/*
	* face right handness
	*/
	uint8_t w_faceRightHandness[8] = { 0,0,1,1,1,1,1,1 };
	faceRightHandness.insert(std::end(faceRightHandness), std::begin(w_faceRightHandness), std::end(w_faceRightHandness));

	/*
	* Face indices cumulative
	*/
	uint64_t initFaceIndicesCumulative = faceIndicesCumulativeCountPerCell.empty() ? 0 : faceIndicesCumulativeCountPerCell.back();
	faceIndicesCumulativeCountPerCell.push_back(initFaceIndicesCumulative + 8);
}

/***
* VTK_POLYHEDRON:
*
* https://examples.vtk.org/site/VTKBook/05Chapter5/#Figure%205-2
*
*/
void vtkEPCWriter::loadFacesForVTK_POLYHEDRON(vtkIdType cellId, std::vector<uint64_t>& p_nodeIndicesPerFace, std::vector<uint64_t>& nodeIndicesCumulativeCountPerFace, std::vector<uint64_t>& faceIndicesPerCell, std::vector<uint64_t>& faceIndicesCumulativeCountPerCell, std::vector<uint8_t>& faceRightHandness)
{
	if (Internal->inputUnstructuredGrid->GetCellType(cellId) == VTK_POLYHEDRON)
	{
		vtkIdType nfaces;
		const vtkIdType* facePtIds;
		Internal->inputUnstructuredGrid->GetFaceStream(cellId, nfaces, facePtIds);

		uint64_t newNodeIndicesCumulativeCountPerFace = nodeIndicesCumulativeCountPerFace.empty() ? 0 : nodeIndicesCumulativeCountPerFace.back();
		uint64_t initFaceId = faceIndicesPerCell.empty() ? 0 : faceIndicesPerCell.back() + 1;
		uint64_t initFaceIndicesCumulative = faceIndicesCumulativeCountPerCell.empty() ? 0 : faceIndicesCumulativeCountPerCell.back();
		faceIndicesCumulativeCountPerCell.push_back(initFaceIndicesCumulative + nfaces);
		faceIndicesPerCell.reserve(nfaces);
		for (vtkIdType id = 0; id < nfaces; ++id)
		{
			faceIndicesPerCell.push_back(initFaceId + id);
			vtkIdType nbPoints = facePtIds[0];
			newNodeIndicesCumulativeCountPerFace += nbPoints;
			nodeIndicesCumulativeCountPerFace.push_back(newNodeIndicesCumulativeCountPerFace);
			faceRightHandness.push_back(1);
			p_nodeIndicesPerFace.reserve(nbPoints);
				for (vtkIdType j = 1; j <= nbPoints; ++j)
				{
					double ptsIds = facePtIds[j];
					p_nodeIndicesPerFace.push_back(facePtIds[j]);
				}
				facePtIds += nbPoints + 1;
		}
	}
}

void vtkEPCWriter::PrintSelf(ostream& os, vtkIndent indent)
{
	this->Superclass::PrintSelf(os, indent);

	os << indent << "FileName: " << (GetFileName() ? GetFileName() : "(none)") << indent;
}

