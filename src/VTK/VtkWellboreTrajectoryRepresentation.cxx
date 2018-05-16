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
VtkResqml2MultiBlockDataSet(fileName, name, uuid, uuidParent), epcPackageRepresentation(pckRep), epcPackageSubRepresentation(pckSubRep)
{
	std::stringstream polylineUuid;
	polylineUuid << uuid << "-Polyline";
	polyline = new VtkWellboreTrajectoryRepresentationPolyLine(getFileName(), name, polylineUuid.str(), uuidParent, pckRep, pckSubRep);

	std::stringstream headUuid;
	headUuid << uuid << "-Head";
	head = new VtkWellboreTrajectoryRepresentationDatum(getFileName(), name, headUuid.str(), uuidParent, pckRep, pckSubRep);

	std::stringstream textUuid;
	textUuid << uuid << "-Text";
	text = new VtkWellboreTrajectoryRepresentationText(getFileName(), name, textUuid.str(), uuidParent, pckRep, pckSubRep);
}


//----------------------------------------------------------------------------
VtkWellboreTrajectoryRepresentation::~VtkWellboreTrajectoryRepresentation()
{
	cout << "VtkWellboreTrajectoryRepresentation::~VtkWellboreTrajectoryRepresentation() " << getUuid() << "\n";
	if (epcPackageRepresentation != nullptr) {
		epcPackageRepresentation = nullptr;
	}

	if (epcPackageSubRepresentation != nullptr) {
		epcPackageSubRepresentation = nullptr;
	}

	if (polyline != nullptr) {
		delete polyline;
		polyline = nullptr;
	}
	if (head != nullptr) {
		delete head;
		head = nullptr;
	}
	if (head != nullptr) {
		delete head;
		head = nullptr;
	}
	if (text != nullptr) {
		delete text;
		text = nullptr;
	}
}

//----------------------------------------------------------------------------
void VtkWellboreTrajectoryRepresentation::createTreeVtk(const std::string & uuid, const std::string & uuidParent, const std::string & name, const Resqml2Type & type)
{
	if (uuid != getUuid())
	{
		this->polyline->createTreeVtk(uuid, uuidParent, name, type);
	}
}

//----------------------------------------------------------------------------
int VtkWellboreTrajectoryRepresentation::createOutput(const std::string & uuid)
{
		this->polyline->createOutput(uuid);
		//	head->createOutput(uuid);
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
		polyline->remove(uuid); 

		std::stringstream headUuid;
		headUuid << getUuid() << "-Head";
		head = new VtkWellboreTrajectoryRepresentationDatum(getFileName(), getName(), polylineUuid.str(), getParent(), epcPackageRepresentation, epcPackageSubRepresentation);

		std::stringstream textUuid;
		textUuid << getUuid() << "-Text";
		text = new VtkWellboreTrajectoryRepresentationText(getFileName(), getName(), polylineUuid.str(), getParent(), epcPackageRepresentation, epcPackageSubRepresentation);
	}
}

//----------------------------------------------------------------------------
void VtkWellboreTrajectoryRepresentation::attach()
{
	unsigned int index =0;
	vtkOutput->SetBlock(index, polyline->getOutput());
	vtkOutput->GetMetaData(index++)->Set(vtkCompositeDataSet::NAME(),polyline->getName().c_str());

	vtkOutput->SetBlock(index, head->getOutput());
	vtkOutput->GetMetaData(index++)->Set(vtkCompositeDataSet::NAME(),head->getName().c_str());
}

void VtkWellboreTrajectoryRepresentation::addProperty(const std::string & uuidProperty, vtkDataArray* dataProperty)
{
	this->polyline->addProperty(uuidProperty, dataProperty);
}

long VtkWellboreTrajectoryRepresentation::getAttachmentPropertyCount(const std::string & uuid, const FesppAttachmentProperty propertyUnit)
{
	return 	this->polyline->getAttachmentPropertyCount(uuid, propertyUnit);
	;
}
