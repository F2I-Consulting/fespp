#include "VtkEpcDocument.h"

// include system
#include <algorithm>
#include <sstream>

// include Vtk
#include <vtkInformation.h>

// include FESAPI
#include <common/EpcDocument.h>
#include "resqml2_0_1/PolylineSetRepresentation.h"
#include "resqml2_0_1/TriangulatedSetRepresentation.h"
#include "resqml2_0_1/Horizon.h"
#include "resqml2_0_1/TectonicBoundaryFeature.h"
#include "resqml2_0_1/HorizonInterpretation.h"
#include "resqml2_0_1/FaultInterpretation.h"
#include "resqml2_0_1/PointSetRepresentation.h"
#include "resqml2_0_1/Grid2dRepresentation.h"
#include "common/AbstractObject.h"
#include "resqml2_0_1/SubRepresentation.h"
#include "resqml2_0_1/AbstractIjkGridRepresentation.h"
#include "resqml2/AbstractValuesProperty.h"
#include "resqml2_0_1/UnstructuredGridRepresentation.h"
#include "resqml2_0_1/WellboreTrajectoryRepresentation.h"
#include "resqml2_0_1/PropertyKindMapper.h"
#include "resqml2_0_1/SubRepresentation.h"
#include "resqml2_0_1/TimeSeries.h"


// include Vtk for Plugin
#include "VtkEpcDocumentSet.h"
#include "VtkGrid2DRepresentation.h"
#include "VtkPolylineRepresentation.h"
#include "VtkTriangulatedRepresentation.h"
#include "VtkWellboreTrajectoryRepresentation.h"
#include "VtkUnstructuredGridRepresentation.h"
#include "VtkIjkGridRepresentation.h"
#include "VtkPartialRepresentation.h"
#include "VtkSetPatch.h"
#include "VtkEpcCommon.h"

