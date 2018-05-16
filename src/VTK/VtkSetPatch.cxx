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

//----------------------------------------------------------------------------
VtkSetPatch::VtkSetPatch(const std::string & fileName, const std::string & name, const std:: string & uuid, const std::string & uuidParent, common::EpcDocument *pck, const int & idProc, const int & maxProc):
VtkResqml2MultiBlockDataSet(fileName, name, uuid, uuidParent, idProc, maxProc), epcPackage(pck)
{
	resqml2_0_1::PolylineSetRepresentation* polylineSetRep = nullptr;
	resqml2_0_1::TriangulatedSetRepresentation * triangulatedSetRep = nullptr;
	common::AbstractObject *object = epcPackage->getResqmlAbstractObjectByUuid(uuid);
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
		uuidIsChildOf[uuid].myType = Resqml2Type::POLYLINE_SET;
		uuidIsChildOf[uuid].uuid = uuid;
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
		uuidIsChildOf[uuid].myType = Resqml2Type::TRIANGULATED_SET;
		uuidIsChildOf[uuid].uuid = uuid;
	}
}

//----------------------------------------------------------------------------
VtkSetPatch::~VtkSetPatch()
{
	cout << "VtkSetPatch::~VtkSetPatch() " << getUuid() << "\n";
	if (epcPackage != nullptr) {
		epcPackage = nullptr;
	}

	uuidToVtkPolylineRepresentation.clear();
	uuidToVtkTriangulatedRepresentation.clear();

	// delete uuidToVtkProperty
#if _MSC_VER < 1600
	for (std::tr1::unordered_map< std::string, VtkProperty* >::const_iterator it = uuidToVtkProperty.begin(); it != uuidToVtkProperty.end(); ++it)
#else
	for (std::unordered_map< std::string, VtkProperty* >::const_iterator it = uuidToVtkProperty.begin(); it != uuidToVtkProperty.end(); ++it)
#endif
	{
	  delete it->second;
	}
	uuidToVtkProperty.clear();

}

//----------------------------------------------------------------------------
void VtkSetPatch::createTreeVtk(const std::string & uuid, const std::string & parent, const std::string & name, const Resqml2Type & type)
{
	uuidIsChildOf[uuid].myType = type;
	uuidIsChildOf[uuid].uuid = uuid;

	resqml2_0_1::PolylineSetRepresentation* polylineSetRep = nullptr;
	resqml2_0_1::TriangulatedSetRepresentation * triangulatedSetRep = nullptr;
	common::AbstractObject* obj = epcPackage->getResqmlAbstractObjectByUuid(getUuid());

	// PROPERTY
	uuidIsChildOf[uuid].myType = uuidIsChildOf[parent].myType;
	uuidIsChildOf[uuid].uuid = uuidIsChildOf[parent].uuid;
	if (uuidIsChildOf[parent].myType == Resqml2Type::POLYLINE_SET)
	{
		if ((obj != nullptr && obj->getXmlTag() == "PolylineRepresentation") || (obj != nullptr && obj->getXmlTag() == "PolylineSetRepresentation"))
		{
			polylineSetRep = static_cast<resqml2_0_1::PolylineSetRepresentation*>(obj);
		}

		for (unsigned int patchIndex = 0; patchIndex < polylineSetRep->getPatchCount(); ++ patchIndex)
		{
			uuidToVtkPolylineRepresentation[uuidIsChildOf[uuid].uuid][patchIndex]->createTreeVtk(uuid, parent, name, type);
		}
	}
	if (uuidIsChildOf[parent].myType == Resqml2Type::TRIANGULATED_SET)
	{
		if (obj != nullptr && obj->getXmlTag() ==  "TriangulatedSetRepresentation")
		{
			triangulatedSetRep = static_cast<resqml2_0_1::TriangulatedSetRepresentation*>(obj);
		}

		for (unsigned int patchIndex = 0; patchIndex < triangulatedSetRep->getPatchCount(); ++ patchIndex)
		{
			uuidToVtkTriangulatedRepresentation[uuidIsChildOf[uuid].uuid][patchIndex]->createTreeVtk(uuid, parent, name, type);
		}
	}
}

//----------------------------------------------------------------------------
void VtkSetPatch::visualize(const std::string & uuid)
{
	std::vector<VtkPolylineRepresentation *> vectPolyPatch;
	std::vector<VtkTriangulatedRepresentation *> vectTriPatch;
	switch (uuidIsChildOf[uuid].myType)
	{
	case Resqml2Type::POLYLINE_SET:
		vectPolyPatch = uuidToVtkPolylineRepresentation[uuidIsChildOf[uuid].uuid];
		for (unsigned int patchIndex = 0; patchIndex < vectPolyPatch.size(); ++patchIndex)
						{
							uuidToVtkPolylineRepresentation[uuidIsChildOf[uuid].uuid][patchIndex]->visualize(uuid);
						}


		break;
	case Resqml2Type::TRIANGULATED_SET:
		vectTriPatch = uuidToVtkTriangulatedRepresentation[uuidIsChildOf[uuid].uuid];
		for (unsigned int patchIndex = 0; patchIndex < vectTriPatch.size(); ++patchIndex)
		{
			uuidToVtkTriangulatedRepresentation[uuidIsChildOf[uuid].uuid][patchIndex]->visualize(uuid);
		}

		break;
	default:
		break;
	}

	std::string parent = uuidIsChildOf[uuid].uuid;
	if (std::find(attachUuids.begin(), attachUuids.end(), parent ) == attachUuids.end())
	{
		this->detach();
		attachUuids.push_back(parent);
		this->attach();
	}
}

