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
#include "VtkSetPatch.h"

// include system
#include <algorithm>
#include <sstream>

// include VTK library
#include <vtkInformation.h>

// include F2i-consulting Energistics Standards API
#include <resqml2_0_1/SubRepresentation.h>
#include <resqml2_0_1/PolylineSetRepresentation.h>
#include <resqml2_0_1/TriangulatedSetRepresentation.h>

// include F2i-consulting Energistics Standards ParaView Plugin
#include "VtkPolylineRepresentation.h"
#include "VtkTriangulatedRepresentation.h"
#include "VtkProperty.h"

#ifdef WITH_TEST
const std::string loggClass = "CLASS=VtkSetPatch ";
#define BEGIN_FUNC(name_func) L_(linfo) << loggClass << " FUNCTION=" << name_func << " CALL_FUNCTUION=none ITERATION=0 API=FESPP STATUS=START"
#define END_FUNC(name_func) L_(linfo) << loggClass << " FUNCTION=" << name_func << " CALL_FUNCTUION=none ITERATION=0 API=FESPP STATUS=END"
#define CALL_FUNC(name_func, call_func, iter, api)  L_(linfo) << loggClass << " FUNCTION=" << name_func << " CALL_FUNCTUION=" << call_func << " ITERATION=" << iter << " API=" << api << " STATUS=IN"
#endif

//----------------------------------------------------------------------------
VtkSetPatch::VtkSetPatch(const std::string & fileName, const std::string & name, const std:: string & uuid, const std::string & uuidParent, common::EpcDocument *pck, const int & idProc, const int & maxProc):
VtkResqml2MultiBlockDataSet(fileName, name, uuid, uuidParent, idProc, maxProc), epcPackage(pck)
{
#ifdef WITH_TEST
	BEGIN_FUNC(__func__);
#endif
//	uuidIsChildOf[uuid] = new VtkEpcCommon();
	resqml2_0_1::PolylineSetRepresentation* polylineSetRep = nullptr;
	resqml2_0_1::TriangulatedSetRepresentation * triangulatedSetRep = nullptr;
	common::AbstractObject *object = epcPackage->getDataObjectByUuid(uuid);
	std::string xmlTag = object->getXmlTag();
	if (xmlTag == "PolylineSetRepresentation")
	{
		polylineSetRep = static_cast<resqml2_0_1::PolylineSetRepresentation*>(object);
		if (polylineSetRep == nullptr)
			cout << "Not found an PolylineSetRepresentation with the right uuid." << endl;
		for (unsigned int patchIndex = 0; patchIndex < polylineSetRep->getPatchCount(); ++ patchIndex)
		{
			std::stringstream sstm;
			sstm << "Patch " << patchIndex;
			uuidToVtkPolylineRepresentation[uuid].push_back(new VtkPolylineRepresentation(fileName, sstm.str(), uuid, uuidParent, patchIndex, epcPackage, nullptr));
		}
		uuidIsChildOf[uuid].setType( VtkEpcCommon::POLYLINE_SET);
		uuidIsChildOf[uuid].setUuid( uuid);
	}
	else if (xmlTag == "TriangulatedSetRepresentation")
	{
		triangulatedSetRep = static_cast<resqml2_0_1::TriangulatedSetRepresentation*>(object);
		if (triangulatedSetRep == nullptr)
			cout << "Not found an ijk grid with the right uuid." << endl;
		for (unsigned int patchIndex = 0; patchIndex < triangulatedSetRep->getPatchCount(); ++ patchIndex)
		{
			std::stringstream sstm;
			sstm << "Patch " << patchIndex;
			uuidToVtkTriangulatedRepresentation[uuid].push_back(new VtkTriangulatedRepresentation(fileName, sstm.str(), uuid, uuidParent, patchIndex, epcPackage, nullptr));
		}
		uuidIsChildOf[uuid].setType(VtkEpcCommon::TRIANGULATED_SET);
		uuidIsChildOf[uuid].setUuid(uuid);
	}
#ifdef WITH_TEST
	END_FUNC(__func__);
#endif
}