//----------------------------------------------------------------------------
VtkEpcDocument::VtkEpcDocument(const std::string & fileName, const int & idProc, const int & maxProc, VtkEpcDocumentSet* epcDocSet) :
VtkResqml2MultiBlockDataSet(fileName, fileName, fileName, "", idProc, maxProc), epcSet(epcDocSet)
{
	common::EpcDocument *pck = nullptr;
	try
	{
		pck = new common::EpcDocument(fileName, common::EpcDocument::READ_ONLY);
	}
	catch (const std::exception & e)
	{
		cout << "EXCEPTION in fesapi when reading file: " << fileName << " : " << e.what();
	}
	epcPackage = pck;

	std::string result = "";
	try
	{
		result = pck->deserialize();
	}
	catch (const std::exception & e)
	{
		cout << "EXCEPTION in fesapi when deserialize file: " << fileName << " : " << e.what();
	}
	if (result.empty())
	{
		//**************
		// polylines
		std::vector<resqml2_0_1::PolylineSetRepresentation*> polylines;
		try
		{
			polylines = pck->getFaultPolylineSetRepSet();
		}
		catch  (const std::exception & e)
		{
			cout << "EXCEPTION in fesapi when call getFaultPolylineSetRepSet with file: " << fileName << " : " << e.what();
		}
		for (auto iter = 0; iter < polylines.size(); ++iter)
		{
			auto interpretation = polylines[iter]->getInterpretation();
			uuidIsChildOf[polylines[iter]->getUuid()] = new VtkEpcCommon();
			std::string uuidParent;
			if (interpretation)
				uuidParent = interpretation->getUuid();
			else
				uuidParent = fileName;
			if (polylines[iter]->isPartial())
			{
				auto vtkEpcSrc = epcSet->getVtkEpcDocument(polylines[iter]->getUuid());
				if (vtkEpcSrc)
				{
					createTreeVtkPartialRep(polylines[iter]->getUuid(), vtkEpcSrc);
					uuidIsChildOf[polylines[iter]->getUuid()]->setParentType( VtkEpcCommon::POLYLINE_SET);
				}
			}
			else
			{
				createTreeVtk(polylines[iter]->getUuid(), uuidParent, polylines[iter]->getTitle().c_str(), VtkEpcCommon::POLYLINE_SET);
			}
		}

		try
		{
			polylines = pck->getHorizonPolylineSetRepSet();
		}
		catch  (const std::exception & e)
		{
			cout << "EXCEPTION in fesapi when call getHorizonPolylineSetRepSet with file: " << fileName << " : " << e.what();
		}
		for (auto iter = 0; iter < polylines.size(); ++iter)
		{
			auto interpretation = polylines[iter]->getInterpretation();
			uuidIsChildOf[polylines[iter]->getUuid()] = new VtkEpcCommon();
			std::string uuidParent;
			if (interpretation)
				uuidParent = interpretation->getUuid();
			else
				uuidParent = fileName;
			if (polylines[iter]->isPartial())
			{
				auto vtkEpcSrc = epcSet->getVtkEpcDocument(polylines[iter]->getUuid());
				if (vtkEpcSrc)
				{
					createTreeVtkPartialRep(polylines[iter]->getUuid(), vtkEpcSrc);
					uuidIsChildOf[polylines[iter]->getUuid()]->setParentType( VtkEpcCommon::POLYLINE_SET);
				}
			}
			else
			{
				createTreeVtk(polylines[iter]->getUuid(), uuidParent, polylines[iter]->getTitle().c_str(), VtkEpcCommon::POLYLINE_SET);
			}
			//property
			auto valuesPropertySet = polylines[iter]->getValuesPropertySet();
			for (unsigned int i = 0; i < valuesPropertySet.size(); ++i)
			{
				uuidIsChildOf[valuesPropertySet[i]->getUuid()] = new VtkEpcCommon();
				createTreeVtk(valuesPropertySet[i]->getUuid(), polylines[iter]->getUuid(), valuesPropertySet[i]->getTitle().c_str(), VtkEpcCommon::PROPERTY);
			}
		}
		//**************

		//**************
		// triangulated
		std::vector<resqml2_0_1::TriangulatedSetRepresentation*> triangulated;
		try
		{
			triangulated = pck->getAllTriangulatedSetRepSet();
		}
		catch  (const std::exception & e)
		{
			cout << "EXCEPTION in fesapi when call getAllTriangulatedSetRepSet with file: " << fileName << " : " << e.what();
		}
		for (auto iter = 0; iter < triangulated.size(); ++iter)
		{
			auto interpretation = triangulated[iter]->getInterpretation();
			uuidIsChildOf[triangulated[iter]->getUuid()] = new VtkEpcCommon();
			std::string uuidParent;
			if (interpretation)
				uuidParent = interpretation->getUuid();
			else
				uuidParent = fileName;
			if (triangulated[iter]->isPartial())
			{
				auto vtkEpcSrc = epcSet->getVtkEpcDocument(triangulated[iter]->getUuid());
				if (vtkEpcSrc)
				{
					createTreeVtkPartialRep(triangulated[iter]->getUuid(), vtkEpcSrc);
					uuidIsChildOf[triangulated[iter]->getUuid()]->setParentType( VtkEpcCommon::TRIANGULATED_SET);
				}
			}
			else
			{
				createTreeVtk(triangulated[iter]->getUuid(), uuidParent, triangulated[iter]->getTitle().c_str(), VtkEpcCommon::TRIANGULATED_SET);
			}
			//property
			auto valuesPropertySet = triangulated[iter]->getValuesPropertySet();
			for (unsigned int i = 0; i < valuesPropertySet.size(); ++i)
			{
				uuidIsChildOf[valuesPropertySet[i]->getUuid()] = new VtkEpcCommon();
				createTreeVtk(valuesPropertySet[i]->getUuid(), triangulated[iter]->getUuid(), valuesPropertySet[i]->getTitle().c_str(), VtkEpcCommon::PROPERTY);
			}
		}
		//**************

		//**************
		// grid2D
		std::vector<resqml2_0_1::Grid2dRepresentation*> grid2D;
		try
		{
			grid2D = pck->getHorizonGrid2dRepSet();
		}
		catch  (const std::exception & e)
		{
			cout << "EXCEPTION in fesapi when call getHorizonGrid2dRepSet with file: " << fileName << " : " << e.what();
		}
		for (auto iter = 0; iter < grid2D.size(); ++iter)
		{
			auto interpretation = grid2D[iter]->getInterpretation();
			uuidIsChildOf[grid2D[iter]->getUuid()] = new VtkEpcCommon();
			std::string uuidParent;
			if (interpretation)
				uuidParent = interpretation->getUuid();
			else
				uuidParent = fileName;
			if (grid2D[iter]->isPartial())
			{
				auto vtkEpcSrc = epcSet->getVtkEpcDocument(grid2D[iter]->getUuid());
				if (vtkEpcSrc)
				{
					createTreeVtkPartialRep(grid2D[iter]->getUuid(), vtkEpcSrc);
					uuidIsChildOf[grid2D[iter]->getUuid()]->setParentType( VtkEpcCommon::GRID_2D);
				}
			}
			else
			{
				createTreeVtk(grid2D[iter]->getUuid(), uuidParent, grid2D[iter]->getTitle().c_str(), VtkEpcCommon::GRID_2D);
			}
			//property
			auto valuesPropertySet = grid2D[iter]->getValuesPropertySet();
			for (unsigned int i = 0; i < valuesPropertySet.size(); ++i)
			{
				uuidIsChildOf[valuesPropertySet[i]->getUuid()] = new VtkEpcCommon();
				createTreeVtk(valuesPropertySet[i]->getUuid(), grid2D[iter]->getUuid(), valuesPropertySet[i]->getTitle().c_str(), VtkEpcCommon::PROPERTY);
			}
		}
		//**************

		//**************
		// ijkGrid
		std::vector<resqml2_0_1::AbstractIjkGridRepresentation*> ijkGrid;
		try
		{
			ijkGrid = pck->getIjkGridRepresentationSet();
		}
		catch  (const std::exception & e)
		{
			cout << "EXCEPTION in fesapi when call getIjkGridRepresentationSet with file: " << fileName << " : " << e.what();
		}
		for (auto iter = 0; iter < ijkGrid.size(); ++iter)
		{
			auto interpretation = ijkGrid[iter]->getInterpretation();
			std::string uuidParent;
			uuidIsChildOf[ijkGrid[iter]->getUuid()] = new VtkEpcCommon();
			if (interpretation)
				uuidParent = interpretation->getUuid();
			else
				uuidParent = fileName;
			if (ijkGrid[iter]->isPartial())
			{
				auto vtkEpcSrc = epcSet->getVtkEpcDocument(ijkGrid[iter]->getUuid());
				if (vtkEpcSrc)
				{
					createTreeVtkPartialRep(ijkGrid[iter]->getUuid(), vtkEpcSrc);
					uuidIsChildOf[ijkGrid[iter]->getUuid()]->setParentType( VtkEpcCommon::IJK_GRID);
				}
			}
			else
			{
				createTreeVtk(ijkGrid[iter]->getUuid(), uuidParent, ijkGrid[iter]->getTitle().c_str(), VtkEpcCommon::IJK_GRID);
			}
			//property
			auto valuesPropertySet = ijkGrid[iter]->getValuesPropertySet();
			for (unsigned int i = 0; i < valuesPropertySet.size(); ++i)
			{
				uuidIsChildOf[valuesPropertySet[i]->getUuid()] = new VtkEpcCommon();
				createTreeVtk(valuesPropertySet[i]->getUuid(), ijkGrid[iter]->getUuid(), valuesPropertySet[i]->getTitle().c_str(), VtkEpcCommon::PROPERTY);
			}
		}
		//**************

		//**************
		// unstructuredGrid
		std::vector<resqml2_0_1::UnstructuredGridRepresentation*> unstructuredGrid;
		try
		{
			unstructuredGrid = pck->getUnstructuredGridRepresentationSet();
		}
		catch  (const std::exception & e)
		{
			cout << "EXCEPTION in fesapi when call getUnstructuredGridRepresentationSet with file: " << fileName << " : " << e.what();
		}
		for (auto iter = 0; iter < unstructuredGrid.size(); ++iter)
		{
			auto interpretation = unstructuredGrid[iter]->getInterpretation();
			std::string uuidParent;
			uuidIsChildOf[unstructuredGrid[iter]->getUuid()] = new VtkEpcCommon();
			if (interpretation)
				uuidParent = interpretation->getUuid();
			else
				uuidParent = fileName;
			if (unstructuredGrid[iter]->isPartial())
			{
				auto vtkEpcSrc = epcSet->getVtkEpcDocument(unstructuredGrid[iter]->getUuid());
				if (vtkEpcSrc)
				{
					createTreeVtkPartialRep(unstructuredGrid[iter]->getUuid(), vtkEpcSrc);
					uuidIsChildOf[unstructuredGrid[iter]->getUuid()]->setParentType( VtkEpcCommon::UNSTRUC_GRID);
				}
			}
			else
			{
				createTreeVtk(unstructuredGrid[iter]->getUuid(), uuidParent, unstructuredGrid[iter]->getTitle().c_str(), VtkEpcCommon::UNSTRUC_GRID);
			}
			//property
			auto valuesPropertySet = unstructuredGrid[iter]->getValuesPropertySet();
			for (unsigned int i = 0; i < valuesPropertySet.size(); ++i)
			{
				uuidIsChildOf[valuesPropertySet[i]->getUuid()] = new VtkEpcCommon();
				createTreeVtk(valuesPropertySet[i]->getUuid(), unstructuredGrid[iter]->getUuid(), valuesPropertySet[i]->getTitle().c_str(), VtkEpcCommon::PROPERTY);
			}
		}
		//**************

		//**************
		// WellboreTrajectory
		std::vector<resqml2_0_1::WellboreTrajectoryRepresentation*> WellboreTrajectory;
		try
		{
			WellboreTrajectory = pck->getWellboreTrajectoryRepresentationSet();
		}
		catch  (const std::exception & e)
		{
			cout << "EXCEPTION in fesapi when call getWellboreTrajectoryRepresentationSet with file: " << fileName << " : " << e.what();
		}
		for (auto iter = 0; iter < WellboreTrajectory.size(); ++iter)
		{
			auto interpretation = WellboreTrajectory[iter]->getInterpretation();
			std::string uuidParent;
			uuidIsChildOf[WellboreTrajectory[iter]->getUuid()] = new VtkEpcCommon();
			if (interpretation)
				uuidParent = interpretation->getUuid();
			else
				uuidParent = fileName;
			if (WellboreTrajectory[iter]->isPartial())
			{
				auto vtkEpcSrc = epcSet->getVtkEpcDocument(WellboreTrajectory[iter]->getUuid());
				if (vtkEpcSrc)
				{
					createTreeVtkPartialRep(WellboreTrajectory[iter]->getUuid(), vtkEpcSrc);
					uuidIsChildOf[WellboreTrajectory[iter]->getUuid()]->setParentType( VtkEpcCommon::WELL_TRAJ);
				}
			}
			else
			{
				createTreeVtk(WellboreTrajectory[iter]->getUuid(), uuidParent, WellboreTrajectory[iter]->getTitle().c_str(), VtkEpcCommon::WELL_TRAJ);
			}
			//property
			auto valuesPropertySet = WellboreTrajectory[iter]->getValuesPropertySet();
			for (unsigned int i = 0; i < valuesPropertySet.size(); ++i)
			{
				uuidIsChildOf[valuesPropertySet[i]->getUuid()] = new VtkEpcCommon();
				createTreeVtk(valuesPropertySet[i]->getUuid(), WellboreTrajectory[iter]->getUuid(), valuesPropertySet[i]->getTitle().c_str(), VtkEpcCommon::PROPERTY);
			}
		}
		//**************


		//**************
		// subRepresentation
		std::vector<resqml2::SubRepresentation*> subRepresentation;
		try
		{
			subRepresentation = pck->getSubRepresentationSet();
		}
		catch  (const std::exception & e)
		{
			cout << "EXCEPTION in fesapi when call getSubRepresentationSet with file: " << fileName << " : " << e.what();
		}
		for (auto iter = 0; iter < subRepresentation.size(); ++iter)
		{
			uuidIsChildOf[subRepresentation[iter]->getUuid()] = new VtkEpcCommon();
			if (subRepresentation[iter]->isPartial())
			{
				auto vtkEpcSrc = epcSet->getVtkEpcDocument(subRepresentation[iter]->getUuid());
				if (vtkEpcSrc)
				{
					createTreeVtkPartialRep(subRepresentation[iter]->getUuid(), vtkEpcSrc);
					uuidIsChildOf[subRepresentation[iter]->getUuid()]->setParentType( VtkEpcCommon::SUB_REP);
				}
			}
			else
			{
				auto uuidParent = subRepresentation[iter]->getSupportingRepresentationUuid(0);
				createTreeVtk(subRepresentation[iter]->getUuid(), uuidParent, subRepresentation[iter]->getTitle().c_str(), VtkEpcCommon::SUB_REP);
			}
			//property
			auto valuesPropertySet = subRepresentation[iter]->getValuesPropertySet();
			for (unsigned int i = 0; i < valuesPropertySet.size(); ++i)
			{
				uuidIsChildOf[valuesPropertySet[i]->getUuid()] = new VtkEpcCommon();
				createTreeVtk(valuesPropertySet[i]->getUuid(), subRepresentation[iter]->getUuid(), valuesPropertySet[i]->getTitle().c_str(), VtkEpcCommon::PROPERTY);
			}
		}
		//**************


		//**************
		// TimeSeries
		std::vector<resqml2::TimeSeries*> timeSeries;
		try
		{
			timeSeries = pck->getTimeSeriesSet();
		}
		catch  (const std::exception & e)
		{
			cout << "EXCEPTION in fesapi when call getTimeSeriesSet with file: " << fileName << " : " << e.what();
		}

		for (auto iter = 0; iter < timeSeries.size(); ++iter)
		{
			auto propSeries = timeSeries[iter]->getPropertySet();
			uuidIsChildOf[propSeries[iter]->getUuid()] = new VtkEpcCommon();
			for (auto i = 0; i < propSeries.size(); ++i)
			{
				uuidIsChildOf[propSeries[i]->getUuid()]->setType( VtkEpcCommon::TIME_SERIES);
				uuidIsChildOf[propSeries[i]->getUuid()]->setTimeIndex( propSeries[i]->getTimeIndex());
				uuidIsChildOf[propSeries[i]->getUuid()]->setTimestamp( propSeries[i]->getTimestamp());

				/*
				if (propSeries[i]->isAssociatedToOneStandardEnergisticsPropertyKind())
				{
					auto propKind = propSeries[i]->getEnergisticsPropertyKind();
				}
				else
				{
					auto propkindUuid = propSeries[i]->getLocalPropertyKindUuid();
					cout << "name : " << propSeries[i]->getTitle() << " - " <<propkindUuid << "\n";
				}
				*/

			}
		}

		//**************
		for (auto &iter : uuidRep)
		{
			treeView.push_back(uuidIsChildOf[iter]);
		}

	}
	else
	{
		try
		{
			pck->close();
		}
		catch (const std::exception & e)
		{
			cout << "EXCEPTION in fesapi when closing file " << fileName << " : " << e.what();
		}
	}
}

