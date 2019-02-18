/*-----------------------------------------------------------------------
Copyright F2I-CONSULTING, (2014)

cedric.robert@f2i-consulting.com

This software is a computer program whose purpose is to display data formatted using Energistics standards.

This software is governed by the CeCILL license under French law and
abiding by the rules of distribution of free software.  You can  use,
modify and/ or redistribute the software under the terms of the CeCILL
license as circulated by CEA, CNRS and INRIA at the following URL
"http://www.cecill.info".

As a counterpart to the access to the source code and  rights to copy,
modify and redistribute granted by the license, users are provided only
with a limited warranty  and the software's author,  the holder of the
economic rights,  and the successive licensors  have only  limited
liability.

In this respect, the user's attention is drawn to the risks associated
with loading,  using,  modifying and/or developing or reproducing the
software by the user in light of its specific status of free software,
that may mean  that it is complicated to manipulate,  and  that  also
therefore means  that it is reserved for developers  and  experienced
professionals having in-depth computer knowledge. Users are therefore
encouraged to load and test the software's suitability as regards their
requirements in conditions enabling the security of their systems and/or
data to be ensured and,  more generally, to use and operate it in the
same conditions as regards security.

The fact that you are presently reading this means that you have had
knowledge of the CeCILL license and that you accept its terms.
-----------------------------------------------------------------------*/
#include "VtkTriangulatedRepresentation.h"

// include VTK library
#include <vtkPointData.h>
#include <vtkCellArray.h>
#include <vtkSmartPointer.h>
#include <vtkTriangle.h>

// include F2i-consulting Energistics Standards API
#include <resqml2_0_1/TriangulatedSetRepresentation.h>

// include F2i-consulting Energistics Standards ParaView Plugin
#include "VtkProperty.h"

//----------------------------------------------------------------------------
VtkTriangulatedRepresentation::VtkTriangulatedRepresentation(const std::string & fileName, const std::string & name, const std::string & uuid, const std::string & uuidParent, const unsigned int & patchNo, common::EpcDocument *pckRep, common::EpcDocument *pckSubRep) :
VtkResqml2PolyData(fileName, name, uuid, uuidParent, pckRep, pckSubRep), patchIndex(patchNo)
{
}

//----------------------------------------------------------------------------
VtkTriangulatedRepresentation::~VtkTriangulatedRepresentation()
{
	cout << "VtkTriangulatedRepresentation::~VtkTriangulatedRepresentation() " << getUuid() << "\n";
	patchIndex = 0;
	lastProperty = "";
}

//----------------------------------------------------------------------------
void VtkTriangulatedRepresentation::createOutput(const std::string &uuid)
{
	if (!subRepresentation)	{

		resqml2_0_1::TriangulatedSetRepresentation* triangulatedSetRepresentation = nullptr;
		common::AbstractObject* obj = epcPackageRepresentation->getResqmlAbstractObjectByUuid(getUuid());
		if (obj != nullptr && obj->getXmlTag() == "TriangulatedSetRepresentation")
		{
			triangulatedSetRepresentation = static_cast<resqml2_0_1::TriangulatedSetRepresentation*>(obj);
		}

		if (!vtkOutput)
		{
			vtkOutput = vtkSmartPointer<vtkPolyData>::New();

			// POINT
			vtkSmartPointer<vtkPoints> triangulatedRepresentationPoints = vtkSmartPointer<vtkPoints>::New();

			unsigned int nodeCount = triangulatedSetRepresentation->getXyzPointCountOfAllPatches();
			double* allXyzPoints = new double[nodeCount * 3];
			triangulatedSetRepresentation->getXyzPointsOfAllPatchesInGlobalCrs(allXyzPoints);

			createVtkPoints(nodeCount, allXyzPoints, triangulatedSetRepresentation->getLocalCrs());

			delete[] allXyzPoints;

			// CELLS
			vtkSmartPointer<vtkCellArray> triangulatedRepresentationTriangles = vtkSmartPointer<vtkCellArray>::New();
			unsigned int * triangleIndices = new unsigned int[triangulatedSetRepresentation->getTriangleCountOfPatch(this->patchIndex) * 3];
			triangulatedSetRepresentation->getTriangleNodeIndicesOfPatch(this->patchIndex, triangleIndices);
			for (unsigned int p = 0; p < triangulatedSetRepresentation->getTriangleCountOfPatch(this->patchIndex); ++p)
			{
				vtkSmartPointer<vtkTriangle> triangulatedRepresentationTriangle = vtkSmartPointer<vtkTriangle>::New();
				triangulatedRepresentationTriangle->GetPointIds()->SetId(0, triangleIndices[p * 3]);
				triangulatedRepresentationTriangle->GetPointIds()->SetId(1, triangleIndices[p * 3 + 1]);
				triangulatedRepresentationTriangle->GetPointIds()->SetId(2, triangleIndices[p * 3 + 2]);
				triangulatedRepresentationTriangles->InsertNextCell(triangulatedRepresentationTriangle);
			}
			vtkOutput->SetPoints(points);
			vtkOutput->SetPolys(triangulatedRepresentationTriangles);

			points = nullptr;

			delete[] triangleIndices;
		}
		else
		{
			if (uuid != getUuid())
			{
				vtkDataArray* arrayProperty = uuidToVtkProperty[uuid]->visualize(uuid, triangulatedSetRepresentation);
				this->addProperty(uuid, arrayProperty);
			}
		}
	}
}

//----------------------------------------------------------------------------
void VtkTriangulatedRepresentation::addProperty(const std::string & uuidProperty, vtkDataArray* dataProperty)
{
	vtkOutput->Modified();
	vtkOutput->GetPointData()->AddArray(dataProperty);
	lastProperty = uuidProperty;
}

long VtkTriangulatedRepresentation::getAttachmentPropertyCount(const std::string & uuid, const VtkEpcCommon::FesppAttachmentProperty propertyUnit)
{
	long result = 0;
	resqml2_0_1::TriangulatedSetRepresentation* triangulatedSetRepresentation = nullptr;
	common::AbstractObject* obj = epcPackageRepresentation->getResqmlAbstractObjectByUuid(getUuid());
	if (obj != nullptr && obj->getXmlTag() == "TriangulatedSetRepresentation")
	{
		triangulatedSetRepresentation = static_cast<resqml2_0_1::TriangulatedSetRepresentation*>(obj);
		result = triangulatedSetRepresentation->getXyzPointCountOfAllPatches();
	}
	return result;
}

