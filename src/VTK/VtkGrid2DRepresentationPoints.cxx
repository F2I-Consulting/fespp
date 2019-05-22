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
#include "VtkGrid2DRepresentationPoints.h"

// include VTK library
#include <vtkSmartPointer.h>
#include <vtkMath.h>
#include <vtkPointData.h>
#include <vtkCellArray.h>

// include F2i-consulting Energistics Standards API
#include <common/EpcDocument.h>
#include <resqml2_0_1/Grid2dRepresentation.h>

// include F2i-consulting Energistics Standards ParaView Plugin
#include "VtkProperty.h"

#ifdef WITH_TEST
const std::string loggClass = "CLASS=VtkGrid2DRepresentationPoints ";
#define BEGIN_FUNC(name_func) L_(linfo) << loggClass << " FUNCTION=" << name_func << " CALL_FUNCTUION=none ITERATION=0 API=FESPP STATUS=START"
#define END_FUNC(name_func) L_(linfo) << loggClass << " FUNCTION=" << name_func << " CALL_FUNCTUION=none ITERATION=0 API=FESPP STATUS=END"
#define CALL_FUNC(name_func, call_func, iter, api)  L_(linfo) << loggClass << " FUNCTION=" << name_func << " CALL_FUNCTUION=" << call_func << " ITERATION=" << iter << " API=" << api << " STATUS=IN"
#endif

//----------------------------------------------------------------------------
VtkGrid2DRepresentationPoints::VtkGrid2DRepresentationPoints(const std::string & fileName, const std::string & name, const std::string & uuid, const std::string & uuidParent, common::EpcDocument *pckEPCRep, common::EpcDocument *pckEPCSubRep) :
		VtkResqml2PolyData(fileName, name, uuid, uuidParent, pckEPCRep, pckEPCSubRep)
{
#ifdef WITH_TEST
	BEGIN_FUNC(__func__);
	END_FUNC(__func__);
#endif
}

VtkGrid2DRepresentationPoints::~VtkGrid2DRepresentationPoints()
{
#ifdef WITH_TEST
	BEGIN_FUNC(__func__);
#endif
	lastProperty = "";
#ifdef WITH_TEST
	END_FUNC(__func__);
#endif
}
//----------------------------------------------------------------------------
void VtkGrid2DRepresentationPoints::createOutput(const std::string & uuid)
{
#ifdef WITH_TEST
	BEGIN_FUNC(__func__);
#endif
	if (!subRepresentation)	{

		resqml2_0_1::Grid2dRepresentation* grid2dRepresentation = nullptr;
		common::AbstractObject* obj = epcPackageRepresentation->getDataObjectByUuid(getUuid().substr(0, 36));

		if (obj != nullptr && obj->getXmlTag() == "Grid2dRepresentation")
			grid2dRepresentation = static_cast<resqml2_0_1::Grid2dRepresentation*>(obj);

		if (!vtkOutput)
		{
			vtkOutput = vtkSmartPointer<vtkPolyData>::New();

			const ULONG64 nbNodeI = grid2dRepresentation->getNodeCountAlongIAxis();
			const ULONG64 nbNodeJ = grid2dRepresentation->getNodeCountAlongJAxis();
			const double originX = grid2dRepresentation->getXOriginInGlobalCrs();
			const double originY = grid2dRepresentation->getYOriginInGlobalCrs();
			double *z = new double[nbNodeI * nbNodeJ];
			grid2dRepresentation->getZValuesInGlobalCrs(z);
			vtkSmartPointer<vtkPoints> points = vtkSmartPointer<vtkPoints>::New();
			vtkSmartPointer<vtkCellArray> vertices = vtkSmartPointer<vtkCellArray>::New();
			const double XIOffset = grid2dRepresentation->getXIOffsetInGlobalCrs();
			const double XJOffset = grid2dRepresentation->getXJOffsetInGlobalCrs();
			const double YIOffset = grid2dRepresentation->getYIOffsetInGlobalCrs();
			const double YJOffset = grid2dRepresentation->getYJOffsetInGlobalCrs();

			double zIndice = 1;

			if (grid2dRepresentation->getLocalCrs()->isDepthOriented())
			{
				zIndice = -1;
			}

			for (ULONG64 j = 0; j < nbNodeJ; ++j)
			{
				for (ULONG64 i = 0; i < nbNodeI; ++i)
				{
					size_t ptId = i + j * nbNodeI;
					if (!(vtkMath::IsNan(z[ptId])))
					{
						vtkIdType pid[1];
						pid[0] = points->InsertNextPoint(
								originX + i*XIOffset + j*XJOffset,
								originY + i*YIOffset + j*YJOffset,
								z[ptId] * zIndice
						);
						vertices->InsertNextCell(1, pid);
					}
				}
			}
			delete[] z;

			vtkOutput->SetPoints(points);
			vtkOutput->SetVerts(vertices);

		}
		else
		{
			if (uuid != getUuid().substr(0, 36))
			{
				vtkDataArray* arrayProperty = uuidToVtkProperty[uuid]->visualize(uuid, grid2dRepresentation);
				this->addProperty(uuid, arrayProperty);
			}
		}
	}
#ifdef WITH_TEST
	END_FUNC(__func__);
#endif
}

//----------------------------------------------------------------------------
void VtkGrid2DRepresentationPoints::addProperty(const std::string & uuidProperty, vtkDataArray* dataProperty)
{
#ifdef WITH_TEST
	BEGIN_FUNC(__func__);
#endif
	vtkOutput->Modified();
	vtkOutput->GetPointData()->AddArray(dataProperty);
	lastProperty = uuidProperty;
#ifdef WITH_TEST
	END_FUNC(__func__);
#endif
}

//----------------------------------------------------------------------------
long VtkGrid2DRepresentationPoints::getAttachmentPropertyCount(const std::string & uuid, const VtkEpcCommon::FesppAttachmentProperty propertyUnit)
{
#ifdef WITH_TEST
	BEGIN_FUNC(__func__);
#endif
	long result = 0;
	resqml2_0_1::Grid2dRepresentation* grid2dRepresentation = nullptr;
	common::AbstractObject* obj = epcPackageRepresentation->getDataObjectByUuid(getUuid().substr(0, 36));

	if (obj != nullptr && obj->getXmlTag() == "Grid2dRepresentation"){
		grid2dRepresentation = static_cast<resqml2_0_1::Grid2dRepresentation*>(obj);

		result = grid2dRepresentation->getNodeCountAlongIAxis() * grid2dRepresentation->getNodeCountAlongJAxis();
	}
#ifdef WITH_TEST
	END_FUNC(__func__);
#endif
	return result;
}

