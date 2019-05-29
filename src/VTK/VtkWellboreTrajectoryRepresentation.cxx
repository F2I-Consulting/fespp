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

#ifdef WITH_TEST
const std::string loggClass = "CLASS=VtkEpcDocument ";
#define BEGIN_FUNC(name_func) L_(linfo) << loggClass << " FUNCTION=" << name_func << " CALL_FUNCTION=none ITERATION=0 API=FESPP STATUS=START"
#define END_FUNC(name_func) L_(linfo) << loggClass << " FUNCTION=" << name_func << " CALL_FUNCTION=none ITERATION=0 API=FESPP STATUS=END"
#define CALL_FUNC(name_func, call_func, iter, api)  L_(linfo) << loggClass << " FUNCTION=" << name_func << " CALL_FUNCTION=" << call_func << " ITERATION=" << iter << " API=" << api << " STATUS=IN"
#endif

//----------------------------------------------------------------------------
VtkWellboreTrajectoryRepresentation::VtkWellboreTrajectoryRepresentation(const std::string & fileName, const std::string & name, const std::string & uuid, const std::string & uuidParent, common::EpcDocument *pckRep, common::EpcDocument *pckSubRep) :
VtkResqml2MultiBlockDataSet(fileName, name, uuid, uuidParent), epcPackageRepresentation(pckRep), epcPackageSubRepresentation(pckSubRep),
polyline(getFileName(), name, uuid+"-Polyline", uuidParent, pckRep, pckSubRep),
head(getFileName(), name, uuid+"-Head", uuidParent, pckRep, pckSubRep),
text(getFileName(), name, uuid+"-Text", uuidParent, pckRep, pckSubRep)
{
#ifdef WITH_TEST
	BEGIN_FUNC(__func__);
#endif
#ifdef WITH_TEST
	END_FUNC(__func__);
#endif
}


//----------------------------------------------------------------------------
VtkWellboreTrajectoryRepresentation::~VtkWellboreTrajectoryRepresentation()
{
#ifdef WITH_TEST
	BEGIN_FUNC(__func__);
#endif
	if (epcPackageRepresentation != nullptr) {
		epcPackageRepresentation = nullptr;
	}

	if (epcPackageSubRepresentation != nullptr) {
		epcPackageSubRepresentation = nullptr;
	}
#ifdef WITH_TEST
	END_FUNC(__func__);
#endif
}

//----------------------------------------------------------------------------
void VtkWellboreTrajectoryRepresentation::createTreeVtk(const std::string & uuid, const std::string & uuidParent, const std::string & name, const VtkEpcCommon::Resqml2Type & type)
{
#ifdef WITH_TEST
	BEGIN_FUNC(__func__);
#endif
	if (uuid != getUuid())	{
		this->polyline.createTreeVtk(uuid, uuidParent, name, type);
	}
#ifdef WITH_TEST
	END_FUNC(__func__);
#endif
}

//----------------------------------------------------------------------------
int VtkWellboreTrajectoryRepresentation::createOutput(const std::string & uuid)
{
#ifdef WITH_TEST
	BEGIN_FUNC(__func__);
#endif
		this->polyline.createOutput(uuid);
		//	head.createOutput(uuid);
#ifdef WITH_TEST
	END_FUNC(__func__);
#endif
	return 1;
}
//----------------------------------------------------------------------------
void VtkWellboreTrajectoryRepresentation::visualize(const std::string & uuid)
{
#ifdef WITH_TEST
	BEGIN_FUNC(__func__);
#endif
	createOutput(uuid);

	this->attach();
#ifdef WITH_TEST
	END_FUNC(__func__);
#endif
}

//----------------------------------------------------------------------------
void VtkWellboreTrajectoryRepresentation::remove(const std::string & uuid)
{
#ifdef WITH_TEST
	BEGIN_FUNC(__func__);
#endif
	if (uuid == getUuid())
	{
		this->detach();
		std::stringstream polylineUuid;
		polylineUuid << getUuid() << "-Polyline";
		polyline.remove(uuid);
	}
#ifdef WITH_TEST
	END_FUNC(__func__);
#endif
}

//----------------------------------------------------------------------------
void VtkWellboreTrajectoryRepresentation::attach()
{
#ifdef WITH_TEST
	BEGIN_FUNC(__func__);
#endif
	unsigned int index =0;
	vtkOutput->SetBlock(index, polyline.getOutput());
	vtkOutput->GetMetaData(index++)->Set(vtkCompositeDataSet::NAME(),polyline.getName().c_str());

	vtkOutput->SetBlock(index, head.getOutput());
	vtkOutput->GetMetaData(index++)->Set(vtkCompositeDataSet::NAME(),head.getName().c_str());
#ifdef WITH_TEST
	END_FUNC(__func__);
#endif
}

//----------------------------------------------------------------------------
void VtkWellboreTrajectoryRepresentation::addProperty(const std::string & uuidProperty, vtkDataArray* dataProperty)
{
#ifdef WITH_TEST
	BEGIN_FUNC(__func__);
#endif
	this->polyline.addProperty(uuidProperty, dataProperty);
#ifdef WITH_TEST
	END_FUNC(__func__);
#endif
}

//----------------------------------------------------------------------------
long VtkWellboreTrajectoryRepresentation::getAttachmentPropertyCount(const std::string & uuid, const VtkEpcCommon::FesppAttachmentProperty propertyUnit)
{
	return 	this->polyline.getAttachmentPropertyCount(uuid, propertyUnit);
}