//----------------------------------------------------------------------------
VtkEpcDocument::~VtkEpcDocument()
{
	// delete uuidToVtkGrid2DRepresentation
#if _MSC_VER < 1600
	for (std::tr1::unordered_map< std::string, VtkGrid2DRepresentation* >::const_iterator it = uuidToVtkGrid2DRepresentation.begin(); it != uuidToVtkGrid2DRepresentation.end(); ++it)
#else
		for (std::unordered_map< std::string, VtkGrid2DRepresentation* >::const_iterator it = uuidToVtkGrid2DRepresentation.begin(); it != uuidToVtkGrid2DRepresentation.end(); ++it)
#endif
		{
			delete it->second;
		}
	uuidToVtkGrid2DRepresentation.clear();

	// delete uuidToVtkPolylineRepresentation
#if _MSC_VER < 1600
	for (std::tr1::unordered_map< std::string, VtkPolylineRepresentation* >::const_iterator it = uuidToVtkPolylineRepresentation.begin(); it != uuidToVtkPolylineRepresentation.end(); ++it)
#else
		for (std::unordered_map< std::string, VtkPolylineRepresentation* >::const_iterator it = uuidToVtkPolylineRepresentation.begin(); it != uuidToVtkPolylineRepresentation.end(); ++it)
#endif
		{
			delete it->second;
		}
	uuidToVtkPolylineRepresentation.clear();

	// delete uuidToVtkTriangulatedRepresentation
#if _MSC_VER < 1600
	for (std::tr1::unordered_map< std::string, VtkTriangulatedRepresentation* >::const_iterator it = uuidToVtkTriangulatedRepresentation.begin(); it != uuidToVtkTriangulatedRepresentation.end(); ++it)
#else
		for (std::unordered_map< std::string, VtkTriangulatedRepresentation* >::const_iterator it = uuidToVtkTriangulatedRepresentation.begin(); it != uuidToVtkTriangulatedRepresentation.end(); ++it)
#endif
		{
			delete it->second;
		}
	uuidToVtkTriangulatedRepresentation.clear();

	// delete uuidToVtkSetPatch
#if _MSC_VER < 1600
	for (std::tr1::unordered_map< std::string, VtkSetPatch* >::const_iterator it = uuidToVtkSetPatch.begin(); it != uuidToVtkSetPatch.end(); ++it)
#else
		for (std::unordered_map< std::string, VtkSetPatch* >::const_iterator it = uuidToVtkSetPatch.begin(); it != uuidToVtkSetPatch.end(); ++it)
#endif
		{
			delete it->second;
		}
	uuidToVtkSetPatch.clear();

	// delete uuidToVtkWellboreTrajectoryRepresentation
#if _MSC_VER < 1600
	for (std::tr1::unordered_map< std::string, VtkWellboreTrajectoryRepresentation* >::const_iterator it = uuidToVtkWellboreTrajectoryRepresentation.begin(); it != uuidToVtkWellboreTrajectoryRepresentation.end(); ++it)
#else
		for (std::unordered_map< std::string, VtkWellboreTrajectoryRepresentation* >::const_iterator it = uuidToVtkWellboreTrajectoryRepresentation.begin(); it != uuidToVtkWellboreTrajectoryRepresentation.end(); ++it)
#endif
		{
			delete it->second;
		}
	uuidToVtkWellboreTrajectoryRepresentation.clear();

	// delete uuidToVtkIjkGridRepresentation
#if _MSC_VER < 1600
	for (std::tr1::unordered_map< std::string, VtkIjkGridRepresentation* >::const_iterator it = uuidToVtkIjkGridRepresentation.begin(); it != uuidToVtkIjkGridRepresentation.end(); ++it)
#else
		for (std::unordered_map< std::string, VtkIjkGridRepresentation* >::const_iterator it = uuidToVtkIjkGridRepresentation.begin(); it != uuidToVtkIjkGridRepresentation.end(); ++it)
#endif
		{
			delete it->second;
		}
	uuidToVtkIjkGridRepresentation.clear();

	// delete uuidToVtkUnstructuredGridRepresentation
#if _MSC_VER < 1600
	for (std::tr1::unordered_map< std::string, VtkUnstructuredGridRepresentation* >::const_iterator it = uuidToVtkUnstructuredGridRepresentation.begin(); it != uuidToVtkUnstructuredGridRepresentation.end(); ++it)
#else
		for (std::unordered_map< std::string, VtkUnstructuredGridRepresentation* >::const_iterator it = uuidToVtkUnstructuredGridRepresentation.begin(); it != uuidToVtkUnstructuredGridRepresentation.end(); ++it)
#endif
		{
			delete it->second;
		}
	uuidToVtkUnstructuredGridRepresentation.clear();

	// delete uuidToVtkPartialRepresentation
#if _MSC_VER < 1600
	for (std::tr1::unordered_map< std::string, VtkPartialRepresentation* >::const_iterator it = uuidToVtkPartialRepresentation.begin(); it != uuidToVtkPartialRepresentation.end(); ++it)
#else
		for (std::unordered_map< std::string, VtkPartialRepresentation* >::const_iterator it = uuidToVtkPartialRepresentation.begin(); it != uuidToVtkPartialRepresentation.end(); ++it)
#endif
		{
			delete it->second;
		}
	uuidToVtkPartialRepresentation.clear();

	uuidPartialRep.clear();
	uuidRep.clear();

	epcSet = nullptr;

	epcPackage->close();
}

//----------------------------------------------------------------------------
void VtkEpcDocument::createTreeVtk(const std::string & uuid, const std::string & parent, const std::string & name, const VtkEpcCommon::Resqml2Type & type)
{
	uuidIsChildOf[uuid]->setType( type);
	uuidIsChildOf[uuid]->setUuid(uuid);
	uuidIsChildOf[uuid]->setParent(parent);
	uuidIsChildOf[uuid]->setName(name);
	uuidIsChildOf[uuid]->setTimeIndex(-1);
	uuidIsChildOf[uuid]->setTimestamp(0);

	if(uuidIsChildOf[parent])
	{
		uuidIsChildOf[uuid]->setParentType( uuidIsChildOf[parent]->getType());
	}
	else
	{
		uuidIsChildOf[uuid]->setParentType( VtkEpcCommon::INTERPRETATION);
	}

	switch (type)
	{
	case VtkEpcCommon::Resqml2Type::GRID_2D:
	{
		addGrid2DTreeVtk(uuid, parent, name);
		break;
	}
	case VtkEpcCommon::Resqml2Type::POLYLINE_SET:
	{
		addPolylineSetTreeVtk(uuid, parent, name);
		break;
	}
	case VtkEpcCommon::Resqml2Type::TRIANGULATED_SET:
	{
		addTriangulatedSetTreeVtk(uuid, parent, name);
		break;
	}
	case VtkEpcCommon::Resqml2Type::WELL_TRAJ:
	{
		addWellTrajTreeVtk(uuid, parent, name);
		break;
	}
	case VtkEpcCommon::Resqml2Type::IJK_GRID:
	{
		addIjkGridTreeVtk(uuid, parent, name);
		break;
	}
	case VtkEpcCommon::Resqml2Type::UNSTRUC_GRID:
	{
		addUnstrucGridTreeVtk(uuid, parent, name);
		break;
	}
	case VtkEpcCommon::Resqml2Type::SUB_REP:
	{
		addSubRepTreeVtk(uuid, parent, name);
		break;
	}
	case VtkEpcCommon::Resqml2Type::PROPERTY:
	{
		addPropertyTreeVtk(uuid, parent, name);
	}
	default:
		break;
	}
	uuidRep.push_back(uuid);
}

//----------------------------------------------------------------------------
void VtkEpcDocument::addGrid2DTreeVtk(const std::string & uuid, const std::string & parent, const std::string & name)
{
	uuidToVtkGrid2DRepresentation[uuid] = new VtkGrid2DRepresentation(getFileName(), name, uuid, parent, epcPackage, nullptr);
}

//----------------------------------------------------------------------------
void VtkEpcDocument::addPolylineSetTreeVtk(const std::string & uuid, const std::string & parent, const std::string & name)
{
	auto object = epcPackage->getResqmlAbstractObjectByUuid(uuid);
	auto typeRepresentation = object->getXmlTag();
	if (typeRepresentation == "PolylineRepresentation") 
	{
		uuidToVtkPolylineRepresentation[uuid] = new VtkPolylineRepresentation(getFileName(), name, uuid, parent, 0, epcPackage, nullptr);
	}
	else  
	{
		uuidToVtkSetPatch[uuid] = new VtkSetPatch(getFileName(), name, uuid, parent, epcPackage, getIdProc(), getMaxProc());
	}
}