//----------------------------------------------------------------------------
VtkSetPatch::~VtkSetPatch()
{
#ifdef WITH_TEST
	BEGIN_FUNC(__func__);
#endif
	if (epcPackage != nullptr) {
		epcPackage = nullptr;
	}

	for(auto i : uuidToVtkPolylineRepresentation) {
		for (const auto& elem : i.second) {
			delete elem;
		}
	}
	uuidToVtkPolylineRepresentation.clear();

	for(auto i : uuidToVtkTriangulatedRepresentation) {
		for (const auto& elem : i.second) {
			delete elem;
		}
	}
	uuidToVtkTriangulatedRepresentation.clear();

	uuidToVtkProperty.clear();
#ifdef WITH_TEST
	END_FUNC(__func__);
#endif

}

//----------------------------------------------------------------------------
void VtkSetPatch::createTreeVtk(const std::string & uuid, const std::string & parent, const std::string & name, const VtkEpcCommon::Resqml2Type & type)
{
#ifdef WITH_TEST
	BEGIN_FUNC(__func__);
#endif
	uuidIsChildOf[uuid].setType( type);
	uuidIsChildOf[uuid].setUuid( uuid);

	resqml2_0_1::PolylineSetRepresentation* polylineSetRep = nullptr;
	resqml2_0_1::TriangulatedSetRepresentation * triangulatedSetRep = nullptr;
	common::AbstractObject* obj = epcPackage->getDataObjectByUuid(getUuid());

	// PROPERTY
	uuidIsChildOf[uuid].setType( uuidIsChildOf[parent].getType());
	uuidIsChildOf[uuid].setUuid( uuidIsChildOf[parent].getUuid());
	if (uuidIsChildOf[parent].getType() == VtkEpcCommon::POLYLINE_SET)
	{
		if ((obj != nullptr && obj->getXmlTag() == "PolylineRepresentation") || (obj != nullptr && obj->getXmlTag() == "PolylineSetRepresentation"))
		{
			polylineSetRep = static_cast<resqml2_0_1::PolylineSetRepresentation*>(obj);
		}

		for (unsigned int patchIndex = 0; patchIndex < polylineSetRep->getPatchCount(); ++ patchIndex)
		{
			uuidToVtkPolylineRepresentation[uuidIsChildOf[uuid].getUuid()][patchIndex]->createTreeVtk(uuid, parent, name, type);
		}
	}
	if (uuidIsChildOf[parent].getType() == VtkEpcCommon::TRIANGULATED_SET)
	{
		if (obj != nullptr && obj->getXmlTag() ==  "TriangulatedSetRepresentation")
		{
			triangulatedSetRep = static_cast<resqml2_0_1::TriangulatedSetRepresentation*>(obj);
		}

		for (unsigned int patchIndex = 0; patchIndex < triangulatedSetRep->getPatchCount(); ++ patchIndex)
		{
			uuidToVtkTriangulatedRepresentation[uuidIsChildOf[uuid].getUuid()][patchIndex]->createTreeVtk(uuid, parent, name, type);
		}
	}
#ifdef WITH_TEST
	END_FUNC(__func__);
#endif
}

