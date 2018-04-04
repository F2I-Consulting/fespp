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


//----------------------------------------------------------------------------
VtkGrid2DRepresentationPoints::VtkGrid2DRepresentationPoints(const std::string & fileName, const std::string & name, const std::string & uuid, const std::string & uuidParent, common::EpcDocument *pckEPCRep, common::EpcDocument *pckEPCSubRep) :
VtkResqml2PolyData(fileName, name, uuid, uuidParent, pckEPCRep, pckEPCSubRep)
{
}

//----------------------------------------------------------------------------
void VtkGrid2DRepresentationPoints::createOutput(const std::string & uuid)
{
	if (!subRepresentation)	{

		resqml2_0_1::Grid2dRepresentation* grid2dRepresentation = nullptr;
		common::AbstractObject* obj = epcPackageRepresentation->getResqmlAbstractObjectByUuid(getUuid().substr(0, 36));

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
}

//----------------------------------------------------------------------------
	void VtkGrid2DRepresentationPoints::addProperty(const std::string uuidProperty, vtkDataArray* dataProperty)
{
	vtkOutput->Modified();
	vtkOutput->GetPointData()->AddArray(dataProperty);
	lastProperty = uuidProperty;
}

	long VtkGrid2DRepresentationPoints::getAttachmentPropertyCount(const std::string & uuid, const FesppAttachmentProperty propertyUnit)
	{
		long result = 0;
		resqml2_0_1::Grid2dRepresentation* grid2dRepresentation = nullptr;
		common::AbstractObject* obj = epcPackageRepresentation->getResqmlAbstractObjectByUuid(getUuid().substr(0, 36));

		if (obj != nullptr && obj->getXmlTag() == "Grid2dRepresentation"){
			grid2dRepresentation = static_cast<resqml2_0_1::Grid2dRepresentation*>(obj);

			result = grid2dRepresentation->getNodeCountAlongIAxis() * grid2dRepresentation->getNodeCountAlongJAxis();
		}
		return result;
	}