//----------------------------------------------------------------------------
void VtkEpcDocument::addTriangulatedSetTreeVtk(const std::string & uuid, const std::string & parent, const std::string & name)
{
	auto object = epcPackage->getResqmlAbstractObjectByUuid(uuid);
	auto typeRepresentation = object->getXmlTag();
	if (typeRepresentation == "TriangulatedRepresentation")  
	{
		uuidToVtkTriangulatedRepresentation[uuid] = new VtkTriangulatedRepresentation(getFileName(), name, uuid, parent, 0, epcPackage, nullptr);
	}
	else  
	{
		uuidToVtkSetPatch[uuid] = new VtkSetPatch(getFileName(), name, uuid, parent, epcPackage, getIdProc(), getMaxProc());
	}
}

//----------------------------------------------------------------------------
void VtkEpcDocument::addWellTrajTreeVtk(const std::string & uuid, const std::string & parent, const std::string & name)
{
	uuidToVtkWellboreTrajectoryRepresentation[uuid] = new VtkWellboreTrajectoryRepresentation(getFileName(), name, uuid, parent, epcPackage, nullptr);
}

//----------------------------------------------------------------------------
void VtkEpcDocument::addIjkGridTreeVtk(const std::string & uuid, const std::string & parent, const std::string & name)
{
	uuidToVtkIjkGridRepresentation[uuid] = new VtkIjkGridRepresentation(getFileName(), name, uuid, parent, epcPackage, nullptr, getIdProc(), getMaxProc());
}

//----------------------------------------------------------------------------
void VtkEpcDocument::addUnstrucGridTreeVtk(const std::string & uuid, const std::string & parent, const std::string & name)
{
	uuidToVtkUnstructuredGridRepresentation[uuid] = new VtkUnstructuredGridRepresentation(getFileName(), name, uuid, parent, epcPackage, nullptr);
}

//----------------------------------------------------------------------------
void VtkEpcDocument::addSubRepTreeVtk(const std::string & uuid, const std::string & parent, const std::string & name)
{
	if (uuidIsChildOf[uuid]->getParentType() == VtkEpcCommon::IJK_GRID)
	{
		uuidToVtkIjkGridRepresentation[uuid] = new VtkIjkGridRepresentation(getFileName(), name, uuid, parent, epcPackage, epcPackage);
	}
	if (uuidIsChildOf[uuid]->getParentType() == VtkEpcCommon::UNSTRUC_GRID)
	{
		uuidToVtkUnstructuredGridRepresentation[uuid] = new VtkUnstructuredGridRepresentation(getFileName(), name, uuid, parent, epcPackage, epcPackage);
	}
	if (uuidIsChildOf[uuid]->getParentType() == VtkEpcCommon::PARTIAL)
	{
		auto parentUuidType = uuidIsChildOf[parent]->getParentType();
		auto pckEPCsrc = uuidToVtkPartialRepresentation[parent]->getEpcSource();

		switch (parentUuidType)
		{
		case VtkEpcCommon::GRID_2D:
		{
			uuidToVtkGrid2DRepresentation[uuid] = new VtkGrid2DRepresentation(getFileName(), name, uuid, parent, pckEPCsrc, epcPackage);
			break;
		}
		case VtkEpcCommon::WELL_TRAJ:
		{
			uuidToVtkWellboreTrajectoryRepresentation[uuid] = new VtkWellboreTrajectoryRepresentation(getFileName(), name, uuid, parent, pckEPCsrc, epcPackage);
			break;
		}
		case VtkEpcCommon::IJK_GRID:
		{
			uuidToVtkIjkGridRepresentation[uuid] = new VtkIjkGridRepresentation(getFileName(), name, uuid, parent, pckEPCsrc, epcPackage);
			break;
		}
		case VtkEpcCommon::UNSTRUC_GRID:
		{
			uuidToVtkUnstructuredGridRepresentation[uuid] = new VtkUnstructuredGridRepresentation(getFileName(), name, uuid, parent, pckEPCsrc, epcPackage);
			break;
		}
		}
	}
}

//----------------------------------------------------------------------------
void VtkEpcDocument::addPropertyTreeVtk(const std::string & uuid, const std::string & parent, const std::string & name)
{
	switch (uuidIsChildOf[parent]->getType())
	{
	case VtkEpcCommon::GRID_2D:
	{
		uuidToVtkGrid2DRepresentation[parent]->createTreeVtk(uuid, parent, name, uuidIsChildOf[uuid]->getType());
		break;
	}
	case VtkEpcCommon::POLYLINE_SET:
	{
		auto object = epcPackage->getResqmlAbstractObjectByUuid(parent);
		auto typeRepresentation = object->getXmlTag();
		if (typeRepresentation == "PolylineRepresentation")
		{
			uuidToVtkPolylineRepresentation[parent]->createTreeVtk(uuid, parent, name, uuidIsChildOf[uuid]->getType());
		}
		else
		{
			uuidToVtkSetPatch[parent]->createTreeVtk(uuid, parent, name, uuidIsChildOf[uuid]->getType());
		}
		break;
	}
	case VtkEpcCommon::TRIANGULATED_SET:
	{
		auto object = epcPackage->getResqmlAbstractObjectByUuid(uuid);
		auto typeRepresentation = object->getXmlTag();
		if (typeRepresentation == "TriangulatedRepresentation")
		{
			uuidToVtkTriangulatedRepresentation[parent]->createTreeVtk(uuid, parent, name, uuidIsChildOf[uuid]->getType());
		}
		else
		{
			uuidToVtkSetPatch[parent]->createTreeVtk(uuid, parent, name, uuidIsChildOf[uuid]->getType());
		}
		break;
	}
	case VtkEpcCommon::WELL_TRAJ:
	{
		uuidToVtkWellboreTrajectoryRepresentation[parent]->createTreeVtk(uuid, parent, name, uuidIsChildOf[uuid]->getType());
		break;
	}
	case VtkEpcCommon::IJK_GRID:
	{
		uuidToVtkIjkGridRepresentation[parent]->createTreeVtk(uuid, parent, name, uuidIsChildOf[uuid]->getType());
		break;
	}
	case VtkEpcCommon::UNSTRUC_GRID:
	{
		uuidToVtkUnstructuredGridRepresentation[parent]->createTreeVtk(uuid, parent, name, uuidIsChildOf[uuid]->getType());
		break;
	}
	case VtkEpcCommon::SUB_REP:
	{
		if (uuidIsChildOf[parent]->getParentType() == VtkEpcCommon::IJK_GRID)
		{
			uuidToVtkIjkGridRepresentation[parent]->createTreeVtk(uuid, parent, name, uuidIsChildOf[uuid]->getType());
		}
		if (uuidIsChildOf[parent]->getParentType() == VtkEpcCommon::UNSTRUC_GRID)
		{
			uuidToVtkUnstructuredGridRepresentation[parent]->createTreeVtk(uuid, parent, name, uuidIsChildOf[uuid]->getType());
		}
		if (uuidIsChildOf[parent]->getParentType() == VtkEpcCommon::PARTIAL)
		{
			auto uuidPartial = uuidIsChildOf[parent]->getParent();

			switch (uuidIsChildOf[uuidPartial]->getParentType())
			{
			case VtkEpcCommon::GRID_2D:
			{
				uuidToVtkGrid2DRepresentation[parent]->createTreeVtk(uuid, parent, name, uuidIsChildOf[uuid]->getType());
				break;
			}
			case VtkEpcCommon::WELL_TRAJ:
			{
				uuidToVtkWellboreTrajectoryRepresentation[parent]->createTreeVtk(uuid, parent, name, uuidIsChildOf[uuid]->getType());
				break;
			}
			case VtkEpcCommon::IJK_GRID:
			{
				uuidToVtkIjkGridRepresentation[parent]->createTreeVtk(uuid, parent, name, uuidIsChildOf[uuid]->getType());
				break;
			}
			case VtkEpcCommon::UNSTRUC_GRID:
			{
				uuidToVtkUnstructuredGridRepresentation[parent]->createTreeVtk(uuid, parent, name, uuidIsChildOf[uuid]->getType());
				break;
			}
			}
		}


		break;
	}
	case VtkEpcCommon::PARTIAL:
	{
		uuidToVtkPartialRepresentation[uuidIsChildOf[parent]->getUuid()]->createTreeVtk(uuid, parent, name, uuidIsChildOf[uuid]->getType());
		break;
	}

	default:
	{
		cout << " parent type unknown " << uuidIsChildOf[uuid]->getParentType() << "\n";
		break;
	}
	}

}



//----------------------------------------------------------------------------
void VtkEpcDocument::createTreeVtkPartialRep(const std::string & uuid, VtkEpcDocument *vtkEpcDowumentWithCompleteRep)
{
	uuidIsChildOf[uuid]->setType( VtkEpcCommon::PARTIAL);
	uuidIsChildOf[uuid]->setUuid(uuid);
	uuidToVtkPartialRepresentation[uuid] = new VtkPartialRepresentation(getFileName(), uuid, vtkEpcDowumentWithCompleteRep, epcPackage);
}