//----------------------------------------------------------------------------
void VtkSetPatch::visualize(const std::string & uuid)
{
#ifdef WITH_TEST
	BEGIN_FUNC(__func__);
#endif
	std::vector<VtkPolylineRepresentation *> vectPolyPatch;
	std::vector<VtkTriangulatedRepresentation *> vectTriPatch;
	switch (uuidIsChildOf[uuid].getType())
	{
	case VtkEpcCommon::POLYLINE_SET:
		vectPolyPatch = uuidToVtkPolylineRepresentation[uuidIsChildOf[uuid].getUuid()];
		for (unsigned int patchIndex = 0; patchIndex < vectPolyPatch.size(); ++patchIndex)
						{
							uuidToVtkPolylineRepresentation[uuidIsChildOf[uuid].getUuid()][patchIndex]->visualize(uuid);
						}


		break;
	case VtkEpcCommon::TRIANGULATED_SET:
		vectTriPatch = uuidToVtkTriangulatedRepresentation[uuidIsChildOf[uuid].getUuid()];
		for (unsigned int patchIndex = 0; patchIndex < vectTriPatch.size(); ++patchIndex)
		{
			uuidToVtkTriangulatedRepresentation[uuidIsChildOf[uuid].getUuid()][patchIndex]->visualize(uuid);
		}

		break;
	default:
		break;
	}

	std::string parent = uuidIsChildOf[uuid].getUuid();
	if (std::find(attachUuids.begin(), attachUuids.end(), parent ) == attachUuids.end())
	{
		this->detach();
		attachUuids.push_back(parent);
		this->attach();
	}
#ifdef WITH_TEST
	END_FUNC(__func__);
#endif
}

//----------------------------------------------------------------------------
void VtkSetPatch::remove(const std::string & uuid)
{
#ifdef WITH_TEST
	BEGIN_FUNC(__func__);
#endif
	switch (uuidIsChildOf[uuid].getType())
	{
	case VtkEpcCommon::POLYLINE_SET:
		for (unsigned int patchIndex = 0; patchIndex < uuidToVtkPolylineRepresentation[uuidIsChildOf[uuid].getUuid()].size(); ++patchIndex)
		{
			uuidToVtkPolylineRepresentation[uuidIsChildOf[uuid].getUuid()][patchIndex]->remove(uuid);
		}
		break;
	case VtkEpcCommon::TRIANGULATED_SET:
		for (unsigned int patchIndex = 0; patchIndex < uuidToVtkTriangulatedRepresentation[uuidIsChildOf[uuid].getUuid()].size(); ++patchIndex)
		{
			uuidToVtkTriangulatedRepresentation[uuidIsChildOf[uuid].getUuid()][patchIndex]->remove(uuid);
		}
		break;
	default:
		break;
	}

	if (!(std::find(attachUuids.begin(), attachUuids.end(), uuid ) == attachUuids.end()))
	{
		this->detach();
		attachUuids.erase(std::find(attachUuids.begin(), attachUuids.end(), uuid ));
		this->attach();
	}
#ifdef WITH_TEST
	END_FUNC(__func__);
#endif
}

//----------------------------------------------------------------------------
void VtkSetPatch::attach()
{
#ifdef WITH_TEST
	BEGIN_FUNC(__func__);
#endif
	unsigned int indexTmp = 0;

	for (unsigned int newBlockIndex = 0; newBlockIndex < attachUuids.size(); ++newBlockIndex)
	{
		std::string uuid = attachUuids[newBlockIndex];
		switch (uuidIsChildOf[uuid].getType())
		{
		case VtkEpcCommon::POLYLINE_SET:
			for (unsigned int patchIndex = 0; patchIndex < uuidToVtkPolylineRepresentation[uuidIsChildOf[uuid].getUuid()].size(); ++patchIndex)
			{
				vtkOutput->SetBlock(indexTmp, uuidToVtkPolylineRepresentation[uuid][patchIndex]->getOutput());
				vtkOutput->GetMetaData(indexTmp)->Set(vtkCompositeDataSet::NAME(),uuidToVtkPolylineRepresentation[uuid][patchIndex]->getName().c_str());
				++indexTmp;
			}
			break;
		case VtkEpcCommon::TRIANGULATED_SET:
			for (unsigned int patchIndex = 0; patchIndex < uuidToVtkTriangulatedRepresentation[uuidIsChildOf[uuid].getUuid()].size(); ++patchIndex)
			{
				vtkOutput->SetBlock(indexTmp, uuidToVtkTriangulatedRepresentation[uuid][patchIndex]->getOutput());
				vtkOutput->GetMetaData(indexTmp)->Set(vtkCompositeDataSet::NAME(),uuidToVtkTriangulatedRepresentation[uuid][patchIndex]->getName().c_str());
				++indexTmp;
			}
			break;
		default:
			break;
		}
	}
#ifdef WITH_TEST
	END_FUNC(__func__);
#endif
}

