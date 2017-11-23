#include "VtkEpcDocument.h"

// include system
#include <algorithm>

// include Vtk
#include <vtkInformation.h>

// include FESAPI
#include <EpcDocument.h>

// include Vtk for Plugin
#include "VtkGrid2DRepresentation.h"
#include "VtkPolylineRepresentation.h"
#include "VtkTriangulatedRepresentation.h"
#include "VtkWellboreTrajectoryRepresentation.h"
#include "VtkUnstructuredGridRepresentation.h"
#include "VtkIjkGridRepresentation.h"
#include "VtkPartialRepresentation.h"
#include "VtkSetPatch.h"

//----------------------------------------------------------------------------
VtkEpcDocument::VtkEpcDocument(const std::string & fileName, common::EpcDocument *pck) :
VtkResqml2MultiBlockDataSet(fileName, fileName, fileName, ""), epcPackage(pck)
{
}

//----------------------------------------------------------------------------
VtkEpcDocument::~VtkEpcDocument()
{
	epcPackage->close();
}

//----------------------------------------------------------------------------
void VtkEpcDocument::createTreeVtk(const std::string & uuid, const std::string & parent, const std::string & name, const Resqml2Type & type)
{
	uuidIsChildOf[uuid].type = type;
	uuidIsChildOf[uuid].uuid = uuid;
	switch (type){
	case Resqml2Type::GRID_2D:	{
									uuidToVtkGrid2DRepresentation[uuid] = new VtkGrid2DRepresentation(getFileName(), name, uuid, parent, epcPackage, nullptr);
									break;
	}
	case Resqml2Type::POLYLINE_SET:	{
										auto object = epcPackage->getResqmlAbstractObjectByUuid(uuid);
										auto typeRepresentation = object->getXmlTag();
										if (typeRepresentation == "PolylineRepresentation") {
											uuidToVtkPolylineRepresentation[uuid] = new VtkPolylineRepresentation(getFileName(), name, uuid, parent, 0, epcPackage, nullptr);
										}
										else  {
											uuidToVtkSetPatch[uuid] = new VtkSetPatch(getFileName(), name, uuid, parent, epcPackage);
										}
										break;
	}
	case Resqml2Type::TRIANGULATED_SET:	{
											auto object = epcPackage->getResqmlAbstractObjectByUuid(uuid);
											auto typeRepresentation = object->getXmlTag();
											if (typeRepresentation == "TriangulatedRepresentation")  {
												uuidToVtkTriangulatedRepresentation[uuid] = new VtkTriangulatedRepresentation(getFileName(), name, uuid, parent, 0, epcPackage, nullptr);
											}
											else  {
												uuidToVtkSetPatch[uuid] = new VtkSetPatch(getFileName(), name, uuid, parent, epcPackage);
											}
											break;
	}
	case Resqml2Type::WELL_TRAJ:	{
										uuidToVtkWellboreTrajectoryRepresentation[uuid] = new VtkWellboreTrajectoryRepresentation(getFileName(), name, uuid, parent, epcPackage, nullptr);
										break;
	}
	case Resqml2Type::IJK_GRID:	{
									uuidToVtkIjkGridRepresentation[uuid] = new VtkIjkGridRepresentation(getFileName(), name, uuid, parent, epcPackage, nullptr);
									break;
	}
	case Resqml2Type::UNSTRUC_GRID:	{
										uuidToVtkUnstructuredGridRepresentation[uuid] = new VtkUnstructuredGridRepresentation(getFileName(), name, uuid, parent, epcPackage, nullptr);
										break;
	}
	case Resqml2Type::SUB_REP:	{
									uuidIsChildOf[uuid].type = uuidIsChildOf[parent].type;
									if (uuidIsChildOf[uuid].type == Resqml2Type::IJK_GRID){
										uuidToVtkIjkGridRepresentation[uuid] = new VtkIjkGridRepresentation(getFileName(), name, uuid, parent, epcPackage, epcPackage);
										uuidToVtkIjkGridRepresentation[uuid]->setSubRepresentation();
									}
									if (uuidIsChildOf[uuid].type == Resqml2Type::UNSTRUC_GRID){
										uuidToVtkUnstructuredGridRepresentation[uuid] = new VtkUnstructuredGridRepresentation(getFileName(), name, uuid, parent, epcPackage, epcPackage);
										uuidToVtkUnstructuredGridRepresentation[uuid]->setSubRepresentation();
									}
									if (uuidIsChildOf[uuid].type == Resqml2Type::PARTIAL){
										auto SubRepType = uuidToVtkPartialRepresentation[parent]->getType();
										auto pckEPCsrc = uuidToVtkPartialRepresentation[parent]->getEpcSource();

										switch (SubRepType){
										case Resqml2Type::GRID_2D:	{
																		uuidIsChildOf[uuid].type = Resqml2Type::GRID_2D;
																		uuidToVtkGrid2DRepresentation[uuid] = new VtkGrid2DRepresentation(getFileName(), name, uuid, parent, pckEPCsrc, epcPackage);
																		break;
										}
										case Resqml2Type::WELL_TRAJ:	{
																			uuidIsChildOf[uuid].type = Resqml2Type::WELL_TRAJ;
																			uuidToVtkWellboreTrajectoryRepresentation[uuid] = new VtkWellboreTrajectoryRepresentation(getFileName(), name, uuid, parent, pckEPCsrc, epcPackage);
																			break;
										}
										case Resqml2Type::IJK_GRID:	{
																		uuidIsChildOf[uuid].type = Resqml2Type::IJK_GRID;
																		uuidToVtkIjkGridRepresentation[uuid] = new VtkIjkGridRepresentation(getFileName(), name, uuid, parent, pckEPCsrc, epcPackage);
																		break;
										}
										case Resqml2Type::UNSTRUC_GRID:	{
																			uuidIsChildOf[uuid].type = Resqml2Type::UNSTRUC_GRID;
																			uuidToVtkUnstructuredGridRepresentation[uuid] = new VtkUnstructuredGridRepresentation(getFileName(), name, uuid, parent, pckEPCsrc, epcPackage);
																			break;
										}
										}
										break;
									}
									break;
	}
	case Resqml2Type::PROPERTY:	{
									uuidIsChildOf[uuid].uuid = parent;
									uuidIsChildOf[uuid].type = uuidIsChildOf[parent].type;
									switch (uuidIsChildOf[parent].type)	{
									case Resqml2Type::GRID_2D:	{
																	uuidToVtkGrid2DRepresentation[parent]->createTreeVtk(uuid, parent, name, type);
																	break;
									}
									case Resqml2Type::POLYLINE_SET:	{
																		auto object = epcPackage->getResqmlAbstractObjectByUuid(parent);
																		auto typeRepresentation = object->getXmlTag();
																		if (typeRepresentation == "PolylineRepresentation")	{
																			uuidToVtkPolylineRepresentation[parent]->createTreeVtk(uuid, parent, name, type);
																		}
																		else{
																			uuidToVtkSetPatch[parent]->createTreeVtk(uuid, parent, name, type);
																		}
																		break;
									}
									case Resqml2Type::TRIANGULATED_SET:	{
																			auto object = epcPackage->getResqmlAbstractObjectByUuid(uuid);
																			auto typeRepresentation = object->getXmlTag();
																			if (typeRepresentation == "TriangulatedRepresentation")	{
																				uuidToVtkTriangulatedRepresentation[parent]->createTreeVtk(uuid, parent, name, type);
																			}
																			else{
																				uuidToVtkSetPatch[parent]->createTreeVtk(uuid, parent, name, type);
																			}
																			break;
									}
									case Resqml2Type::WELL_TRAJ:	{
																		uuidToVtkWellboreTrajectoryRepresentation[parent]->createTreeVtk(uuid, parent, name, type);
																		break;
									}
									case Resqml2Type::IJK_GRID:	{
																	uuidToVtkIjkGridRepresentation[parent]->createTreeVtk(uuid, parent, name, type);
																	break;
									}
									case Resqml2Type::UNSTRUC_GRID:	{
																		uuidToVtkUnstructuredGridRepresentation[parent]->createTreeVtk(uuid, parent, name, type);
																		break;
									}
									default:
										break;
									}
	}
	default:
		break;
	}

	}


	//----------------------------------------------------------------------------
	void VtkEpcDocument::createTreeVtkPartialRep(const std::string & uuid, const std::string & parent, VtkEpcDocument *vtkEpcDowumentWithCompleteRep)
	{
		uuidIsChildOf[uuid].type = Resqml2Type::PARTIAL;
		uuidIsChildOf[uuid].uuid = uuid;
		uuidToVtkPartialRepresentation[uuid] = new VtkPartialRepresentation(getFileName(), uuid, vtkEpcDowumentWithCompleteRep, epcPackage);
	}

	void VtkEpcDocument::deleteTreeVtkPartial(const std::string & uuid)
	{
		uuidIsChildOf.erase(uuid);
		delete uuidToVtkPartialRepresentation[uuid];
		uuidToVtkPartialRepresentation.erase(uuid);
	}

	//----------------------------------------------------------------------------
	void VtkEpcDocument::visualize(const std::string & uuid)
	{
		switch (uuidIsChildOf[uuid].type)
		{
		case Resqml2Type::GRID_2D:	{
										uuidToVtkGrid2DRepresentation[uuidIsChildOf[uuid].uuid]->visualize(uuid);
										break;
		}
		case Resqml2Type::POLYLINE_SET:	{
											auto object = epcPackage->getResqmlAbstractObjectByUuid(uuidIsChildOf[uuid].uuid);
											auto typeRepresentation = object->getXmlTag();
											if (typeRepresentation == "PolylineRepresentation") {
												uuidToVtkPolylineRepresentation[uuidIsChildOf[uuid].uuid]->visualize(uuid);
											}
											else  {
												uuidToVtkSetPatch[uuidIsChildOf[uuid].uuid]->visualize(uuid);
											}
											break;
		}
		case Resqml2Type::TRIANGULATED_SET:	{
												auto object = epcPackage->getResqmlAbstractObjectByUuid(uuidIsChildOf[uuid].uuid);
												auto typeRepresentation = object->getXmlTag();
												if (typeRepresentation == "TriangulatedRepresentation")  {
													uuidToVtkTriangulatedRepresentation[uuidIsChildOf[uuid].uuid]->visualize(uuid);
												}
												else  {
													uuidToVtkSetPatch[uuidIsChildOf[uuid].uuid]->visualize(uuid);
												}
												break;
		}
		case Resqml2Type::WELL_TRAJ:	{
											uuidToVtkWellboreTrajectoryRepresentation[uuidIsChildOf[uuid].uuid]->visualize(uuid);
											break;
		}
		case Resqml2Type::IJK_GRID:{
									   uuidToVtkIjkGridRepresentation[uuidIsChildOf[uuid].uuid]->visualize(uuid);
									   break;
		}
		case Resqml2Type::UNSTRUC_GRID:{
										   uuidToVtkUnstructuredGridRepresentation[uuidIsChildOf[uuid].uuid]->visualize(uuid);
										   break;
		}
		case Resqml2Type::PARTIAL:{
									  uuidToVtkPartialRepresentation[uuidIsChildOf[uuid].uuid]->visualize(uuid);
									  break;
		}
		default:
			break;
		}
		// attach representation to EpcDocument VtkMultiBlockDataSet
		std::string parent = uuidIsChildOf[uuid].uuid;
		if (std::find(attachUuids.begin(), attachUuids.end(), parent) == attachUuids.end())
		{
			this->detach();
			attachUuids.push_back(parent);
			this->attach();
		}
	}

	//----------------------------------------------------------------------------
	void VtkEpcDocument::remove(const std::string & uuid)
	{
		switch (uuidIsChildOf[uuid].type)
		{
		case Resqml2Type::GRID_2D:	{
										uuidToVtkGrid2DRepresentation[uuidIsChildOf[uuid].uuid]->remove(uuid);
										if (uuid == uuidIsChildOf[uuid].uuid){
											this->detach();
											attachUuids.erase(std::find(attachUuids.begin(), attachUuids.end(), uuid));
											this->attach();
										}
										break;
		}
		case Resqml2Type::POLYLINE_SET:	{
											auto object = epcPackage->getResqmlAbstractObjectByUuid(uuidIsChildOf[uuid].uuid);
											auto typeRepresentation = object->getXmlTag();
											if (typeRepresentation == "PolylineRepresentation") {
												uuidToVtkPolylineRepresentation[uuidIsChildOf[uuid].uuid]->remove(uuid);
												if (uuid == uuidIsChildOf[uuid].uuid){
													this->detach();
													attachUuids.erase(std::find(attachUuids.begin(), attachUuids.end(), uuid));
													this->attach();
												}
											}
											else  {
												uuidToVtkSetPatch[uuidIsChildOf[uuid].uuid]->remove(uuid);
												if (uuid == uuidIsChildOf[uuid].uuid){
													this->detach();
													attachUuids.erase(std::find(attachUuids.begin(), attachUuids.end(), uuid));
													this->attach();
												}
											}
											break;
		}
		case Resqml2Type::TRIANGULATED_SET:	{
												auto object = epcPackage->getResqmlAbstractObjectByUuid(uuidIsChildOf[uuid].uuid);
												auto typeRepresentation = object->getXmlTag();
												if (typeRepresentation == "TriangulatedRepresentation")  {
													uuidToVtkTriangulatedRepresentation[uuidIsChildOf[uuid].uuid]->remove(uuid);
													if (uuid == uuidIsChildOf[uuid].uuid){
														this->detach();
														attachUuids.erase(std::find(attachUuids.begin(), attachUuids.end(), uuid));
														this->attach();
													}
												}
												else  {
													uuidToVtkSetPatch[uuidIsChildOf[uuid].uuid]->remove(uuid);
													if (uuid == uuidIsChildOf[uuid].uuid){
														this->detach();
														attachUuids.erase(std::find(attachUuids.begin(), attachUuids.end(), uuid));
														this->attach();
													}
												}
												break;
		}
		case Resqml2Type::WELL_TRAJ:	{
											uuidToVtkWellboreTrajectoryRepresentation[uuidIsChildOf[uuid].uuid]->remove(uuid);
											if (uuid == uuidIsChildOf[uuid].uuid){
												this->detach();
												attachUuids.erase(std::find(attachUuids.begin(), attachUuids.end(), uuid));
												this->attach();
											}
											break;
		}
		case Resqml2Type::IJK_GRID:
		{
									  uuidToVtkIjkGridRepresentation[uuidIsChildOf[uuid].uuid]->remove(uuid);
									  if (uuidIsChildOf[uuid].uuid == uuid)
									  {
										  this->detach();
										  attachUuids.erase(std::find(attachUuids.begin(), attachUuids.end(), uuid));
										  this->attach();
									  }
									  break;
		}
		case Resqml2Type::UNSTRUC_GRID:
		{
										  uuidToVtkUnstructuredGridRepresentation[uuidIsChildOf[uuid].uuid]->remove(uuid);
										  if (uuidIsChildOf[uuid].uuid == uuid)
										  {
											  this->detach();
											  attachUuids.erase(std::find(attachUuids.begin(), attachUuids.end(), uuid));
											  this->attach();
										  }
										  break;
		}
		case Resqml2Type::PARTIAL:
		{
									 uuidToVtkPartialRepresentation[uuidIsChildOf[uuid].uuid]->remove(uuid);
									 break;
		}
		default:
			break;
		}
	}

	//----------------------------------------------------------------------------
	void VtkEpcDocument::attach()
	{
		for (unsigned int newBlockIndex = 0; newBlockIndex < attachUuids.size(); ++newBlockIndex)
		{
			std::string uuid = attachUuids[newBlockIndex];
			if (uuidIsChildOf[uuid].type == GRID_2D)
			{
				vtkOutput->SetBlock(newBlockIndex, uuidToVtkGrid2DRepresentation[uuid]->getOutput());
				vtkOutput->GetMetaData(newBlockIndex)->Set(vtkCompositeDataSet::NAME(), uuidToVtkGrid2DRepresentation[uuid]->getUuid().c_str());
			}
			if (uuidIsChildOf[uuid].type == POLYLINE_SET)
			{
				auto object = epcPackage->getResqmlAbstractObjectByUuid(uuid);
				auto typeRepresentation = object->getXmlTag();
				if (typeRepresentation == "PolylineRepresentation") {
					vtkOutput->SetBlock(newBlockIndex, uuidToVtkPolylineRepresentation[uuid]->getOutput());
					vtkOutput->GetMetaData(newBlockIndex)->Set(vtkCompositeDataSet::NAME(), uuidToVtkPolylineRepresentation[uuid]->getUuid().c_str());
				}
				else  {
					vtkOutput->SetBlock(newBlockIndex, uuidToVtkSetPatch[uuid]->getOutput());
					vtkOutput->GetMetaData(newBlockIndex)->Set(vtkCompositeDataSet::NAME(), uuidToVtkSetPatch[uuid]->getUuid().c_str());
				}
			}
			if (uuidIsChildOf[uuid].type == TRIANGULATED_SET)
			{
				auto object = epcPackage->getResqmlAbstractObjectByUuid(uuid);
				auto typeRepresentation = object->getXmlTag();
				if (typeRepresentation == "TriangulatedRepresentation")  {
					vtkOutput->SetBlock(newBlockIndex, uuidToVtkTriangulatedRepresentation[uuid]->getOutput());
					vtkOutput->GetMetaData(newBlockIndex)->Set(vtkCompositeDataSet::NAME(), uuidToVtkTriangulatedRepresentation[uuid]->getUuid().c_str());
				}
				else  {
					vtkOutput->SetBlock(newBlockIndex, uuidToVtkSetPatch[uuid]->getOutput());
					vtkOutput->GetMetaData(newBlockIndex)->Set(vtkCompositeDataSet::NAME(), uuidToVtkSetPatch[uuid]->getUuid().c_str());
				}
			}
			if (uuidIsChildOf[uuid].type == WELL_TRAJ)
			{
				vtkOutput->SetBlock(newBlockIndex, uuidToVtkWellboreTrajectoryRepresentation[uuid]->getOutput());
				vtkOutput->GetMetaData(newBlockIndex)->Set(vtkCompositeDataSet::NAME(), uuidToVtkWellboreTrajectoryRepresentation[uuid]->getUuid().c_str());
			}
			if (uuidIsChildOf[uuid].type == Resqml2Type::IJK_GRID)
			{
				vtkOutput->SetBlock(newBlockIndex, uuidToVtkIjkGridRepresentation[uuid]->getOutput());
				vtkOutput->GetMetaData(newBlockIndex)->Set(vtkCompositeDataSet::NAME(), uuidToVtkIjkGridRepresentation[uuid]->getUuid().c_str());
			}
			if (uuidIsChildOf[uuid].type == Resqml2Type::UNSTRUC_GRID)
			{
				vtkOutput->SetBlock(newBlockIndex, uuidToVtkUnstructuredGridRepresentation[uuid]->getOutput());
				vtkOutput->GetMetaData(newBlockIndex)->Set(vtkCompositeDataSet::NAME(), uuidToVtkUnstructuredGridRepresentation[uuid]->getUuid().c_str());
			}
		}
	}

	void VtkEpcDocument::addProperty(const std::string uuidProperty, vtkDataArray* dataProperty)
	{
		switch (uuidIsChildOf[uuidProperty].type)
		{
		case Resqml2Type::GRID_2D:	{
										uuidToVtkGrid2DRepresentation[uuidIsChildOf[uuidProperty].uuid]->addProperty(uuidProperty, dataProperty);
										break;
		}
		case Resqml2Type::POLYLINE_SET:	{
											auto object = epcPackage->getResqmlAbstractObjectByUuid(uuidIsChildOf[uuidProperty].uuid);
											auto typeRepresentation = object->getXmlTag();
											if (typeRepresentation == "PolylineRepresentation") {
												uuidToVtkPolylineRepresentation[uuidIsChildOf[uuidProperty].uuid]->addProperty(uuidProperty, dataProperty);
											}
											else  {
												uuidToVtkSetPatch[uuidIsChildOf[uuidProperty].uuid]->addProperty(uuidProperty, dataProperty);
											}
											break;
		}
		case Resqml2Type::TRIANGULATED_SET:	{
												auto object = epcPackage->getResqmlAbstractObjectByUuid(uuidIsChildOf[uuidProperty].uuid);
												auto typeRepresentation = object->getXmlTag();
												if (typeRepresentation == "TriangulatedRepresentation")  {
													uuidToVtkTriangulatedRepresentation[uuidIsChildOf[uuidProperty].uuid]->addProperty(uuidProperty, dataProperty);
												}
												else  {
													uuidToVtkSetPatch[uuidIsChildOf[uuidProperty].uuid]->addProperty(uuidProperty, dataProperty);
												}
												break;
		}
		case Resqml2Type::WELL_TRAJ:	{
											uuidToVtkWellboreTrajectoryRepresentation[uuidIsChildOf[uuidProperty].uuid]->addProperty(uuidProperty, dataProperty);
											break;
		}
		case Resqml2Type::IJK_GRID:
			uuidToVtkIjkGridRepresentation[uuidIsChildOf[uuidProperty].uuid]->addProperty(uuidProperty, dataProperty);
			break;
		case Resqml2Type::UNSTRUC_GRID:
			uuidToVtkUnstructuredGridRepresentation[uuidIsChildOf[uuidProperty].uuid]->addProperty(uuidProperty, dataProperty);
			break;
		default:
			break;
		}
		// attach representation to EpcDocument VtkMultiBlockDataSet
		std::string parent = uuidIsChildOf[uuidProperty].uuid;
		if (std::find(attachUuids.begin(), attachUuids.end(), parent) == attachUuids.end())
		{
			this->detach();
			attachUuids.push_back(parent);
			this->attach();
		}
	}

	long VtkEpcDocument::getAttachmentPropertyCount(const std::string & uuid, const FesppAttachmentProperty propertyUnit)
	{
		long result = 0;
		switch (uuidIsChildOf[uuid].type)
		{
		case Resqml2Type::IJK_GRID:
			result = uuidToVtkIjkGridRepresentation[uuidIsChildOf[uuid].uuid]->getAttachmentPropertyCount(uuid, propertyUnit);
			break;
		case Resqml2Type::UNSTRUC_GRID:
			result = uuidToVtkUnstructuredGridRepresentation[uuidIsChildOf[uuid].uuid]->getAttachmentPropertyCount(uuid, propertyUnit);
			break;
		default:
			break;
		}

		return result;
	}

	VtkAbstractObject::Resqml2Type VtkEpcDocument::getType(std::string uuid)
	{
		return uuidIsChildOf[uuid].type;
	}

	common::EpcDocument * VtkEpcDocument::getEpcDocument()
	{
		return epcPackage;
	}