//----------------------------------------------------------------------------
void VtkSetPatch::remove(const std::string & uuid)
{
	switch (uuidIsChildOf[uuid].myType)
	{
	case Resqml2Type::POLYLINE_SET:
		for (unsigned int patchIndex = 0; patchIndex < uuidToVtkPolylineRepresentation[uuidIsChildOf[uuid].uuid].size(); ++patchIndex)
		{
			uuidToVtkPolylineRepresentation[uuidIsChildOf[uuid].uuid][patchIndex]->remove(uuid);
		}
		break;
	case Resqml2Type::TRIANGULATED_SET:
		for (unsigned int patchIndex = 0; patchIndex < uuidToVtkTriangulatedRepresentation[uuidIsChildOf[uuid].uuid].size(); ++patchIndex)
		{
			uuidToVtkTriangulatedRepresentation[uuidIsChildOf[uuid].uuid][patchIndex]->remove(uuid);
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
}

//----------------------------------------------------------------------------
void VtkSetPatch::attach()
{
	unsigned int indexTmp = 0;

	for (unsigned int newBlockIndex = 0; newBlockIndex < attachUuids.size(); ++newBlockIndex)
	{
		std::string uuid = attachUuids[newBlockIndex];
		switch (uuidIsChildOf[uuid].myType)
		{
		case Resqml2Type::POLYLINE_SET:
			for (unsigned int patchIndex = 0; patchIndex < uuidToVtkPolylineRepresentation[uuidIsChildOf[uuid].uuid].size(); ++patchIndex)
			{
				vtkOutput->SetBlock(indexTmp, uuidToVtkPolylineRepresentation[uuid][patchIndex]->getOutput());
				vtkOutput->GetMetaData(indexTmp)->Set(vtkCompositeDataSet::NAME(),uuidToVtkPolylineRepresentation[uuid][patchIndex]->getName().c_str());
				++indexTmp;
			}
			break;
		case Resqml2Type::TRIANGULATED_SET:
			for (unsigned int patchIndex = 0; patchIndex < uuidToVtkTriangulatedRepresentation[uuidIsChildOf[uuid].uuid].size(); ++patchIndex)
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
}

void VtkSetPatch::addProperty(const std::string & uuidProperty, vtkDataArray* dataProperty)
{
	std::vector<VtkPolylineRepresentation *> vectPolyPatch;
	std::vector<VtkTriangulatedRepresentation *> vectTriPatch;
	switch (uuidIsChildOf[uuidProperty].myType)
	{
	case Resqml2Type::POLYLINE_SET:
		vectPolyPatch = uuidToVtkPolylineRepresentation[uuidIsChildOf[uuidProperty].uuid];
		for (unsigned int patchIndex = 0; patchIndex < vectPolyPatch.size(); ++patchIndex)
		{
			uuidToVtkPolylineRepresentation[uuidIsChildOf[uuidProperty].uuid][patchIndex]->addProperty(uuidProperty, dataProperty);
		}


		break;
	case Resqml2Type::TRIANGULATED_SET:
		vectTriPatch = uuidToVtkTriangulatedRepresentation[uuidIsChildOf[uuidProperty].uuid];
		for (unsigned int patchIndex = 0; patchIndex < vectTriPatch.size(); ++patchIndex)
		{
			uuidToVtkTriangulatedRepresentation[uuidIsChildOf[uuidProperty].uuid][patchIndex]->addProperty(uuidProperty, dataProperty);
		}

		break;
	default:
		break;
	}

	std::string parent = uuidIsChildOf[uuidProperty].uuid;
	if (std::find(attachUuids.begin(), attachUuids.end(), parent) == attachUuids.end())
	{
		this->detach();
		attachUuids.push_back(parent);
		this->attach();
	}
}


long VtkSetPatch::getAttachmentPropertyCount(const std::string & uuid, const FesppAttachmentProperty propertyUnit)
{
	long result = 0;
	std::vector<VtkPolylineRepresentation *> vectPolyPatch;
	std::vector<VtkTriangulatedRepresentation *> vectTriPatch;
	switch (uuidIsChildOf[uuid].myType)
	{
	case Resqml2Type::POLYLINE_SET:
		vectPolyPatch = uuidToVtkPolylineRepresentation[uuidIsChildOf[uuid].uuid];
		for (unsigned int patchIndex = 0; patchIndex < vectPolyPatch.size(); ++patchIndex)
		{
			result=uuidToVtkPolylineRepresentation[uuidIsChildOf[uuid].uuid][patchIndex]->getAttachmentPropertyCount(uuid, propertyUnit);
		}
		break;
	case Resqml2Type::TRIANGULATED_SET:
		vectTriPatch = uuidToVtkTriangulatedRepresentation[uuidIsChildOf[uuid].uuid];
		for (unsigned int patchIndex = 0; patchIndex < vectTriPatch.size(); ++patchIndex)
		{
			result=uuidToVtkTriangulatedRepresentation[uuidIsChildOf[uuid].uuid][patchIndex]->getAttachmentPropertyCount(uuid, propertyUnit);
		}
		break;
	default:
		break;
	}
	return result;
}