//----------------------------------------------------------------------------
void VtkSetPatch::addProperty(const std::string & uuidProperty, vtkDataArray* dataProperty)
{
#ifdef WITH_TEST
	BEGIN_FUNC(__func__);
#endif
	std::vector<VtkPolylineRepresentation *> vectPolyPatch;
	std::vector<VtkTriangulatedRepresentation *> vectTriPatch;
	switch (uuidIsChildOf[uuidProperty].getType())
	{
	case VtkEpcCommon::POLYLINE_SET:
		vectPolyPatch = uuidToVtkPolylineRepresentation[uuidIsChildOf[uuidProperty].getUuid()];
		for (unsigned int patchIndex = 0; patchIndex < vectPolyPatch.size(); ++patchIndex)
		{
			uuidToVtkPolylineRepresentation[uuidIsChildOf[uuidProperty].getUuid()][patchIndex]->addProperty(uuidProperty, dataProperty);
		}


		break;
	case VtkEpcCommon::TRIANGULATED_SET:
		vectTriPatch = uuidToVtkTriangulatedRepresentation[uuidIsChildOf[uuidProperty].getUuid()];
		for (unsigned int patchIndex = 0; patchIndex < vectTriPatch.size(); ++patchIndex)
		{
			uuidToVtkTriangulatedRepresentation[uuidIsChildOf[uuidProperty].getUuid()][patchIndex]->addProperty(uuidProperty, dataProperty);
		}

		break;
	default:
		break;
	}

	std::string parent = uuidIsChildOf[uuidProperty].getUuid();
	if (std::find(attachUuids.begin(), attachUuids.end(), parent) == attachUuids.end())
	{
		this->detach();
		attachUuids.push_back(parent);
		this->attach();
	}
#ifdef WITH_TEST
	END_FUNC(__func__);
#endif
}

//----------------------------------------------------------------------------
long VtkSetPatch::getAttachmentPropertyCount(const std::string & uuid, const VtkEpcCommon::FesppAttachmentProperty propertyUnit)
{
#ifdef WITH_TEST
	BEGIN_FUNC(__func__);
#endif
	long result = 0;
	std::vector<VtkPolylineRepresentation *> vectPolyPatch;
	std::vector<VtkTriangulatedRepresentation *> vectTriPatch;
	switch (uuidIsChildOf[uuid].getType())
	{
	case VtkEpcCommon::POLYLINE_SET:
		vectPolyPatch = uuidToVtkPolylineRepresentation[uuidIsChildOf[uuid].getUuid()];
		for (unsigned int patchIndex = 0; patchIndex < vectPolyPatch.size(); ++patchIndex)
		{
			result=uuidToVtkPolylineRepresentation[uuidIsChildOf[uuid].getUuid()][patchIndex]->getAttachmentPropertyCount(uuid, propertyUnit);
		}
		break;
	case VtkEpcCommon::TRIANGULATED_SET:
		vectTriPatch = uuidToVtkTriangulatedRepresentation[uuidIsChildOf[uuid].getUuid()];
		for (unsigned int patchIndex = 0; patchIndex < vectTriPatch.size(); ++patchIndex)
		{
			result=uuidToVtkTriangulatedRepresentation[uuidIsChildOf[uuid].getUuid()][patchIndex]->getAttachmentPropertyCount(uuid, propertyUnit);
		}
		break;
	default:
		break;
	}
#ifdef WITH_TEST
	END_FUNC(__func__);
#endif
	return result;
}