/* A DELETER
 * //----------------------------------------------------------------------------
void VtkEpcDocument::deleteTreeVtkPartial(const std::string & uuid)
{
	uuidIsChildOf.erase(uuid);
	delete uuidToVtkPartialRepresentation[uuid];
	uuidToVtkPartialRepresentation.erase(uuid);
}

//----------------------------------------------------------------------------
void VtkEpcDocument::visualize(const std::string & uuid, const int timeIndex)
{

}

//----------------------------------------------------------------------------
void VtkEpcDocument::setIndexTimeSeries(const int & index)
{
	if (indexTimesSeries != index)
	{
		indexTimesSeries = index;
	}

}*/

//----------------------------------------------------------------------------
void VtkEpcDocument::visualize(const std::string & uuid)
{
	auto uuidToAttach = uuidIsChildOf[uuid]->getUuid();
	switch (uuidIsChildOf[uuid]->getType())
	{
	case VtkEpcCommon::GRID_2D:
	{
		uuidToVtkGrid2DRepresentation[uuidIsChildOf[uuid]->getUuid()]->visualize(uuid);
		break;
	}
	case VtkEpcCommon::POLYLINE_SET:
	{
		auto object = epcPackage->getResqmlAbstractObjectByUuid(uuidIsChildOf[uuid]->getUuid());
		auto typeRepresentation = object->getXmlTag();
		if (typeRepresentation == "PolylineRepresentation")
		{
			uuidToVtkPolylineRepresentation[uuidIsChildOf[uuid]->getUuid()]->visualize(uuid);
		}
		else
		{
			uuidToVtkSetPatch[uuidIsChildOf[uuid]->getUuid()]->visualize(uuid);
		}
		break;
	}
	case VtkEpcCommon::TRIANGULATED_SET:
	{
		auto object = epcPackage->getResqmlAbstractObjectByUuid(uuidIsChildOf[uuid]->getUuid());
		auto typeRepresentation = object->getXmlTag();
		if (typeRepresentation == "TriangulatedRepresentation")
		{
			uuidToVtkTriangulatedRepresentation[uuidIsChildOf[uuid]->getUuid()]->visualize(uuid);
		}
		else
		{
			uuidToVtkSetPatch[uuidIsChildOf[uuid]->getUuid()]->visualize(uuid);
		}
		break;
	}
	case VtkEpcCommon::WELL_TRAJ:
	{
		uuidToVtkWellboreTrajectoryRepresentation[uuidIsChildOf[uuid]->getUuid()]->visualize(uuid);
		break;
	}
	case VtkEpcCommon::IJK_GRID:
	{
		uuidToVtkIjkGridRepresentation[uuidIsChildOf[uuid]->getUuid()]->visualize(uuid);
		break;
	}
	case VtkEpcCommon::UNSTRUC_GRID:
	{
		uuidToVtkUnstructuredGridRepresentation[uuidIsChildOf[uuid]->getUuid()]->visualize(uuid);
		break;
	}
	case VtkEpcCommon::SUB_REP:
	{
		if (uuidIsChildOf[uuid]->getParentType() == VtkEpcCommon::IJK_GRID)
		{
			uuidToVtkIjkGridRepresentation[uuidIsChildOf[uuid]->getUuid()]->visualize(uuid);
		}
		if (uuidIsChildOf[uuid]->getParentType() == VtkEpcCommon::UNSTRUC_GRID)
		{
			uuidToVtkUnstructuredGridRepresentation[uuidIsChildOf[uuid]->getUuid()]->visualize(uuid);
		}
		if (uuidIsChildOf[uuid]->getParentType() == VtkEpcCommon::PARTIAL)
		{
			auto parent = uuidIsChildOf[uuid]->getParent();

			switch (uuidIsChildOf[parent]->getParentType() )
			{
			case VtkEpcCommon::GRID_2D:
			{
				uuidToVtkGrid2DRepresentation[uuid]->visualize(uuid);
				break;
			}
			case VtkEpcCommon::WELL_TRAJ:
			{
				uuidToVtkWellboreTrajectoryRepresentation[uuid]->visualize(uuid);
				break;
			}
			case VtkEpcCommon::IJK_GRID:
			{
				uuidToVtkIjkGridRepresentation[uuid]->visualize(uuid);
				break;
			}
			case VtkEpcCommon::UNSTRUC_GRID:
			{
				uuidToVtkUnstructuredGridRepresentation[uuid]->visualize(uuid);
				break;
			}
			}
		}
		break;
	}
	case VtkEpcCommon::PARTIAL:
	{
		uuidToVtkPartialRepresentation[uuidIsChildOf[uuid]->getUuid()]->visualize(uuid);
		break;
	}
	case (VtkEpcCommon::TIME_SERIES):
				{
		uuidToAttach = uuidIsChildOf[uuid]->getParent();
		switch (uuidIsChildOf[uuid]->getParentType())
		{
		case VtkEpcCommon::GRID_2D:
		{
			uuidToVtkGrid2DRepresentation[uuidIsChildOf[uuid]->getParent()]->visualize(uuid);
			break;
		}
		case VtkEpcCommon::POLYLINE_SET:
		{
			auto object = epcPackage->getResqmlAbstractObjectByUuid(uuidIsChildOf[uuid]->getParent());
			auto typeRepresentation = object->getXmlTag();
			if (typeRepresentation == "PolylineRepresentation")
			{
				uuidToVtkPolylineRepresentation[uuidIsChildOf[uuid]->getParent()]->visualize(uuid);
			}
			else
			{
				uuidToVtkSetPatch[uuidIsChildOf[uuid]->getParent()]->visualize(uuid);
			}
			break;
		}
		case VtkEpcCommon::TRIANGULATED_SET:
		{
			auto object = epcPackage->getResqmlAbstractObjectByUuid(uuid);
			auto typeRepresentation = object->getXmlTag();
			if (typeRepresentation == "TriangulatedRepresentation")
			{
				uuidToVtkTriangulatedRepresentation[uuidIsChildOf[uuid]->getParent()]->visualize(uuid);
			}
			else
			{
				uuidToVtkSetPatch[uuidIsChildOf[uuid]->getParent()]->visualize(uuid);
			}
			break;
		}
		case VtkEpcCommon::WELL_TRAJ:
		{
			uuidToVtkWellboreTrajectoryRepresentation[uuidIsChildOf[uuid]->getParent()]->visualize(uuid);
			break;
		}
		case VtkEpcCommon::IJK_GRID:
		{
			uuidToVtkIjkGridRepresentation[uuidIsChildOf[uuid]->getParent()]->visualize(uuid);
			break;
		}
		case VtkEpcCommon::UNSTRUC_GRID:
		{
			uuidToVtkUnstructuredGridRepresentation[uuidIsChildOf[uuid]->getParent()]->visualize(uuid);
			break;
		}
		case VtkEpcCommon::SUB_REP:
		{
			if (uuidIsChildOf[uuidIsChildOf[uuid]->getParent()]->getParentType() == VtkEpcCommon::IJK_GRID)
			{
				uuidToVtkIjkGridRepresentation[uuidIsChildOf[uuidIsChildOf[uuid]->getParent()]->getUuid()]->visualize(uuid);
			}
			if (uuidIsChildOf[uuidIsChildOf[uuid]->getParent()]->getParentType() == VtkEpcCommon::UNSTRUC_GRID)
			{
				uuidToVtkUnstructuredGridRepresentation[uuidIsChildOf[uuidIsChildOf[uuid]->getParent()]->getUuid()]->visualize(uuid);
			}
			if (uuidIsChildOf[uuidIsChildOf[uuid]->getParent()]->getParentType() == VtkEpcCommon::PARTIAL)
			{
				if (uuidIsChildOf[uuidIsChildOf[uuidIsChildOf[uuid]->getParent()]->getParent()]->getParentType() == VtkEpcCommon::IJK_GRID)
				{
					uuidToVtkIjkGridRepresentation[uuidIsChildOf[uuidIsChildOf[uuid]->getParent()]->getUuid()]->visualize(uuid);
				}
				if (uuidIsChildOf[uuidIsChildOf[uuidIsChildOf[uuid]->getParent()]->getParent()]->getParentType() == VtkEpcCommon::UNSTRUC_GRID)
				{
					uuidToVtkUnstructuredGridRepresentation[uuidIsChildOf[uuidIsChildOf[uuid]->getParent()]->getUuid()]->visualize(uuid);
				}
			}
			break;
		}
		case VtkEpcCommon::PARTIAL:
		{
			uuidToVtkPartialRepresentation[uuidIsChildOf[uuid]->getParent()]->visualize(uuid);
			break;
		}
		default:
			break;
		}
				}
	case VtkEpcCommon::PROPERTY:
	{
		uuidToAttach = uuidIsChildOf[uuid]->getParent();
		switch (uuidIsChildOf[uuid]->getParentType())
		{
		case VtkEpcCommon::GRID_2D:
		{
			uuidToVtkGrid2DRepresentation[uuidIsChildOf[uuid]->getParent()]->visualize(uuid);
			break;
		}
		case VtkEpcCommon::POLYLINE_SET:
		{
			auto object = epcPackage->getResqmlAbstractObjectByUuid(uuidIsChildOf[uuid]->getParent());
			auto typeRepresentation = object->getXmlTag();
			if (typeRepresentation == "PolylineRepresentation")
			{
				uuidToVtkPolylineRepresentation[uuidIsChildOf[uuid]->getParent()]->visualize(uuid);
			}
			else
			{
				uuidToVtkSetPatch[uuidIsChildOf[uuid]->getParent()]->visualize(uuid);
			}
			break;
		}
		case VtkEpcCommon::TRIANGULATED_SET:
		{
			auto object = epcPackage->getResqmlAbstractObjectByUuid(uuid);
			auto typeRepresentation = object->getXmlTag();
			if (typeRepresentation == "TriangulatedRepresentation")
			{
				uuidToVtkTriangulatedRepresentation[uuidIsChildOf[uuid]->getParent()]->visualize(uuid);
			}
			else
			{
				uuidToVtkSetPatch[uuidIsChildOf[uuid]->getParent()]->visualize(uuid);
			}
			break;
		}
		case VtkEpcCommon::WELL_TRAJ:
		{
			uuidToVtkWellboreTrajectoryRepresentation[uuidIsChildOf[uuid]->getParent()]->visualize(uuid);
			break;
		}
		case VtkEpcCommon::IJK_GRID:
		{
			uuidToVtkIjkGridRepresentation[uuidIsChildOf[uuid]->getParent()]->visualize(uuid);
			break;
		}
		case VtkEpcCommon::UNSTRUC_GRID:
		{
			uuidToVtkUnstructuredGridRepresentation[uuidIsChildOf[uuid]->getParent()]->visualize(uuid);
			break;
		}
		case VtkEpcCommon::SUB_REP:
		{
			if (uuidIsChildOf[uuidIsChildOf[uuid]->getParent()]->getParentType() == VtkEpcCommon::IJK_GRID)
			{
				uuidToVtkIjkGridRepresentation[uuidIsChildOf[uuidIsChildOf[uuid]->getParent()]->getUuid()]->visualize(uuid);
			}
			if (uuidIsChildOf[uuidIsChildOf[uuid]->getParent()]->getParentType() == VtkEpcCommon::UNSTRUC_GRID)
			{
				uuidToVtkUnstructuredGridRepresentation[uuidIsChildOf[uuidIsChildOf[uuid]->getParent()]->getUuid()]->visualize(uuid);
			}
			if (uuidIsChildOf[uuidIsChildOf[uuid]->getParent()]->getParentType() == VtkEpcCommon::PARTIAL)
			{
				if (uuidIsChildOf[uuidIsChildOf[uuidIsChildOf[uuid]->getParent()]->getParent()]->getParentType() == VtkEpcCommon::IJK_GRID)
				{
					uuidToVtkIjkGridRepresentation[uuidIsChildOf[uuidIsChildOf[uuid]->getParent()]->getUuid()]->visualize(uuid);
				}
				if (uuidIsChildOf[uuidIsChildOf[uuidIsChildOf[uuid]->getParent()]->getParent()]->getParentType() == VtkEpcCommon::UNSTRUC_GRID)
				{
					uuidToVtkUnstructuredGridRepresentation[uuidIsChildOf[uuidIsChildOf[uuid]->getParent()]->getUuid()]->visualize(uuid);
				}
			}
			break;
		}
		case VtkEpcCommon::PARTIAL:
		{
			uuidToVtkPartialRepresentation[uuidIsChildOf[uuid]->getParent()]->visualize(uuid);
			break;
		}
		default:
			break;
		}
	}
	default:
	{
		break;
	}

	}
	// attach representation to EpcDocument VtkMultiBlockDataSet
	if (std::find(attachUuids.begin(), attachUuids.end(), uuidToAttach) == attachUuids.end())
	{
		this->detach();
		attachUuids.push_back(uuidToAttach);
		this->attach();
	}
}

