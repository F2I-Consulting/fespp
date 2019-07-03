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
#include "VtkGrid2DRepresentationCells.h"

// include VTK library
#include <vtkSmartPointer.h>
#include <vtkMath.h>
#include <vtkCellArray.h>

// include F2i-consulting Energistics Standards API
#include <common/EpcDocument.h>
#include <resqml2_0_1/Grid2dRepresentation.h>

//----------------------------------------------------------------------------
VtkGrid2DRepresentationCells::VtkGrid2DRepresentationCells(const std::string & fileName, const std::string & name, const std::string & uuid, const std::string & uuidParent, common::EpcDocument *pckEPCRep, common::EpcDocument *pckEPCSubRep) :
VtkResqml2StructuredGrid(fileName, name, uuid, uuidParent, pckEPCRep, pckEPCSubRep)
{

}

VtkGrid2DRepresentationCells::~VtkGrid2DRepresentationCells()
{
	lastProperty = "";
}
//----------------------------------------------------------------------------
void VtkGrid2DRepresentationCells::createOutput(const std::string & uuid)
{
	if (!subRepresentation)	{

		resqml2_0_1::Grid2dRepresentation* grid2dRepresentation = nullptr;
		common::AbstractObject* obj = epcPackageRepresentation->getDataObjectByUuid(uuid);
		if (obj != nullptr && obj->getXmlTag() == "Grid2dRepresentation")
		{
			grid2dRepresentation = static_cast<resqml2_0_1::Grid2dRepresentation*>(obj);
		}

		if (!vtkOutput)
		{
			vtkOutput = vtkSmartPointer<vtkStructuredGrid>::New();

			resqml2_0_1::Grid2dRepresentation *supportingGrid = grid2dRepresentation->getSupportingRepresentation();
			resqml2_0_1::Grid2dRepresentation *featureGrid = grid2dRepresentation;
			unsigned int nbNodeI = supportingGrid->getNodeCountAlongIAxis();
			unsigned int nbNodeJ = supportingGrid->getNodeCountAlongJAxis();
			double originX, originY;
			originX = supportingGrid->getXOriginInGlobalCrs();
			originY = supportingGrid->getYOriginInGlobalCrs();
			double *z = new double[nbNodeI * nbNodeJ];
			featureGrid->getZValuesInGlobalCrs(z);
			vtkSmartPointer<vtkPoints> points = vtkSmartPointer<vtkPoints>::New();
			vtkSmartPointer<vtkCellArray> vertices = vtkSmartPointer<vtkCellArray>::New();
			double XIOffset = supportingGrid->getXIOffsetInGlobalCrs();
			double XJOffset = supportingGrid->getXJOffsetInGlobalCrs();
			double YIOffset = supportingGrid->getYIOffsetInGlobalCrs();
			double YJOffset = supportingGrid->getYJOffsetInGlobalCrs();
			std::vector<unsigned int> ptBlank;
			for (unsigned int j = 0; j < nbNodeJ; ++j)
			{
				for (unsigned int i = 0; i < nbNodeI; ++i)
				{
					unsigned int ptId = i + j * nbNodeI;
					if (vtkMath::IsNan(z[ptId]))
					{
						points->InsertNextPoint(i, j, i + j);
						ptBlank.push_back(ptId);
					}
					else
					{
						points->InsertNextPoint(
							originX + i*XIOffset + j*XJOffset,
							originY + i*YIOffset + j*YJOffset,
							z[ptId]
							);
					}
				}
			}

			delete[] z;
			vtkOutput->SetDimensions(nbNodeI, nbNodeJ, 1);
			vtkOutput->SetPoints(points);

			for (unsigned int i = 0; i < ptBlank.size(); ++i)
				vtkOutput->BlankPoint(ptBlank[i]);

			vtkOutput->Modified();
		}
	}
}
