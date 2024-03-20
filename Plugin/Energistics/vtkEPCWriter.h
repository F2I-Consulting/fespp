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

/**
 * @class   vtkEPCWriter
 *
 * Writer for the "epc" format. Write an unstructured grid as an resqml file.
 * It supports export of data arrays as properties.
 */

#ifndef __vtkEPCWriter_h
#define __vtkEPCWriter_h

#include <vector>

#include "EnergisticsModule.h"
#include "vtkWriter.h"

struct EPCWriterInternal;
class vtkUnstructuredGrid;
class vtkCell;

namespace common {
	class DataObjectRepository;
}
namespace eml2 {
	class AbstractHdfProxy;
}
namespace resqml2 {
	class LocalDepth3dCrs;
	class UnstructuredGridRepresentation;
}

class ENERGISTICS_EXPORT vtkEPCWriter : public vtkWriter
{
public:
	static vtkEPCWriter* New();
	vtkTypeMacro(vtkEPCWriter, vtkWriter);
	void PrintSelf(ostream& os, vtkIndent indent) override;

	//@{
	/**
	 * Specify the file name to be written.
	 */
	vtkSetStringMacro(FileName);
	vtkGetStringMacro(FileName);
	//@}

protected:
	vtkEPCWriter();
	~vtkEPCWriter() override;

	int FillInputPortInformation(int port, vtkInformation* info) override;
	int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
	void WriteData() override;

	resqml2::UnstructuredGridRepresentation* writeUnstructuredGrid(common::DataObjectRepository&, eml2::AbstractHdfProxy*, resqml2::LocalDepth3dCrs*);
	void writeProperties(common::DataObjectRepository&, eml2::AbstractHdfProxy*, resqml2::UnstructuredGridRepresentation*);

	char* FileName = nullptr;

private:
	std::vector<double> getUnstructuredGridPoints();

	/***
	* VTK_TETRA
	*/
	void loadFacesForVTK_TETRA(vtkIdType const* ptsIds, std::vector<uint64_t>& nodeIndicesPerFace, std::vector<uint64_t>& nodeIndicesCumulativeCountPerFace, std::vector<uint64_t>& faceIndicesPerCell, std::vector<uint64_t>& faceIndicesCumulativeCountPerCell, std::vector<uint8_t>& faceRightHandness);
	/***
	* VTK_HEXAHEDRON
	*/
	void loadFacesForVTK_HEXAHEDRON(vtkIdType const* ptsIds, std::vector<uint64_t>& nodeIndicesPerFace, std::vector<uint64_t>& nodeIndicesCumulativeCountPerFace, std::vector<uint64_t>& faceIndicesPerCell, std::vector<uint64_t>& faceIndicesCumulativeCountPerCell, std::vector<uint8_t>& faceRightHandness);
	/***
	* VTK_WEDGE
	*/
	void loadFacesForVTK_WEDGE(vtkIdType const* ptsIds, std::vector<uint64_t>& nodeIndicesPerFace, std::vector<uint64_t>& nodeIndicesCumulativeCountPerFace, std::vector<uint64_t>& faceIndicesPerCell, std::vector<uint64_t>& faceIndicesCumulativeCountPerCell, std::vector<uint8_t>& faceRightHandness);
	/***
	* VTK_PYRAMID
	*/
	void loadFacesForVTK_PYRAMID(vtkIdType const* ptsIds, std::vector<uint64_t>& nodeIndicesPerFace, std::vector<uint64_t>& nodeIndicesCumulativeCountPerFace, std::vector<uint64_t>& faceIndicesPerCell, std::vector<uint64_t>& faceIndicesCumulativeCountPerCell, std::vector<uint8_t>& faceRightHandness);
	/***
	* VTK_PENTAGONAL_PRISM
	*/
	void loadFacesForVTK_PENTAGONAL_PRISM(vtkIdType const* ptsIds, std::vector<uint64_t>& nodeIndicesPerFace, std::vector<uint64_t>& nodeIndicesCumulativeCountPerFace, std::vector<uint64_t>& faceIndicesPerCell, std::vector<uint64_t>& faceIndicesCumulativeCountPerCell, std::vector<uint8_t>& faceRightHandness);
	/***
	* VTK_HEXAGONAL_PRISM
	*/
	void loadFacesForVTK_HEXAGONAL_PRISM(vtkIdType const* ptsIds, std::vector<uint64_t>& nodeIndicesPerFace, std::vector<uint64_t>& nodeIndicesCumulativeCountPerFace, std::vector<uint64_t>& faceIndicesPerCell, std::vector<uint64_t>& faceIndicesCumulativeCountPerCell, std::vector<uint8_t>& faceRightHandness);
	/***
	* VTK_POLYHEDRON
	*/
	void loadFacesForVTK_POLYHEDRON(vtkIdType cellId, std::vector<uint64_t>& nodeIndicesPerFace, std::vector<uint64_t>& nodeIndicesCumulativeCountPerFace, std::vector<uint64_t>& faceIndicesPerCell, std::vector<uint64_t>& faceIndicesCumulativeCountPerCell, std::vector<uint8_t>& faceRightHandness);

	vtkEPCWriter(const vtkEPCWriter&) = delete;
	void operator=(const vtkEPCWriter&) = delete;

	EPCWriterInternal* Internal;
};
#endif
