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
					uuidPartialRep.push_back(polylines[iter]->getUuid());
					uuidIsChildOf[polylines[iter]->getUuid()].myType = VtkEpcTools::PARTIAL;
					uuidIsChildOf[polylines[iter]->getUuid()].uuid = polylines[iter]->getUuid();
				}
				else
				{
					uuidIsChildOf[polylines[iter]->getUuid()].myType = VtkEpcTools::PARTIAL;
					uuidIsChildOf[polylines[iter]->getUuid()].uuid = "UNKNOWN";
				}

			}
			else
			{
				createTreeVtk(polylines[iter]->getUuid(), uuidParent, polylines[iter]->getTitle().c_str(), VtkEpcTools::POLYLINE_SET);
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
					uuidPartialRep.push_back(polylines[iter]->getUuid());
					uuidIsChildOf[polylines[iter]->getUuid()].myType = VtkEpcTools::PARTIAL;
					uuidIsChildOf[polylines[iter]->getUuid()].uuid = polylines[iter]->getUuid();
				}
				else
				{
					uuidIsChildOf[polylines[iter]->getUuid()].myType = VtkEpcTools::PARTIAL;
					uuidIsChildOf[polylines[iter]->getUuid()].uuid = "UNKNOWN";
				}
			}
			else
			{
				createTreeVtk(polylines[iter]->getUuid(), uuidParent, polylines[iter]->getTitle().c_str(), VtkEpcTools::POLYLINE_SET);
			}
			//property
			auto valuesPropertySet = polylines[iter]->getValuesPropertySet();
			for (unsigned int i = 0; i < valuesPropertySet.size(); ++i)
			{
				createTreeVtk(valuesPropertySet[i]->getUuid(), polylines[iter]->getUuid(), valuesPropertySet[i]->getTitle().c_str(), VtkEpcTools::PROPERTY);
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
					uuidPartialRep.push_back(triangulated[iter]->getUuid());
					uuidIsChildOf[triangulated[iter]->getUuid()].myType = VtkEpcTools::PARTIAL;
					uuidIsChildOf[triangulated[iter]->getUuid()].uuid = triangulated[iter]->getUuid();
				}
				else
				{
					uuidIsChildOf[triangulated[iter]->getUuid()].myType = VtkEpcTools::PARTIAL;
					uuidIsChildOf[triangulated[iter]->getUuid()].uuid = "UNKNOWN";
				}
			}
			else
			{
				createTreeVtk(triangulated[iter]->getUuid(), uuidParent, triangulated[iter]->getTitle().c_str(), VtkEpcTools::TRIANGULATED_SET);
			}
			//property
			auto valuesPropertySet = triangulated[iter]->getValuesPropertySet();
			for (unsigned int i = 0; i < valuesPropertySet.size(); ++i)
			{
				createTreeVtk(valuesPropertySet[i]->getUuid(), triangulated[iter]->getUuid(), valuesPropertySet[i]->getTitle().c_str(), VtkEpcTools::PROPERTY);
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
					uuidPartialRep.push_back(grid2D[iter]->getUuid());
					uuidIsChildOf[grid2D[iter]->getUuid()].myType = VtkEpcTools::PARTIAL;
					uuidIsChildOf[grid2D[iter]->getUuid()].uuid = grid2D[iter]->getUuid();
				}
				else
				{
					uuidIsChildOf[grid2D[iter]->getUuid()].myType = VtkEpcTools::PARTIAL;
					uuidIsChildOf[grid2D[iter]->getUuid()].uuid = "UNKNOWN";
				}
			}
			else
			{
				createTreeVtk(grid2D[iter]->getUuid(), uuidParent, grid2D[iter]->getTitle().c_str(), VtkEpcTools::GRID_2D);
			}
			//property
			auto valuesPropertySet = grid2D[iter]->getValuesPropertySet();
			for (unsigned int i = 0; i < valuesPropertySet.size(); ++i)
			{
				createTreeVtk(valuesPropertySet[i]->getUuid(), grid2D[iter]->getUuid(), valuesPropertySet[i]->getTitle().c_str(), VtkEpcTools::PROPERTY);
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
					uuidPartialRep.push_back(ijkGrid[iter]->getUuid());
					uuidIsChildOf[ijkGrid[iter]->getUuid()].myType = VtkEpcTools::PARTIAL;
					uuidIsChildOf[ijkGrid[iter]->getUuid()].uuid = ijkGrid[iter]->getUuid();
				}
				else
				{
					uuidIsChildOf[ijkGrid[iter]->getUuid()].myType = VtkEpcTools::PARTIAL;
					uuidIsChildOf[ijkGrid[iter]->getUuid()].uuid = "UNKNOWN";
				}

			}
			else
			{
				createTreeVtk(ijkGrid[iter]->getUuid(), uuidParent, ijkGrid[iter]->getTitle().c_str(), VtkEpcTools::IJK_GRID);
			}
			//property
			auto valuesPropertySet = ijkGrid[iter]->getValuesPropertySet();
			for (unsigned int i = 0; i < valuesPropertySet.size(); ++i)
			{
				createTreeVtk(valuesPropertySet[i]->getUuid(), ijkGrid[iter]->getUuid(), valuesPropertySet[i]->getTitle().c_str(), VtkEpcTools::PROPERTY);
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
					uuidPartialRep.push_back(unstructuredGrid[iter]->getUuid());
					uuidIsChildOf[unstructuredGrid[iter]->getUuid()].myType = VtkEpcTools::PARTIAL;
					uuidIsChildOf[unstructuredGrid[iter]->getUuid()].uuid = unstructuredGrid[iter]->getUuid();
				}
				else
				{
					uuidIsChildOf[unstructuredGrid[iter]->getUuid()].myType = VtkEpcTools::PARTIAL;
					uuidIsChildOf[unstructuredGrid[iter]->getUuid()].uuid = "UNKNOWN";
				}
			}
			else
			{
				createTreeVtk(unstructuredGrid[iter]->getUuid(), uuidParent, unstructuredGrid[iter]->getTitle().c_str(), VtkEpcTools::UNSTRUC_GRID);
			}
			//property
			auto valuesPropertySet = unstructuredGrid[iter]->getValuesPropertySet();
			for (unsigned int i = 0; i < valuesPropertySet.size(); ++i)
			{
				createTreeVtk(valuesPropertySet[i]->getUuid(), unstructuredGrid[iter]->getUuid(), valuesPropertySet[i]->getTitle().c_str(), VtkEpcTools::PROPERTY);
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
					uuidPartialRep.push_back(WellboreTrajectory[iter]->getUuid());
					uuidIsChildOf[WellboreTrajectory[iter]->getUuid()].myType = VtkEpcTools::PARTIAL;
					uuidIsChildOf[WellboreTrajectory[iter]->getUuid()].uuid = WellboreTrajectory[iter]->getUuid();
				}
				else
				{
					uuidIsChildOf[WellboreTrajectory[iter]->getUuid()].myType = VtkEpcTools::PARTIAL;
					uuidIsChildOf[WellboreTrajectory[iter]->getUuid()].uuid = "UNKNOWN";
				}
			}
			else
			{
				createTreeVtk(WellboreTrajectory[iter]->getUuid(), uuidParent, WellboreTrajectory[iter]->getTitle().c_str(), VtkEpcTools::WELL_TRAJ);
			}
			//property
			auto valuesPropertySet = WellboreTrajectory[iter]->getValuesPropertySet();
			for (unsigned int i = 0; i < valuesPropertySet.size(); ++i)
			{
				createTreeVtk(valuesPropertySet[i]->getUuid(), WellboreTrajectory[iter]->getUuid(), valuesPropertySet[i]->getTitle().c_str(), VtkEpcTools::PROPERTY);
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
			if (subRepresentation[iter]->isPartial())
			{
				auto vtkEpcSrc = epcSet->getVtkEpcDocument(subRepresentation[iter]->getUuid());
				if (vtkEpcSrc)
				{
					createTreeVtkPartialRep(subRepresentation[iter]->getUuid(), vtkEpcSrc);
					uuidPartialRep.push_back(subRepresentation[iter]->getUuid());
					uuidIsChildOf[subRepresentation[iter]->getUuid()].myType = VtkEpcTools::PARTIAL;
					uuidIsChildOf[subRepresentation[iter]->getUuid()].uuid = subRepresentation[iter]->getUuid();
				}
				else
				{
					uuidIsChildOf[subRepresentation[iter]->getUuid()].myType = VtkEpcTools::PARTIAL;
					uuidIsChildOf[subRepresentation[iter]->getUuid()].uuid = "UNKNOWN";
				}

			}
			else
			{
				auto uuidParent = subRepresentation[iter]->getSupportingRepresentationUuid(0);

				createTreeVtk(subRepresentation[iter]->getUuid(), uuidParent, subRepresentation[iter]->getTitle().c_str(), VtkEpcTools::SUB_REP);
			}
			//property
			auto valuesPropertySet = subRepresentation[iter]->getValuesPropertySet();
			for (unsigned int i = 0; i < valuesPropertySet.size(); ++i)
			{
				createTreeVtk(valuesPropertySet[i]->getUuid(), subRepresentation[iter]->getUuid(), valuesPropertySet[i]->getTitle().c_str(), VtkEpcTools::PROPERTY);
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
			for (auto i = 0; i < propSeries.size(); ++i)
			{
				uuidIsChildOf[propSeries[i]->getUuid()].myType = VtkEpcTools::TIME_SERIES;
				uuidIsChildOf[propSeries[i]->getUuid()].timeIndex = propSeries[i]->getTimeIndex();
				/*
				if (propSeries[i]->isAssociatedToOneStandardEnergisticsPropertyKind)
				{
					getEnergisticsPropertyKind
				}
				else
				{
					getLocalPropertyKindUuid
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
void VtkEpcDocument::createTreeVtk(const std::string & uuid, const std::string & parent, const std::string & name, const VtkEpcTools::Resqml2Type & type)
{
	uuidIsChildOf[uuid].myType = type;
	uuidIsChildOf[uuid].uuid = "UNKNOWN";
	uuidIsChildOf[uuid].parentType = uuidIsChildOf[parent].myType;
	uuidIsChildOf[uuid].parent = parent;
	uuidIsChildOf[uuid].name = name;
	uuidIsChildOf[uuid].timeIndex = 0;

	if (uuidIsChildOf[parent].uuid != "UNKNOWN")
	{
		uuidIsChildOf[uuid].uuid = uuid;
	switch (type)
	{
	case VtkEpcTools::Resqml2Type::GRID_2D:
	{
		addGrid2DTreeVtk(uuid, parent, name);
		break;
	}
	case VtkEpcTools::Resqml2Type::POLYLINE_SET:
	{
		addPolylineSetTreeVtk(uuid, parent, name);
		break;
	}
	case VtkEpcTools::Resqml2Type::TRIANGULATED_SET:
	{
		addTriangulatedSetTreeVtk(uuid, parent, name);
		break;
	}
	case VtkEpcTools::Resqml2Type::WELL_TRAJ:
	{
		addWellTrajTreeVtk(uuid, parent, name);
		break;
	}
	case VtkEpcTools::Resqml2Type::IJK_GRID:
	{
		addIjkGridTreeVtk(uuid, parent, name);
		break;
	}
	case VtkEpcTools::Resqml2Type::UNSTRUC_GRID:
	{
		addUnstrucGridTreeVtk(uuid, parent, name);
		break;
	}
	case VtkEpcTools::Resqml2Type::SUB_REP:
	{
		addSubRepTreeVtk(uuid, parent, name);
		break;
	}
	case VtkEpcTools::Resqml2Type::PROPERTY:
	{
		addPropertyTreeVtk(uuid, parent, name);
	}
	default:
		break;
	}
	uuidRep.push_back(uuid);
	}
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
	if (uuidIsChildOf[uuid].parentType == VtkEpcTools::Resqml2Type::IJK_GRID)
	{
		uuidToVtkIjkGridRepresentation[uuid] = new VtkIjkGridRepresentation(getFileName(), name, uuid, parent, epcPackage, epcPackage);
	}
	if (uuidIsChildOf[uuid].parentType == VtkEpcTools::Resqml2Type::UNSTRUC_GRID)
	{
		uuidToVtkUnstructuredGridRepresentation[uuid] = new VtkUnstructuredGridRepresentation(getFileName(), name, uuid, parent, epcPackage, epcPackage);
	}
	if (uuidIsChildOf[uuid].parentType == VtkEpcTools::Resqml2Type::PARTIAL)
	{
		auto parentUuidType = uuidToVtkPartialRepresentation[parent]->getType();
		auto pckEPCsrc = uuidToVtkPartialRepresentation[parent]->getEpcSource();

		switch (parentUuidType)
		{
		case VtkEpcTools::Resqml2Type::GRID_2D:
		{
			uuidIsChildOf[uuid].parentType = VtkEpcTools::Resqml2Type::GRID_2D;
			uuidToVtkGrid2DRepresentation[uuid] = new VtkGrid2DRepresentation(getFileName(), name, uuid, parent, pckEPCsrc, epcPackage);
			break;
		}
		case VtkEpcTools::Resqml2Type::WELL_TRAJ:
		{
			uuidIsChildOf[uuid].parentType = VtkEpcTools::Resqml2Type::WELL_TRAJ;
			uuidToVtkWellboreTrajectoryRepresentation[uuid] = new VtkWellboreTrajectoryRepresentation(getFileName(), name, uuid, parent, pckEPCsrc, epcPackage);
			break;
		}
		case VtkEpcTools::Resqml2Type::IJK_GRID:
		{
			uuidIsChildOf[uuid].parentType = VtkEpcTools::Resqml2Type::IJK_GRID;
			uuidToVtkIjkGridRepresentation[uuid] = new VtkIjkGridRepresentation(getFileName(), name, uuid, parent, pckEPCsrc, epcPackage);
			break;
		}
		case VtkEpcTools::Resqml2Type::UNSTRUC_GRID:
		{
			uuidIsChildOf[uuid].parentType = VtkEpcTools::Resqml2Type::UNSTRUC_GRID;
			uuidToVtkUnstructuredGridRepresentation[uuid] = new VtkUnstructuredGridRepresentation(getFileName(), name, uuid, parent, pckEPCsrc, epcPackage);
			break;
		}
		}
	}
}

//----------------------------------------------------------------------------
void VtkEpcDocument::addPropertyTreeVtk(const std::string & uuid, const std::string & parent, const std::string & name)
{
	switch (uuidIsChildOf[uuid].parentType)
	{
	case VtkEpcTools::GRID_2D:
	{
		uuidToVtkGrid2DRepresentation[parent]->createTreeVtk(uuid, parent, name, uuidIsChildOf[uuid].myType);
		break;
	}
	case VtkEpcTools::POLYLINE_SET:
	{
		auto object = epcPackage->getResqmlAbstractObjectByUuid(parent);
		auto typeRepresentation = object->getXmlTag();
		if (typeRepresentation == "PolylineRepresentation")
		{
			uuidToVtkPolylineRepresentation[parent]->createTreeVtk(uuid, parent, name, uuidIsChildOf[uuid].myType);
		}
		else
		{
			uuidToVtkSetPatch[parent]->createTreeVtk(uuid, parent, name, uuidIsChildOf[uuid].myType);
		}
		break;
	}
	case VtkEpcTools::TRIANGULATED_SET:
	{
		auto object = epcPackage->getResqmlAbstractObjectByUuid(uuid);
		auto typeRepresentation = object->getXmlTag();
		if (typeRepresentation == "TriangulatedRepresentation")
		{
			uuidToVtkTriangulatedRepresentation[parent]->createTreeVtk(uuid, parent, name, uuidIsChildOf[uuid].myType);
		}
		else
		{
			uuidToVtkSetPatch[parent]->createTreeVtk(uuid, parent, name, uuidIsChildOf[uuid].myType);
		}
		break;
	}
	case VtkEpcTools::WELL_TRAJ:
	{
		uuidToVtkWellboreTrajectoryRepresentation[parent]->createTreeVtk(uuid, parent, name, uuidIsChildOf[uuid].myType);
		break;
	}
	case VtkEpcTools::IJK_GRID:
	{
		uuidToVtkIjkGridRepresentation[parent]->createTreeVtk(uuid, parent, name, uuidIsChildOf[uuid].myType);
		break;
	}
	case VtkEpcTools::UNSTRUC_GRID:
	{
		uuidToVtkUnstructuredGridRepresentation[parent]->createTreeVtk(uuid, parent, name, uuidIsChildOf[uuid].myType);
		break;
	}
	case VtkEpcTools::SUB_REP:
	{
		if (uuidIsChildOf[parent].parentType == VtkEpcTools::Resqml2Type::IJK_GRID)
		{
			uuidToVtkIjkGridRepresentation[uuidIsChildOf[parent].uuid]->createTreeVtk(uuid, parent, name, uuidIsChildOf[uuid].myType);
		}
		if (uuidIsChildOf[parent].parentType == VtkEpcTools::Resqml2Type::UNSTRUC_GRID)
		{
			uuidToVtkUnstructuredGridRepresentation[uuidIsChildOf[parent].uuid]->createTreeVtk(uuid, parent, name, uuidIsChildOf[uuid].myType);
		}
		if (uuidIsChildOf[parent].parentType == VtkEpcTools::Resqml2Type::PARTIAL)
		{
			uuidToVtkPartialRepresentation[uuidIsChildOf[parent].uuid]->createTreeVtk(uuid, parent, name, uuidIsChildOf[uuid].myType);
		}
		break;
	}
	case VtkEpcTools::PARTIAL:
	{
		auto parentInfoUuid = uuidToVtkPartialRepresentation[parent]->getInfoUuid();
		auto pckEPCsrc = uuidToVtkPartialRepresentation[parent]->getEpcSource();

		switch (parentInfoUuid.myType)
		{
		case VtkEpcTools::GRID_2D:
		{
			uuidIsChildOf[uuid].parentType = VtkEpcTools::Resqml2Type::GRID_2D;
			if (!uuidToVtkGrid2DRepresentation[parentInfoUuid.uuid])
				uuidToVtkGrid2DRepresentation[parentInfoUuid.uuid] = new VtkGrid2DRepresentation(getFileName(), name, parentInfoUuid.uuid, parentInfoUuid.parent, pckEPCsrc, epcPackage);
			uuidToVtkGrid2DRepresentation[parent]->createTreeVtk(uuid, parentInfoUuid.uuid, name, uuidIsChildOf[uuid].myType);
			break;
		}
		case VtkEpcTools::WELL_TRAJ:
		{
			uuidIsChildOf[uuid].parentType = VtkEpcTools::Resqml2Type::WELL_TRAJ;
			if (!uuidToVtkWellboreTrajectoryRepresentation[parentInfoUuid.uuid])
				uuidToVtkWellboreTrajectoryRepresentation[parentInfoUuid.uuid] = new VtkWellboreTrajectoryRepresentation(getFileName(), name, parentInfoUuid.uuid, parentInfoUuid.parent, pckEPCsrc, epcPackage);
			uuidToVtkWellboreTrajectoryRepresentation[parent]->createTreeVtk(uuid, parentInfoUuid.uuid, name, uuidIsChildOf[uuid].myType);
			break;
		}
		case VtkEpcTools::IJK_GRID:
		{
			uuidIsChildOf[uuid].parentType = VtkEpcTools::Resqml2Type::IJK_GRID;
			if (!uuidToVtkIjkGridRepresentation[parentInfoUuid.uuid])
				uuidToVtkIjkGridRepresentation[parentInfoUuid.uuid] = new VtkIjkGridRepresentation(getFileName(), name, parentInfoUuid.uuid, parentInfoUuid.parent, pckEPCsrc, epcPackage);
			uuidToVtkIjkGridRepresentation[parent]->createTreeVtk(uuid, parentInfoUuid.uuid, name, uuidIsChildOf[uuid].myType);
			break;
		}
		case VtkEpcTools::UNSTRUC_GRID:
		{
			uuidIsChildOf[uuid].parentType = VtkEpcTools::Resqml2Type::UNSTRUC_GRID;
			if (!uuidToVtkUnstructuredGridRepresentation[parentInfoUuid.uuid])
				uuidToVtkUnstructuredGridRepresentation[parentInfoUuid.uuid] = new VtkUnstructuredGridRepresentation(getFileName(), name, parentInfoUuid.uuid, parentInfoUuid.parent, pckEPCsrc, epcPackage);
			uuidToVtkUnstructuredGridRepresentation[parent]->createTreeVtk(uuid, parentInfoUuid.uuid, name, uuidIsChildOf[uuid].myType);
			break;
		}
		case VtkEpcTools::SUB_REP:
		{
			if (parentInfoUuid.parentType == VtkEpcTools::Resqml2Type::IJK_GRID)
			{
				uuidIsChildOf[uuid].parentType = VtkEpcTools::Resqml2Type::IJK_GRID;
				if (!uuidToVtkIjkGridRepresentation[parentInfoUuid.uuid])
					uuidToVtkIjkGridRepresentation[parentInfoUuid.uuid] = new VtkIjkGridRepresentation(getFileName(), name, parentInfoUuid.uuid, parentInfoUuid.parent, pckEPCsrc, epcPackage);
				uuidToVtkIjkGridRepresentation[parent]->createTreeVtk(uuid, parentInfoUuid.uuid, name, uuidIsChildOf[uuid].myType);
			}
			if (parentInfoUuid.parentType == VtkEpcTools::Resqml2Type::UNSTRUC_GRID)
			{
				uuidIsChildOf[uuid].parentType = VtkEpcTools::Resqml2Type::UNSTRUC_GRID;
				if (!uuidToVtkUnstructuredGridRepresentation[parentInfoUuid.uuid])
					uuidToVtkUnstructuredGridRepresentation[parentInfoUuid.uuid] = new VtkUnstructuredGridRepresentation(getFileName(), name, parentInfoUuid.uuid, parentInfoUuid.parent, pckEPCsrc, epcPackage);
				uuidToVtkUnstructuredGridRepresentation[parent]->createTreeVtk(uuid, parentInfoUuid.uuid, name, uuidIsChildOf[uuid].myType);
			}
			if (parentInfoUuid.parentType == VtkEpcTools::Resqml2Type::GRID_2D)
			{
				uuidIsChildOf[uuid].parentType = VtkEpcTools::Resqml2Type::GRID_2D;
				if (!uuidToVtkGrid2DRepresentation[parentInfoUuid.uuid])
					uuidToVtkGrid2DRepresentation[parentInfoUuid.uuid] = new VtkGrid2DRepresentation(getFileName(), name, parentInfoUuid.uuid, parentInfoUuid.parent, pckEPCsrc, epcPackage);
				uuidToVtkGrid2DRepresentation[parent]->createTreeVtk(uuid, parentInfoUuid.uuid, name, uuidIsChildOf[uuid].myType);
			}
			if (parentInfoUuid.parentType == VtkEpcTools::Resqml2Type::WELL_TRAJ)
			{
				uuidIsChildOf[uuid].parentType = VtkEpcTools::Resqml2Type::WELL_TRAJ;
				if (!uuidToVtkWellboreTrajectoryRepresentation[parentInfoUuid.uuid])
					uuidToVtkWellboreTrajectoryRepresentation[parentInfoUuid.uuid] = new VtkWellboreTrajectoryRepresentation(getFileName(), name, parentInfoUuid.uuid, parentInfoUuid.parent, pckEPCsrc, epcPackage);
				uuidToVtkWellboreTrajectoryRepresentation[parent]->createTreeVtk(uuid, parentInfoUuid.uuid, name, uuidIsChildOf[uuid].myType);
			}
			break;
		}
		default:
		{
			cout << " parent type unknown " << uuidIsChildOf[uuid].parentType << "\n";
			break;
		}
		}
	}
	}
}



//----------------------------------------------------------------------------
void VtkEpcDocument::createTreeVtkPartialRep(const std::string & uuid, VtkEpcDocument *vtkEpcDowumentWithCompleteRep)
{
	uuidIsChildOf[uuid].myType = VtkEpcTools::Resqml2Type::PARTIAL;
	uuidIsChildOf[uuid].uuid = uuid;
	uuidToVtkPartialRepresentation[uuid] = new VtkPartialRepresentation(getFileName(), uuid, vtkEpcDowumentWithCompleteRep, epcPackage);
}

//----------------------------------------------------------------------------
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

}

//----------------------------------------------------------------------------
void VtkEpcDocument::visualize(const std::string & uuid)
{
	auto uuidToAttach = uuidIsChildOf[uuid].uuid;
	switch (uuidIsChildOf[uuid].myType)
	{
	case VtkEpcTools::GRID_2D:
	{
		uuidToVtkGrid2DRepresentation[uuidIsChildOf[uuid].uuid]->visualize(uuid);
		break;
	}
	case VtkEpcTools::POLYLINE_SET:
	{
		auto object = epcPackage->getResqmlAbstractObjectByUuid(uuidIsChildOf[uuid].uuid);
		auto typeRepresentation = object->getXmlTag();
		if (typeRepresentation == "PolylineRepresentation")
		{
			uuidToVtkPolylineRepresentation[uuidIsChildOf[uuid].uuid]->visualize(uuid);
		}
		else
		{
			uuidToVtkSetPatch[uuidIsChildOf[uuid].uuid]->visualize(uuid);
		}
		break;
	}
	case VtkEpcTools::TRIANGULATED_SET:
	{
		auto object = epcPackage->getResqmlAbstractObjectByUuid(uuidIsChildOf[uuid].uuid);
		auto typeRepresentation = object->getXmlTag();
		if (typeRepresentation == "TriangulatedRepresentation")
		{
			uuidToVtkTriangulatedRepresentation[uuidIsChildOf[uuid].uuid]->visualize(uuid);
		}
		else
		{
			uuidToVtkSetPatch[uuidIsChildOf[uuid].uuid]->visualize(uuid);
		}
		break;
	}
	case VtkEpcTools::WELL_TRAJ:
	{
		uuidToVtkWellboreTrajectoryRepresentation[uuidIsChildOf[uuid].uuid]->visualize(uuid);
		break;
	}
	case VtkEpcTools::IJK_GRID:
	{
		uuidToVtkIjkGridRepresentation[uuidIsChildOf[uuid].uuid]->visualize(uuid);
		break;
	}
	case VtkEpcTools::UNSTRUC_GRID:
	{
		uuidToVtkUnstructuredGridRepresentation[uuidIsChildOf[uuid].uuid]->visualize(uuid);
		break;
	}
	case VtkEpcTools::SUB_REP:
	{
		if (uuidIsChildOf[uuid].parentType == VtkEpcTools::Resqml2Type::IJK_GRID)
		{
			uuidToVtkIjkGridRepresentation[uuidIsChildOf[uuid].uuid]->visualize(uuid);
		}
		if (uuidIsChildOf[uuid].parentType == VtkEpcTools::Resqml2Type::UNSTRUC_GRID)
		{
			uuidToVtkUnstructuredGridRepresentation[uuidIsChildOf[uuid].uuid]->visualize(uuid);
		}
		if (uuidIsChildOf[uuid].parentType == VtkEpcTools::Resqml2Type::PARTIAL)
		{
			uuidToVtkPartialRepresentation[uuidIsChildOf[uuid].uuid]->visualize(uuid);
		}
		break;
	}
	case VtkEpcTools::PARTIAL:
	{
		uuidToVtkPartialRepresentation[uuidIsChildOf[uuid].uuid]->visualize(uuid);
		break;
	}
	case VtkEpcTools::PROPERTY:
	{
		uuidToAttach = uuidIsChildOf[uuid].parent;
		switch (uuidIsChildOf[uuid].parentType)
		{
		case VtkEpcTools::Resqml2Type::GRID_2D:
		{
			uuidToVtkGrid2DRepresentation[uuidIsChildOf[uuid].parent]->visualize(uuid);
			break;
		}
		case VtkEpcTools::POLYLINE_SET:
		{
			auto object = epcPackage->getResqmlAbstractObjectByUuid(uuidIsChildOf[uuid].parent);
			auto typeRepresentation = object->getXmlTag();
			if (typeRepresentation == "PolylineRepresentation")
			{
				uuidToVtkPolylineRepresentation[uuidIsChildOf[uuid].parent]->visualize(uuid);
			}
			else
			{
				uuidToVtkSetPatch[uuidIsChildOf[uuid].parent]->visualize(uuid);
			}
			break;
		}
		case VtkEpcTools::TRIANGULATED_SET:
		{
			auto object = epcPackage->getResqmlAbstractObjectByUuid(uuid);
			auto typeRepresentation = object->getXmlTag();
			if (typeRepresentation == "TriangulatedRepresentation")
			{
				uuidToVtkTriangulatedRepresentation[uuidIsChildOf[uuid].parent]->visualize(uuid);
			}
			else
			{
				uuidToVtkSetPatch[uuidIsChildOf[uuid].parent]->visualize(uuid);
			}
			break;
		}
		case VtkEpcTools::WELL_TRAJ:
		{
			uuidToVtkWellboreTrajectoryRepresentation[uuidIsChildOf[uuid].parent]->visualize(uuid);
			break;
		}
		case VtkEpcTools::IJK_GRID:
		{
			uuidToVtkIjkGridRepresentation[uuidIsChildOf[uuid].parent]->visualize(uuid);
			break;
		}
		case VtkEpcTools::UNSTRUC_GRID:
		{
			uuidToVtkUnstructuredGridRepresentation[uuidIsChildOf[uuid].parent]->visualize(uuid);
			break;
		}
		case VtkEpcTools::SUB_REP:
		{
			if (uuidIsChildOf[uuidIsChildOf[uuid].parent].parentType == VtkEpcTools::Resqml2Type::IJK_GRID)
			{
				uuidToVtkIjkGridRepresentation[uuidIsChildOf[uuidIsChildOf[uuid].parent].uuid]->visualize(uuid);
			}
			if (uuidIsChildOf[uuidIsChildOf[uuid].parent].parentType == VtkEpcTools::Resqml2Type::UNSTRUC_GRID)
			{
				uuidToVtkUnstructuredGridRepresentation[uuidIsChildOf[uuidIsChildOf[uuid].parent].uuid]->visualize(uuid);
			}
			if (uuidIsChildOf[uuidIsChildOf[uuid].parent].parentType == VtkEpcTools::Resqml2Type::PARTIAL)
			{
				uuidToVtkPartialRepresentation[uuidIsChildOf[uuidIsChildOf[uuid].parent].uuid]->visualize(uuid);
			}
			break;
		}
		case VtkEpcTools::PARTIAL:
		{
			uuidToVtkPartialRepresentation[uuidIsChildOf[uuid].uuid]->visualize(uuid);
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
	if (uuidIsChildOf[uuid].myType == VtkEpcTools::Resqml2Type::PROPERTY)
	{
		uuidtoAttach = uuidIsChildOf[uuid].parent;
	}

	if (std::find(attachUuids.begin(), attachUuids.end(), uuidtoAttach) != attachUuids.end())
	{
		switch (uuidIsChildOf[uuidtoAttach].myType)
		{
		case VtkEpcTools::Resqml2Type::GRID_2D:	{
			uuidToVtkGrid2DRepresentation[uuidtoAttach]->remove(uuid);
			if (uuid == uuidtoAttach){
				this->detach();
				attachUuids.erase(std::find(attachUuids.begin(), attachUuids.end(), uuid));
				this->attach();
			}
			break;
		}
		case VtkEpcTools::Resqml2Type::POLYLINE_SET:	{
			auto object = epcPackage->getResqmlAbstractObjectByUuid(uuidtoAttach);
			auto typeRepresentation = object->getXmlTag();
			if (typeRepresentation == "PolylineRepresentation") {
				uuidToVtkPolylineRepresentation[uuidIsChildOf[uuidtoAttach].uuid]->remove(uuid);
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
		case VtkEpcTools::Resqml2Type::TRIANGULATED_SET:	{
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
		case VtkEpcTools::Resqml2Type::WELL_TRAJ:	{
			uuidToVtkWellboreTrajectoryRepresentation[uuidtoAttach]->remove(uuid);
			if (uuid == uuidtoAttach){
				this->detach();
				attachUuids.erase(std::find(attachUuids.begin(), attachUuids.end(), uuid));
				this->attach();
			}
			break;
		}
		case VtkEpcTools::Resqml2Type::IJK_GRID:
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
		case VtkEpcTools::Resqml2Type::UNSTRUC_GRID:
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
		case VtkEpcTools::Resqml2Type::SUB_REP:
		{
			if (uuidIsChildOf[uuidIsChildOf[uuid].parent].parentType == VtkEpcTools::Resqml2Type::IJK_GRID)
			{
				uuidToVtkIjkGridRepresentation[uuidIsChildOf[uuidIsChildOf[uuid].parent].uuid]->remove(uuid);
			}
			if (uuidIsChildOf[uuidIsChildOf[uuid].parent].parentType == VtkEpcTools::Resqml2Type::UNSTRUC_GRID)
			{
				uuidToVtkUnstructuredGridRepresentation[uuidIsChildOf[uuidIsChildOf[uuid].parent].uuid]->remove(uuid);
			}
			if (uuidIsChildOf[uuidIsChildOf[uuid].parent].parentType == VtkEpcTools::Resqml2Type::PARTIAL)
			{
				uuidToVtkPartialRepresentation[uuidIsChildOf[uuidIsChildOf[uuid].parent].uuid]->remove(uuid);
			}
			if (uuid == uuidtoAttach)
			{
				this->detach();
				attachUuids.erase(std::find(attachUuids.begin(), attachUuids.end(), uuid));
				this->attach();
			}
			break;
		}
		case VtkEpcTools::Resqml2Type::PARTIAL:
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
		if (uuidIsChildOf[uuid].myType == VtkEpcTools::GRID_2D)
		{
			vtkOutput->SetBlock(newBlockIndex, uuidToVtkGrid2DRepresentation[uuid]->getOutput());
			vtkOutput->GetMetaData(newBlockIndex)->Set(vtkCompositeDataSet::NAME(), uuidToVtkGrid2DRepresentation[uuid]->getUuid().c_str());
		}
		if (uuidIsChildOf[uuid].myType == VtkEpcTools::POLYLINE_SET)
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
		if (uuidIsChildOf[uuid].myType == VtkEpcTools::TRIANGULATED_SET)
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
		if (uuidIsChildOf[uuid].myType == VtkEpcTools::WELL_TRAJ)
		{
			vtkOutput->SetBlock(newBlockIndex, uuidToVtkWellboreTrajectoryRepresentation[uuid]->getOutput());
			vtkOutput->GetMetaData(newBlockIndex)->Set(vtkCompositeDataSet::NAME(), uuidToVtkWellboreTrajectoryRepresentation[uuid]->getUuid().c_str());
		}
		if (uuidIsChildOf[uuid].myType == VtkEpcTools::IJK_GRID)
		{
			vtkOutput->SetBlock(newBlockIndex, uuidToVtkIjkGridRepresentation[uuid]->getOutput());
			vtkOutput->GetMetaData(newBlockIndex)->Set(vtkCompositeDataSet::NAME(), uuidToVtkIjkGridRepresentation[uuid]->getUuid().c_str());
		}
		if (uuidIsChildOf[uuid].myType == VtkEpcTools::UNSTRUC_GRID)
		{
			vtkOutput->SetBlock(newBlockIndex, uuidToVtkUnstructuredGridRepresentation[uuid]->getOutput());
			vtkOutput->GetMetaData(newBlockIndex)->Set(vtkCompositeDataSet::NAME(), uuidToVtkUnstructuredGridRepresentation[uuid]->getUuid().c_str());
		}
		if (uuidIsChildOf[uuid].myType == VtkEpcTools::SUB_REP)
		{
			if (uuidIsChildOf[uuid].parentType == VtkEpcTools::IJK_GRID)
			{
				vtkOutput->SetBlock(newBlockIndex, uuidToVtkIjkGridRepresentation[uuidIsChildOf[uuid].uuid]->getOutput());
				vtkOutput->GetMetaData(newBlockIndex)->Set(vtkCompositeDataSet::NAME(), uuidToVtkIjkGridRepresentation[uuid]->getUuid().c_str());
			}
			if (uuidIsChildOf[uuid].parentType == VtkEpcTools::UNSTRUC_GRID)
			{
				vtkOutput->SetBlock(newBlockIndex, uuidToVtkUnstructuredGridRepresentation[uuidIsChildOf[uuid].uuid]->getOutput());
				vtkOutput->GetMetaData(newBlockIndex)->Set(vtkCompositeDataSet::NAME(), uuidToVtkUnstructuredGridRepresentation[uuidIsChildOf[uuid].uuid]->getUuid().c_str());
			}
			if (uuidIsChildOf[uuid].parentType == VtkEpcTools::PARTIAL)
			{
				auto parentUuidType = uuidToVtkPartialRepresentation[uuidIsChildOf[uuid].parent]->getType();

				switch (parentUuidType)
				{
				case VtkEpcTools::GRID_2D:
				{
					vtkOutput->SetBlock(newBlockIndex, uuidToVtkGrid2DRepresentation[uuid]->getOutput());
					vtkOutput->GetMetaData(newBlockIndex)->Set(vtkCompositeDataSet::NAME(), uuidToVtkGrid2DRepresentation[uuid]->getUuid().c_str());
					break;
				}
				case VtkEpcTools::WELL_TRAJ:
				{
					vtkOutput->SetBlock(newBlockIndex, uuidToVtkWellboreTrajectoryRepresentation[uuid]->getOutput());
					vtkOutput->GetMetaData(newBlockIndex)->Set(vtkCompositeDataSet::NAME(), uuidToVtkWellboreTrajectoryRepresentation[uuid]->getUuid().c_str());
					break;
				}
				case VtkEpcTools::IJK_GRID:
				{
					vtkOutput->SetBlock(newBlockIndex, uuidToVtkIjkGridRepresentation[uuidIsChildOf[uuid].uuid]->getOutput());
					vtkOutput->GetMetaData(newBlockIndex)->Set(vtkCompositeDataSet::NAME(), uuidToVtkIjkGridRepresentation[uuid]->getUuid().c_str());
					break;
				}
				case VtkEpcTools::UNSTRUC_GRID:
				{
					vtkOutput->SetBlock(newBlockIndex, uuidToVtkUnstructuredGridRepresentation[uuidIsChildOf[uuid].uuid]->getOutput());
					vtkOutput->GetMetaData(newBlockIndex)->Set(vtkCompositeDataSet::NAME(), uuidToVtkUnstructuredGridRepresentation[uuidIsChildOf[uuid].uuid]->getUuid().c_str());
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
	switch (uuidIsChildOf[uuidProperty].myType)
	{
	case VtkEpcTools::GRID_2D:	{
		uuidToVtkGrid2DRepresentation[uuidIsChildOf[uuidProperty].uuid]->addProperty(uuidProperty, dataProperty);
		break;
	}
	case VtkEpcTools::POLYLINE_SET:	{
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
	case VtkEpcTools::TRIANGULATED_SET:	{
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
	case VtkEpcTools::WELL_TRAJ:	{
		uuidToVtkWellboreTrajectoryRepresentation[uuidIsChildOf[uuidProperty].uuid]->addProperty(uuidProperty, dataProperty);
		break;
	}
	case VtkEpcTools::IJK_GRID:
		uuidToVtkIjkGridRepresentation[uuidIsChildOf[uuidProperty].uuid]->addProperty(uuidProperty, dataProperty);
		break;
	case VtkEpcTools::UNSTRUC_GRID:
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

long VtkEpcDocument::getAttachmentPropertyCount(const std::string & uuid, const VtkEpcTools::FesppAttachmentProperty propertyUnit)
{
	long result = 0;
	switch (uuidIsChildOf[uuid].myType)
	{
	case VtkEpcTools::Resqml2Type::IJK_GRID:
		result = uuidToVtkIjkGridRepresentation[uuidIsChildOf[uuid].uuid]->getAttachmentPropertyCount(uuid, propertyUnit);
		break;
	case VtkEpcTools::Resqml2Type::UNSTRUC_GRID:
		result = uuidToVtkUnstructuredGridRepresentation[uuidIsChildOf[uuid].uuid]->getAttachmentPropertyCount(uuid, propertyUnit);
		break;
	default:
		break;
	}
	return result;
}

//----------------------------------------------------------------------------
VtkEpcTools::Resqml2Type VtkEpcDocument::getType(std::string uuid)
{
	return uuidIsChildOf[uuid].myType;
}

//----------------------------------------------------------------------------
VtkEpcTools::infoUuid VtkEpcDocument::getInfoUuid(std::string uuid)
{
	return uuidIsChildOf[uuid];
}

//----------------------------------------------------------------------------
common::EpcDocument * VtkEpcDocument::getEpcDocument()
{
	return epcPackage;
}

//----------------------------------------------------------------------------
int VtkEpcDocument::getCountPartialUuid()
{
	return uuidPartialRep.size();
}

//----------------------------------------------------------------------------
std::vector<std::string> VtkEpcDocument::getPartialUuid()
{
	return uuidPartialRep;
}
//----------------------------------------------------------------------------
int VtkEpcDocument::getCountUuid()
{
	return uuidRep.size();

}

//----------------------------------------------------------------------------
std::vector<std::string> VtkEpcDocument::getUuid()
{
	return uuidRep;
}

std::string VtkEpcDocument::getFullUuid(std::string uuidPartial)
{
	return uuidPartial;
}

void VtkEpcDocument::addPartialUUid(const std::string & uuid, VtkEpcDocument *vtkEpcDowumentWithCompleteRep)
{
	uuidIsChildOf[uuid].myType = VtkEpcTools::Resqml2Type::PARTIAL;
	uuidIsChildOf[uuid].uuid = uuid;
	uuidToVtkPartialRepresentation[uuid] = new VtkPartialRepresentation(getFileName(), uuid, vtkEpcDowumentWithCompleteRep, epcPackage);
}

//----------------------------------------------------------------------------
std::vector<VtkEpcTools::infoUuid> VtkEpcDocument::getTreeView() const
{
	return treeView;
}