//----------------------------------------------------------------------------
void VtkEpcDocument::remove(const std::string & uuid)
{
	auto uuidtoAttach = uuid;
	if (uuidIsChildOf[uuid]->getType() == VtkEpcCommon::PROPERTY)
	{
		uuidtoAttach = uuidIsChildOf[uuid]->getParent();
	}

	if (std::find(attachUuids.begin(), attachUuids.end(), uuidtoAttach) != attachUuids.end())
	{
		switch (uuidIsChildOf[uuidtoAttach]->getType())
		{
		case VtkEpcCommon::GRID_2D:	{
			uuidToVtkGrid2DRepresentation[uuidtoAttach]->remove(uuid);
			if (uuid == uuidtoAttach){
				this->detach();
				attachUuids.erase(std::find(attachUuids.begin(), attachUuids.end(), uuid));
				this->attach();
			}
			break;
		}
		case VtkEpcCommon::POLYLINE_SET:	{
			auto object = epcPackage->getResqmlAbstractObjectByUuid(uuidtoAttach);
			auto typeRepresentation = object->getXmlTag();
			if (typeRepresentation == "PolylineRepresentation") {
				uuidToVtkPolylineRepresentation[uuidIsChildOf[uuidtoAttach]->getUuid()]->remove(uuid);
				if (uuid == uuidtoAttach){
					this->detach();
					attachUuids.erase(std::find(attachUuids.begin(), attachUuids.end(), uuid));
					this->attach();
				}
			}
			else  {
				uuidToVtkSetPatch[uuidtoAttach]->remove(uuid);
				if (uuid == uuidtoAttach){
					this->detach();
					attachUuids.erase(std::find(attachUuids.begin(), attachUuids.end(), uuid));
					this->attach();
				}
			}
			break;
		}
		case VtkEpcCommon::TRIANGULATED_SET:	{
			auto object = epcPackage->getResqmlAbstractObjectByUuid(uuidtoAttach);
			auto typeRepresentation = object->getXmlTag();
			if (typeRepresentation == "TriangulatedRepresentation")  {
				uuidToVtkTriangulatedRepresentation[uuidtoAttach]->remove(uuid);
				if (uuid == uuidtoAttach){
					this->detach();
					attachUuids.erase(std::find(attachUuids.begin(), attachUuids.end(), uuid));
					this->attach();
				}
			}
			else  {
				uuidToVtkSetPatch[uuidtoAttach]->remove(uuid);
				if (uuid == uuidtoAttach){
					this->detach();
					attachUuids.erase(std::find(attachUuids.begin(), attachUuids.end(), uuid));
					this->attach();
				}
			}
			break;
		}
		case VtkEpcCommon::WELL_TRAJ:	{
			uuidToVtkWellboreTrajectoryRepresentation[uuidtoAttach]->remove(uuid);
			if (uuid == uuidtoAttach){
				this->detach();
				attachUuids.erase(std::find(attachUuids.begin(), attachUuids.end(), uuid));
				this->attach();
			}
			break;
		}
		case VtkEpcCommon::IJK_GRID:
		{
			uuidToVtkIjkGridRepresentation[uuidtoAttach]->remove(uuid);
			if (uuid == uuidtoAttach)
			{
				this->detach();
				attachUuids.erase(std::find(attachUuids.begin(), attachUuids.end(), uuid));
				this->attach();
			}
			break;
		}
		case VtkEpcCommon::UNSTRUC_GRID:
		{
			uuidToVtkUnstructuredGridRepresentation[uuidtoAttach]->remove(uuid);
			if (uuid == uuidtoAttach)
			{
				this->detach();
				attachUuids.erase(std::find(attachUuids.begin(), attachUuids.end(), uuid));
				this->attach();
			}
			break;
		}
		case VtkEpcCommon::SUB_REP:
		{
			if (uuidIsChildOf[uuidIsChildOf[uuid]->getParent()]->getParentType() == VtkEpcCommon::Resqml2Type::IJK_GRID)
			{
				uuidToVtkIjkGridRepresentation[uuidIsChildOf[uuidIsChildOf[uuid]->getParent()]->getUuid()]->remove(uuid);
			}
			if (uuidIsChildOf[uuidIsChildOf[uuid]->getParent()]->getParentType() == VtkEpcCommon::Resqml2Type::UNSTRUC_GRID)
			{
				uuidToVtkUnstructuredGridRepresentation[uuidIsChildOf[uuidIsChildOf[uuid]->getParent()]->getUuid()]->remove(uuid);
			}
			if (uuidIsChildOf[uuidIsChildOf[uuid]->getParent()]->getParentType() == VtkEpcCommon::PARTIAL)
			{
				if (uuidIsChildOf[uuidIsChildOf[uuidIsChildOf[uuid]->getParent()]->getParent()]->getParentType() == VtkEpcCommon::IJK_GRID)
				{
					uuidToVtkIjkGridRepresentation[uuidIsChildOf[uuidIsChildOf[uuid]->getParent()]->getUuid()]->remove(uuid);
				}
				if (uuidIsChildOf[uuidIsChildOf[uuidIsChildOf[uuid]->getParent()]->getParent()]->getParentType() == VtkEpcCommon::UNSTRUC_GRID)
				{
					uuidToVtkUnstructuredGridRepresentation[uuidIsChildOf[uuidIsChildOf[uuid]->getParent()]->getUuid()]->remove(uuid);
				}
			}
			if (uuid == uuidtoAttach)
			{
				this->detach();
				attachUuids.erase(std::find(attachUuids.begin(), attachUuids.end(), uuid));
				this->attach();
			}
			break;
		}
		case VtkEpcCommon::PARTIAL:
		{
			uuidToVtkPartialRepresentation[uuidtoAttach]->remove(uuid);
			break;
		}
		default:
			break;
		}
	}
}

