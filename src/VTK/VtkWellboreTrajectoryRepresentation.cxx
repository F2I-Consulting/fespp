#include "VtkWellboreTrajectoryRepresentation.h"

#include <sstream>

// include VTK library
#include <vtkInformation.h>

// include F2i-consulting Energistics Standards API 
#include <common/EpcDocument.h>
#include <resqml2_0_1/WellboreTrajectoryRepresentation.h>

// include F2i-consulting Energistics Standards ParaView Plugin
#include "VtkWellboreTrajectoryRepresentationDatum.h"
#include "VtkWellboreTrajectoryRepresentationText.h"
#include "VtkWellboreTrajectoryRepresentationPolyLine.h"

//----------------------------------------------------------------------------
VtkWellboreTrajectoryRepresentation::VtkWellboreTrajectoryRepresentation(const std::string & fileName, const std::string & name, const std::string & uuid, const std::string & uuidParent, common::EpcDocument *pckRep, common::EpcDocument *pckSubRep) :
VtkResqml2MultiBlockDataSet(fileName, name, uuid, uuidParent), epcPackageRepresentation(pckRep), epcPackageSubRepresentation(pckSubRep),
polyline(getFileName(), name, uuid+"-Polyline", uuidParent, pckRep, pckSubRep),
head(getFileName(), name, uuid+"-Head", uuidParent, pckRep, pckSubRep),
text(getFileName(), name, uuid+"-Text", uuidParent, pckRep, pckSubRep)
{
}


//----------------------------------------------------------------------------
VtkWellboreTrajectoryRepresentation::~VtkWellboreTrajectoryRepresentation()
{
	if (epcPackageRepresentation != nullptr) {
		epcPackageRepresentation = nullptr;
	}

	if (epcPackageSubRepresentation != nullptr) {
		epcPackageSubRepresentation = nullptr;
	}

	polyline.~VtkWellboreTrajectoryRepresentationPolyLine();
	head.~VtkWellboreTrajectoryRepresentationDatum();
	text.~VtkWellboreTrajectoryRepresentationText();
}

//----------------------------------------------------------------------------
void VtkWellboreTrajectoryRepresentation::createTreeVtk(const std::string & uuid, const std::string & uuidParent, const std::string & name, const VtkEpcCommon::Resqml2Type & type)
{
	if (uuid != getUuid())
	{
		this->polyline.createTreeVtk(uuid, uuidParent, name, type);
	}
}

//----------------------------------------------------------------------------
int VtkWellboreTrajectoryRepresentation::createOutput(const std::string & uuid)
{
		this->polyline.createOutput(uuid);
		//	head.createOutput(uuid);
	return 1;
}
//----------------------------------------------------------------------------
void VtkWellboreTrajectoryRepresentation::visualize(const std::string & uuid)
{
	createOutput(uuid);

	this->attach();
}

//----------------------------------------------------------------------------
void VtkWellboreTrajectoryRepresentation::remove(const std::string & uuid)
{
	if (uuid == getUuid())
	{
		this->detach();
		std::stringstream polylineUuid;
		polylineUuid << getUuid() << "-Polyline";
		polyline.remove(uuid);
	}
}

//----------------------------------------------------------------------------
void VtkWellboreTrajectoryRepresentation::attach()
{
	unsigned int index =0;
	vtkOutput->SetBlock(index, polyline.getOutput());
	vtkOutput->GetMetaData(index++)->Set(vtkCompositeDataSet::NAME(),polyline.getName().c_str());

	vtkOutput->SetBlock(index, head.getOutput());
	vtkOutput->GetMetaData(index++)->Set(vtkCompositeDataSet::NAME(),head.getName().c_str());
}

void VtkWellboreTrajectoryRepresentation::addProperty(const std::string & uuidProperty, vtkDataArray* dataProperty)
{
	this->polyline.addProperty(uuidProperty, dataProperty);
}

long VtkWellboreTrajectoryRepresentation::getAttachmentPropertyCount(const std::string & uuid, const VtkEpcCommon::FesppAttachmentProperty propertyUnit)
{
	return 	this->polyline.getAttachmentPropertyCount(uuid, propertyUnit);
	;
}