//----------------------------------------------------------------------------
void VtkEpcDocument::attach()
{
	for (unsigned int newBlockIndex = 0; newBlockIndex < attachUuids.size(); ++newBlockIndex)
	{
		std::string uuid = attachUuids[newBlockIndex];
		if (uuidIsChildOf[uuid]->getType() == VtkEpcCommon::GRID_2D)
		{
			vtkOutput->SetBlock(newBlockIndex, uuidToVtkGrid2DRepresentation[uuid]->getOutput());
			vtkOutput->GetMetaData(newBlockIndex)->Set(vtkCompositeDataSet::NAME(), uuidToVtkGrid2DRepresentation[uuid]->getUuid().c_str());
		}
		if (uuidIsChildOf[uuid]->getType() == VtkEpcCommon::POLYLINE_SET)
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
		if (uuidIsChildOf[uuid]->getType() == VtkEpcCommon::TRIANGULATED_SET)
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
		if (uuidIsChildOf[uuid]->getType() == VtkEpcCommon::WELL_TRAJ)
		{
			vtkOutput->SetBlock(newBlockIndex, uuidToVtkWellboreTrajectoryRepresentation[uuid]->getOutput());
			vtkOutput->GetMetaData(newBlockIndex)->Set(vtkCompositeDataSet::NAME(), uuidToVtkWellboreTrajectoryRepresentation[uuid]->getUuid().c_str());
		}
		if (uuidIsChildOf[uuid]->getType() == VtkEpcCommon::IJK_GRID)
		{
			vtkOutput->SetBlock(newBlockIndex, uuidToVtkIjkGridRepresentation[uuid]->getOutput());
			vtkOutput->GetMetaData(newBlockIndex)->Set(vtkCompositeDataSet::NAME(), uuidToVtkIjkGridRepresentation[uuid]->getUuid().c_str());
		}
		if (uuidIsChildOf[uuid]->getType() == VtkEpcCommon::UNSTRUC_GRID)
		{
			vtkOutput->SetBlock(newBlockIndex, uuidToVtkUnstructuredGridRepresentation[uuid]->getOutput());
			vtkOutput->GetMetaData(newBlockIndex)->Set(vtkCompositeDataSet::NAME(), uuidToVtkUnstructuredGridRepresentation[uuid]->getUuid().c_str());
		}
		if (uuidIsChildOf[uuid]->getType() == VtkEpcCommon::SUB_REP)
		{
			if (uuidIsChildOf[uuid]->getParentType() == VtkEpcCommon::IJK_GRID)
			{
				vtkOutput->SetBlock(newBlockIndex, uuidToVtkIjkGridRepresentation[uuidIsChildOf[uuid]->getUuid()]->getOutput());
				vtkOutput->GetMetaData(newBlockIndex)->Set(vtkCompositeDataSet::NAME(), uuidToVtkIjkGridRepresentation[uuid]->getUuid().c_str());
			}
			if (uuidIsChildOf[uuid]->getParentType() == VtkEpcCommon::UNSTRUC_GRID)
			{
				vtkOutput->SetBlock(newBlockIndex, uuidToVtkUnstructuredGridRepresentation[uuidIsChildOf[uuid]->getUuid()]->getOutput());
				vtkOutput->GetMetaData(newBlockIndex)->Set(vtkCompositeDataSet::NAME(), uuidToVtkUnstructuredGridRepresentation[uuidIsChildOf[uuid]->getUuid()]->getUuid().c_str());
			}
			if (uuidIsChildOf[uuid]->getParentType() == VtkEpcCommon::PARTIAL)
			{
				auto parentUuidType = uuidToVtkPartialRepresentation[uuidIsChildOf[uuid]->getParent()]->getType();

				switch (parentUuidType)
				{
				case VtkEpcCommon::GRID_2D:
				{
					vtkOutput->SetBlock(newBlockIndex, uuidToVtkGrid2DRepresentation[uuid]->getOutput());
					vtkOutput->GetMetaData(newBlockIndex)->Set(vtkCompositeDataSet::NAME(), uuidToVtkGrid2DRepresentation[uuid]->getUuid().c_str());
					break;
				}
				case VtkEpcCommon::WELL_TRAJ:
				{
					vtkOutput->SetBlock(newBlockIndex, uuidToVtkWellboreTrajectoryRepresentation[uuid]->getOutput());
					vtkOutput->GetMetaData(newBlockIndex)->Set(vtkCompositeDataSet::NAME(), uuidToVtkWellboreTrajectoryRepresentation[uuid]->getUuid().c_str());
					break;
				}
				case VtkEpcCommon::IJK_GRID:
				{
					vtkOutput->SetBlock(newBlockIndex, uuidToVtkIjkGridRepresentation[uuidIsChildOf[uuid]->getUuid()]->getOutput());
					vtkOutput->GetMetaData(newBlockIndex)->Set(vtkCompositeDataSet::NAME(), uuidToVtkIjkGridRepresentation[uuid]->getUuid().c_str());
					break;
				}
				case VtkEpcCommon::UNSTRUC_GRID:
				{
					vtkOutput->SetBlock(newBlockIndex, uuidToVtkUnstructuredGridRepresentation[uuidIsChildOf[uuid]->getUuid()]->getOutput());
					vtkOutput->GetMetaData(newBlockIndex)->Set(vtkCompositeDataSet::NAME(), uuidToVtkUnstructuredGridRepresentation[uuidIsChildOf[uuid]->getUuid()]->getUuid().c_str());
					break;
				}
				default:
					break;
				}
			}
		}
	}
}

//----------------------------------------------------------------------------
void VtkEpcDocument::addProperty(const std::string & uuidProperty, vtkDataArray* dataProperty)
{
	switch (uuidIsChildOf[uuidProperty]->getType())
	{
	case VtkEpcCommon::GRID_2D:
	{
		uuidToVtkGrid2DRepresentation[uuidIsChildOf[uuidProperty]->getUuid()]->addProperty(uuidProperty, dataProperty);
		break;
	}
	case VtkEpcCommon::POLYLINE_SET:
	{
		auto object = epcPackage->getResqmlAbstractObjectByUuid(uuidIsChildOf[uuidProperty]->getUuid());
		auto typeRepresentation = object->getXmlTag();
		if (typeRepresentation == "PolylineRepresentation")
		{
			uuidToVtkPolylineRepresentation[uuidIsChildOf[uuidProperty]->getUuid()]->addProperty(uuidProperty, dataProperty);
		}
		else
		{
			uuidToVtkSetPatch[uuidIsChildOf[uuidProperty]->getUuid()]->addProperty(uuidProperty, dataProperty);
		}
		break;
	}
	case VtkEpcCommon::TRIANGULATED_SET:
	{
		auto object = epcPackage->getResqmlAbstractObjectByUuid(uuidIsChildOf[uuidProperty]->getUuid());
		auto typeRepresentation = object->getXmlTag();
		if (typeRepresentation == "TriangulatedRepresentation")
		{
			uuidToVtkTriangulatedRepresentation[uuidIsChildOf[uuidProperty]->getUuid()]->addProperty(uuidProperty, dataProperty);
		}
		else
		{
			uuidToVtkSetPatch[uuidIsChildOf[uuidProperty]->getUuid()]->addProperty(uuidProperty, dataProperty);
		}
		break;
	}
	case VtkEpcCommon::WELL_TRAJ:
	{
		uuidToVtkWellboreTrajectoryRepresentation[uuidIsChildOf[uuidProperty]->getUuid()]->addProperty(uuidProperty, dataProperty);
		break;
	}
	case VtkEpcCommon::IJK_GRID:
		uuidToVtkIjkGridRepresentation[uuidIsChildOf[uuidProperty]->getUuid()]->addProperty(uuidProperty, dataProperty);
		break;
	case VtkEpcCommon::UNSTRUC_GRID:
	{
		uuidToVtkUnstructuredGridRepresentation[uuidIsChildOf[uuidProperty]->getUuid()]->addProperty(uuidProperty, dataProperty);
		break;
	}
	case VtkEpcCommon::SUB_REP:
	{
		if (uuidIsChildOf[uuidProperty]->getParentType() == VtkEpcCommon::IJK_GRID)
		{
			uuidToVtkIjkGridRepresentation[uuidIsChildOf[uuidProperty]->getUuid()]->addProperty(uuidProperty, dataProperty);
		}
		if (uuidIsChildOf[uuidProperty]->getParentType() == VtkEpcCommon::UNSTRUC_GRID)
		{
			uuidToVtkUnstructuredGridRepresentation[uuidIsChildOf[uuidProperty]->getUuid()]->addProperty(uuidProperty, dataProperty);
		}
		if (uuidIsChildOf[uuidProperty]->getParentType() == VtkEpcCommon::PARTIAL)
		{
			auto parentUuidType = uuidToVtkPartialRepresentation[uuidIsChildOf[uuidProperty]->getParent()]->getType();

			switch (parentUuidType)
			{
			case VtkEpcCommon::GRID_2D:
			{
				uuidToVtkGrid2DRepresentation[uuidIsChildOf[uuidProperty]->getUuid()]->addProperty(uuidProperty, dataProperty);
				break;
			}
			case VtkEpcCommon::WELL_TRAJ:
			{
				uuidToVtkWellboreTrajectoryRepresentation[uuidIsChildOf[uuidProperty]->getUuid()]->addProperty(uuidProperty, dataProperty);
				break;
			}
			case VtkEpcCommon::IJK_GRID:
			{
				uuidToVtkIjkGridRepresentation[uuidIsChildOf[uuidProperty]->getUuid()]->addProperty(uuidProperty, dataProperty);
				break;
			}
			case VtkEpcCommon::UNSTRUC_GRID:
			{
				uuidToVtkUnstructuredGridRepresentation[uuidIsChildOf[uuidProperty]->getUuid()]->addProperty(uuidProperty, dataProperty);
				break;
			}
			default:
				break;
			}
		}
	}
	default:
		break;
	}
	// attach representation to EpcDocument VtkMultiBlockDataSet
	std::string parent = uuidIsChildOf[uuidProperty]->getUuid();
	if (std::find(attachUuids.begin(), attachUuids.end(), parent) == attachUuids.end())
	{
		this->detach();
		attachUuids.push_back(parent);
		this->attach();
	}
}

//----------------------------------------------------------------------------
long VtkEpcDocument::getAttachmentPropertyCount(const std::string & uuid, const VtkEpcCommon::FesppAttachmentProperty propertyUnit)
{
	long result = 0;
	switch (uuidIsChildOf[uuid]->getType())
	{
	case VtkEpcCommon::Resqml2Type::IJK_GRID:
		result = uuidToVtkIjkGridRepresentation[uuidIsChildOf[uuid]->getUuid()]->getAttachmentPropertyCount(uuid, propertyUnit);
		break;
	case VtkEpcCommon::Resqml2Type::UNSTRUC_GRID:
		result = uuidToVtkUnstructuredGridRepresentation[uuidIsChildOf[uuid]->getUuid()]->getAttachmentPropertyCount(uuid, propertyUnit);
		break;
	case VtkEpcCommon::Resqml2Type::SUB_REP:
		if (uuidIsChildOf[uuid]->getParentType() == VtkEpcCommon::IJK_GRID)
		{
			result = uuidToVtkIjkGridRepresentation[uuidIsChildOf[uuid]->getUuid()]->getAttachmentPropertyCount(uuid, propertyUnit);
		}
		if (uuidIsChildOf[uuid]->getParentType() == VtkEpcCommon::UNSTRUC_GRID)
		{
			result = uuidToVtkUnstructuredGridRepresentation[uuidIsChildOf[uuid]->getUuid()]->getAttachmentPropertyCount(uuid, propertyUnit);
		}
		if (uuidIsChildOf[uuid]->getParentType() == VtkEpcCommon::PARTIAL)
		{
			auto uuidPartial = uuidIsChildOf[uuid]->getParent();

			if (uuidIsChildOf[uuidPartial]->getParentType() == VtkEpcCommon::IJK_GRID)
			{
				result = uuidToVtkIjkGridRepresentation[uuid]->getAttachmentPropertyCount(uuid, propertyUnit);
			}
			if (uuidIsChildOf[uuid]->getParentType() == VtkEpcCommon::UNSTRUC_GRID)
			{
				result = uuidToVtkUnstructuredGridRepresentation[uuid]->getAttachmentPropertyCount(uuid, propertyUnit);
			}
		}
		break;
	default:
		break;
	}
	return result;
}

//----------------------------------------------------------------------------
int VtkEpcDocument::getICellCount(const std::string & uuid)
{
	long result = 0;
	if (uuidIsChildOf[uuid]->getType() == VtkEpcCommon::Resqml2Type::IJK_GRID)
	{
		result = uuidToVtkIjkGridRepresentation[uuidIsChildOf[uuid]->getUuid()]->getICellCount(uuid);
	}
	else if (uuidIsChildOf[uuid]->getType() == VtkEpcCommon::Resqml2Type::SUB_REP)
	{
		if (uuidIsChildOf[uuid]->getParentType() == VtkEpcCommon::IJK_GRID)
		{
			result = uuidToVtkIjkGridRepresentation[uuidIsChildOf[uuid]->getUuid()]->getICellCount(uuid);
		}
		if (uuidIsChildOf[uuid]->getParentType() == VtkEpcCommon::PARTIAL)
		{
			auto uuidPartial = uuidIsChildOf[uuid]->getParent();

			if (uuidIsChildOf[uuidPartial]->getParentType() == VtkEpcCommon::IJK_GRID)
			{
				result = uuidToVtkIjkGridRepresentation[uuid]->getICellCount(uuid);
			}
		}
	}
	return result;
}

//----------------------------------------------------------------------------
int VtkEpcDocument::getJCellCount(const std::string & uuid)
{
	long result = 0;
	if (uuidIsChildOf[uuid]->getType() == VtkEpcCommon::Resqml2Type::IJK_GRID)
	{
		result = uuidToVtkIjkGridRepresentation[uuidIsChildOf[uuid]->getUuid()]->getJCellCount(uuid);
	}
	else if (uuidIsChildOf[uuid]->getType() == VtkEpcCommon::Resqml2Type::SUB_REP)
	{
		if (uuidIsChildOf[uuid]->getParentType() == VtkEpcCommon::IJK_GRID)
		{
			result = uuidToVtkIjkGridRepresentation[uuidIsChildOf[uuid]->getUuid()]->getJCellCount(uuid);
		}
		if (uuidIsChildOf[uuid]->getParentType() == VtkEpcCommon::PARTIAL)
		{
			auto uuidPartial = uuidIsChildOf[uuid]->getParent();

			if (uuidIsChildOf[uuidPartial]->getParentType() == VtkEpcCommon::IJK_GRID)
			{
				result = uuidToVtkIjkGridRepresentation[uuid]->getJCellCount(uuid);
			}
		}
	}
	return result;
}

//----------------------------------------------------------------------------
int VtkEpcDocument::getKCellCount(const std::string & uuid)
{
	long result = 0;
	if (uuidIsChildOf[uuid]->getType() == VtkEpcCommon::Resqml2Type::IJK_GRID)
	{
		result = uuidToVtkIjkGridRepresentation[uuidIsChildOf[uuid]->getUuid()]->getKCellCount(uuid);
	}
	else if (uuidIsChildOf[uuid]->getType() == VtkEpcCommon::Resqml2Type::SUB_REP)
	{
		if (uuidIsChildOf[uuid]->getParentType() == VtkEpcCommon::IJK_GRID)
		{
			result = uuidToVtkIjkGridRepresentation[uuidIsChildOf[uuid]->getUuid()]->getKCellCount(uuid);
		}
		if (uuidIsChildOf[uuid]->getParentType() == VtkEpcCommon::PARTIAL)
		{
			auto uuidPartial = uuidIsChildOf[uuid]->getParent();

			if (uuidIsChildOf[uuidPartial]->getParentType() == VtkEpcCommon::IJK_GRID)
			{
				result = uuidToVtkIjkGridRepresentation[uuid]->getKCellCount(uuid);
			}
		}
	}
	return result;
}

//----------------------------------------------------------------------------
int VtkEpcDocument::getInitKIndex(const std::string & uuid)
{
	long result = 0;
	if (uuidIsChildOf[uuid]->getType() == VtkEpcCommon::Resqml2Type::IJK_GRID)
	{
		result = uuidToVtkIjkGridRepresentation[uuidIsChildOf[uuid]->getUuid()]->getInitKIndex(uuid);
	}
	else if (uuidIsChildOf[uuid]->getType() == VtkEpcCommon::Resqml2Type::SUB_REP)
	{
		if (uuidIsChildOf[uuid]->getParentType() == VtkEpcCommon::IJK_GRID)
		{
			result = uuidToVtkIjkGridRepresentation[uuidIsChildOf[uuid]->getUuid()]->getInitKIndex(uuid);
		}
		if (uuidIsChildOf[uuid]->getParentType() == VtkEpcCommon::PARTIAL)
		{
			auto uuidPartial = uuidIsChildOf[uuid]->getParent();

			if (uuidIsChildOf[uuidPartial]->getParentType() == VtkEpcCommon::IJK_GRID)
			{
				result = uuidToVtkIjkGridRepresentation[uuid]->getInitKIndex(uuid);
			}
		}
	}
	return result;
}

//----------------------------------------------------------------------------
VtkEpcCommon::Resqml2Type VtkEpcDocument::getType(std::string uuid)
{
	return uuidIsChildOf[uuid]->getType();
}

//----------------------------------------------------------------------------
VtkEpcCommon* VtkEpcDocument::getInfoUuid(std::string uuid)
{
	return uuidIsChildOf[uuid];
}

//----------------------------------------------------------------------------
common::EpcDocument * VtkEpcDocument::getEpcDocument()
{
	return epcPackage;
}

//----------------------------------------------------------------------------
std::vector<std::string> VtkEpcDocument::getListUuid()
{
	return uuidRep;
}


//----------------------------------------------------------------------------
std::vector<VtkEpcCommon*> VtkEpcDocument::getTreeView() const
{
	return treeView;
}
