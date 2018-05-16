#include "PQSelectionPanel.h"
#include "ui_PQSelectionPanel.h"

// include API Resqml2
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

// include Qt
#include <QtGui>
#include <QFileInfo>
#include <QIcon>
#include <qtreewidget.h>
#include <QHeaderView>
#include <QMenu>
#include <QProgressDialog>
#include <QList>
#include <qmessagebox.h>
#include <qprogressbar.h>
#include <qlayout.h>

// include ParaView
#include <PQToolsManager.h>
#include <pqPropertiesPanel.h>
#include <qobject.h>
#include <pqView.h>
#include <pqPipelineSource.h>
#include <pqApplicationCore.h>
#include <pqServerManagerModel.h>
#include <pqServer.h>
#include <pqOutputPort.h>
#include <pqActiveObjects.h>
#include <pqOutputPort.h>
#include <pqCoreUtilities.h>

// include VTK
#include <vtkSMProxy.h>
#include <vtkSMIntVectorProperty.h>
#include <vtkSMSessionProxyManager.h>
#include <vtkSMPropertyHelper.h>
#include <vtkSMStringVectorProperty.h>
#include <vtkInformation.h>

// include system
#include <algorithm>
#include <sstream>
#include <stdexcept>

namespace
{
pqPropertiesPanel* getpqPropertiesPanel()
{
	// get multi-block inspector panel
	pqPropertiesPanel *panel = 0;
	foreach(QWidget *widget, qApp->topLevelWidgets())
	{
		panel = widget->findChild<pqPropertiesPanel *>();

		if (panel)
		{
			break;
		}
	}
	return panel;
}


}

//----------------------------------------------------------------------------
void PQSelectionPanel::constructor()
{
	setWindowTitle("Selection widget");
	QWidget* t_widget = new QWidget(this);
	Ui::panelSelection ui;
	ui.setupUi(t_widget);
	setWidget(t_widget);
	treeWidget = ui.treeWidget;

	treeWidget->setStyleSheet("QWidget::branch:has-siblings:!adjoins-item{border-image: url(:vline.png) 0;}"
			"QWidget::branch:has-siblings:adjoins-item{border-image: url(:branch-more.png) 0;}"
			"QWidget::branch:!has-children:!has-siblings:adjoins-item{border-image: url(:branch-end.png) 0;}"
			"QWidget::branch:has-children:!has-siblings:closed,QTreeView::branch:closed:has-children:has-siblings{border-image: none; image: url(:branch-closed.png);}"
			"QWidget::branch:open:has-children:!has-siblings,QTreeView::branch:open:has-children:has-siblings{border-image: none; image: url(:branch-open.png);}");


	treeWidget->header()->setStretchLastSection(false);
	treeWidget->header()->resizeSection(1, 20);
	treeWidget->header()->setSectionResizeMode(0, QHeaderView::Stretch);
	treeWidget->expandToDepth(0);
	treeWidget->header()->close();

	connect(ui.treeWidget, SIGNAL(itemClicked(QTreeWidgetItem*, int)), this, SLOT(clicSelection(QTreeWidgetItem*, int)));
	connect(ui.treeWidget, SIGNAL(itemChanged(QTreeWidgetItem*, int)), this, SLOT(onItemCheckedUnchecked(QTreeWidgetItem*, int)));

	radioButtonCount = 0;

	indexFile = 0;

	addTreeRoot("root", "epcdocument");
}

//******************************* ACTIONS ************************************

//----------------------------------------------------------------------------
void PQSelectionPanel::clicSelection(QTreeWidgetItem* item, int column)
{
	unsigned int cpt;
	pickedBlocks = itemUuid[item];
	if (pickedBlocks!="root")
	{
		QFileInfo file(uuidToFilename[pickedBlocks].c_str());
		pqPipelineSource * source = findPipelineSource("EpcDocument");
		if (source)
		{
			pqActiveObjects *activeObjects = &pqActiveObjects::instance();
			activeObjects->setActiveSource(source);
		}
		if (!(uuidToFilename[pickedBlocks] == pickedBlocks))
		{
			emit selectionName(uuidToFilename[pickedBlocks], pickedBlocks, pcksave[uuidToFilename[pickedBlocks]]);
		}
	}
}

//----------------------------------------------------------------------------
void PQSelectionPanel::onItemCheckedUnchecked(QTreeWidgetItem * item, int column)
{
	std::string uuid = itemUuid[item];
	if (!(uuid == ""))
	{
		if (column == 0)
		{
			if (item->checkState(0) == Qt::Checked)
			{
				uuidVisible.push_back(uuid);
				this->loadUuid(uuid);
			}
			else
			{
				bool removeOK = false;
				// Property exist
				if (uuidItem[uuid]->childCount() > 0 && mapUuidWithProperty[uuid] != "")
				{
					std::string uuidOld = mapUuidWithProperty[uuid];
					this->removeUuid(uuidOld);

					mapUuidWithProperty[uuid] = "";
					mapUuidParentButtonInvisible[uuid]->setChecked(true);
					uuidItem[uuid]->setData(0, Qt::CheckStateRole, QVariant());
				}

				std::vector<std::string>::iterator it_Visible = std::find(uuidVisible.begin(), uuidVisible.end(), uuid);
				if (it_Visible != uuidVisible.end())
				{
					removeOK = true;
					uuidVisible.erase(std::find(uuidVisible.begin(), uuidVisible.end(), uuid));
					this->removeUuid(uuid);
				}
			}
		}
	}
}

//----------------------------------------------------------------------------
void PQSelectionPanel::deleteTreeView()
{
	cout << "void PQSelectionPanel::deleteTreeView()· IN\n";
	treeWidget->clear();

	indexFile = 0;
	radioButtonCount = 0;
	allFileName.clear();
	uuidToFilename.clear();
	filenameToUuids.clear();
	filenameToUuidsPartial.clear();

	uuidItem.clear();
	itemUuid.clear();
	uuidParentItem.clear();

	mapUuidProperty.clear();
	mapUuidWithProperty.clear();

	displayUuid.clear();

	mapRadioButtonNo.clear();

	mapUuidParentButtonInvisible.clear();

	std::string pickedBlocks = "";
	uuidVisible.clear();

	uuidCheckable.clear();

	pcksave.clear();
	uuidParentGroupButton.clear();
	radioButtonToUuid.clear();

	addTreeRoot("root", "epcdocument");
	cout << "void PQSelectionPanel::deleteTreeView()· OUT\n";

}

//----------------------------------------------------------------------------
void PQSelectionPanel::checkedRadioButton(int rbNo)
{
	if (mapRadioButtonNo.contains(rbNo))
	{
		std::string uuid = mapRadioButtonNo[rbNo];
		std::string uuidParent = itemUuid[uuidParentItem[uuid]];
		uuidParentItem[uuid]->setCheckState(0, Qt::Checked);

		std::string uuidOld = mapUuidWithProperty[uuidParent];
		if (!(uuidOld == ""))
		{
			this->removeUuid(uuidOld);
		}

		mapUuidWithProperty[uuidParent] = uuid;
		this->loadUuid(uuid);
	}
}

//****************************************************************************

//********************************* TreeView *********************************

bool PQSelectionPanel::canAddFile(const char* fileName)
{

	std::vector<std::string>::iterator it_present = std::find(allFileName.begin(), allFileName.end(), std::string(fileName));
	if (it_present != allFileName.end())
		return false;

	return true;
}
//----------------------------------------------------------------------------
void PQSelectionPanel::addFileName(const std::string & fileName)
{
	if (std::find(allFileName.begin(), allFileName.end(),fileName)==allFileName.end())
	{
		common::EpcDocument *pck = nullptr;
		try{
			pck = new common::EpcDocument(fileName, common::EpcDocument::READ_ONLY);
		}
		catch (const std::exception & e)
		{
			cout << "EXCEPTION in fesapi when reading file: " << fileName << " : " << e.what();
		}
		std::string result = "";
		try{
			result = pck->deserialize();
		}
		catch (const std::exception & e)
		{
			cout << "EXCEPTION in fesapi when deserialize file: " << fileName << " : " << e.what();
		}
		if (result.empty())
		{
			allFileName.push_back(fileName);
			uuidToFilename[fileName] = fileName;

			pcksave[fileName] = pck;

			// add treeView polylines representation
			addTreePolylines(fileName, pck->getFaultPolylineSetRepSet());
			addTreePolylines(fileName, pck->getHorizonPolylineSetRepSet());

			// add treeView triangulated representation
			addTreeTriangulated(fileName, pck->getAllTriangulatedSetRepSet());

			// add treeView Grid2D representation
			addTreeGrid2D(fileName, pck->getHorizonGrid2dRepSet());

			// add treeView ijkGrid representation
			addTreeIjkGrid(fileName, pck->getIjkGridRepresentationSet());

			// add treeView UnstrucutredGrid representation
			addTreeUnstructuredGrid(fileName, pck->getUnstructuredGridRepresentationSet());

			// add treeView UnstrucutredGrid representation
			addTreeWellboreTrajectory(fileName, pck->getWellboreTrajectoryRepresentationSet());

			// add treeView Sub-representation
			addTreeSubRepresentation(fileName, pck->getSubRepresentationSet());

			++indexFile;

		}
		else
		{
			try{
				pck->close();
			}
			catch (const std::exception & e)
			{
				cout << "EXCEPTION in fesapi when closing file " << fileName << " : " << e.what();
			}
		}
	}
}

//----------------------------------------------------------------------------
void PQSelectionPanel::addTreeRoot(QString name, QString description)
{
	QTreeWidgetItem *treeItem = new QTreeWidgetItem(treeWidget);

	treeItem->setExpanded(true);
	treeItem->setText(0, name);
	treeItem->setText(1, "");
	uuidItem[name.toStdString()] = treeItem;
	itemUuid[treeItem] = name.toStdString();
}

//----------------------------------------------------------------------------
void PQSelectionPanel::addTreeChild(QTreeWidgetItem *parent,
		QString name, std::string uuid)
{
	std::stringstream sstm;
	sstm << indexFile;
	QTreeWidgetItem *treeItem = new QTreeWidgetItem();
	treeItem->setExpanded(true);
	treeItem->setText(0, name);
	treeItem->setFlags(treeItem->flags() | Qt::ItemIsSelectable);

	parent->addChild(treeItem);

	uuidItem[uuid] = treeItem;
	itemUuid[treeItem] = uuid;
	filenameToUuids[uuidToFilename[uuid]].push_back(uuid);
}

//----------------------------------------------------------------------------
std::string PQSelectionPanel::addFeatInterp(const std::string & fileName, resqml2::AbstractFeatureInterpretation * interpretation)
{

	std::string uuidInterpretation;
	if (interpretation)
	{
		uuidInterpretation = interpretation->getUuid();

		if (uuidItem.count(uuidInterpretation) < 1)
		{
			resqml2::AbstractFeature * feature = interpretation->getInterpretedFeature();
			std::string uuidFeature = feature->getUuid();
			if (uuidItem.count(uuidFeature) < 1)
			{
				// add Feature
				uuidToFilename[uuidFeature] = fileName;
				addTreeChild(uuidItem["root"],
						feature->getTitle().c_str(),
						uuidFeature
				);
			}

			// add Interpretation
			if (uuidItem.contains(uuidFeature)) {
				uuidToFilename[uuidInterpretation] = fileName;

				addTreeChild(uuidItem[uuidFeature],
						interpretation->getTitle().c_str(),
						uuidInterpretation
				);
			}
		}
		return uuidInterpretation;
	}
	else
	{
		return fileName;
	}
}

//----------------------------------------------------------------------------
void PQSelectionPanel::addTreePolylines(const std::string & fileName, std::vector<resqml2_0_1::PolylineSetRepresentation*> polylines)
{
	QIcon icon;
	icon.addFile(QString::fromUtf8(":Polyline.png"), QSize(), QIcon::Normal, QIcon::Off);
	for (size_t polylineIter = 0; polylineIter < polylines.size(); ++polylineIter)
	{
		bool propertyTreeView = false;

		std::string uuidParent = addFeatInterp(fileName, polylines[polylineIter]->getInterpretation());
		if (polylines[polylineIter]->isPartial()) {
			if (uuidItem.count(polylines[polylineIter]->getUuid())>0){
				filenameToUuidsPartial[fileName].push_back(polylines[polylineIter]->getUuid());
				propertyTreeView = true;
			}
			else
			{
				QMessageBox msgBox;
				msgBox.setIcon(QMessageBox::Information);
				msgBox.setText(QString(("Partial UUID: " + polylines[polylineIter]->getUuid() + " and the complete UUID not found.").c_str()));
				msgBox.exec();
			}
		}
		else {
			uuidToFilename[polylines[polylineIter]->getUuid()] = fileName;
			addTreeRepresentation(uuidItem[uuidParent],
					polylines[polylineIter]->getTitle().c_str(),
					polylines[polylineIter]->getUuid(),
					icon
			);
			propertyTreeView = true;
		}
		if (propertyTreeView){
			std::vector<resqml2::AbstractValuesProperty*> valuesPropertySet = polylines[polylineIter]->getValuesPropertySet();
			addTreeProperty(uuidItem[polylines[polylineIter]->getUuid()], valuesPropertySet);
		}
	}
}

//----------------------------------------------------------------------------
void PQSelectionPanel::addTreeTriangulated(const std::string & fileName, std::vector<resqml2_0_1::TriangulatedSetRepresentation*> triangulated)
{
	QIcon icon;
	icon.addFile(QString::fromUtf8(":Triangulated.png"), QSize(), QIcon::Normal, QIcon::Off);
	for (size_t triangulatedIter = 0; triangulatedIter < triangulated.size(); ++triangulatedIter)
	{
		bool propertyTreeView = false;

		std::string uuidParent = addFeatInterp(fileName, triangulated[triangulatedIter]->getInterpretation());
		if (triangulated[triangulatedIter]->isPartial()){
			if (uuidItem.count(triangulated[triangulatedIter]->getUuid())>0){
				filenameToUuidsPartial[fileName].push_back(triangulated[triangulatedIter]->getUuid());

				propertyTreeView = true;
			}
			else
			{
				QMessageBox msgBox;
				msgBox.setIcon(QMessageBox::Information);
				msgBox.setText(QString(("Partial UUID: " + triangulated[triangulatedIter]->getUuid() + " and the complete UUID not found.").c_str()));
				msgBox.exec();
			}
		}
		else{
			uuidToFilename[triangulated[triangulatedIter]->getUuid()] = fileName;

			addTreeRepresentation(uuidItem[uuidParent],
					triangulated[triangulatedIter]->getTitle().c_str(),
					triangulated[triangulatedIter]->getUuid(),
					icon
			);
			propertyTreeView = true;
		}
		if (propertyTreeView){
			std::vector<resqml2::AbstractValuesProperty*> valuesPropertySet = triangulated[triangulatedIter]->getValuesPropertySet();
			addTreeProperty(uuidItem[triangulated[triangulatedIter]->getUuid()], valuesPropertySet);
		}
	}
}

//----------------------------------------------------------------------------
void PQSelectionPanel::addTreeGrid2D(const std::string & fileName, std::vector<resqml2_0_1::Grid2dRepresentation*> grid2D)
{
	QIcon icon;
	icon.addFile(QString::fromUtf8(":Grid2D.png"), QSize(), QIcon::Normal, QIcon::Off);
	for (size_t grid2DIter = 0; grid2DIter < grid2D.size(); ++grid2DIter)
	{
		bool propertyTreeView = false;

		std::string uuidParent = addFeatInterp(fileName, grid2D[grid2DIter]->getInterpretation());
		if (grid2D[grid2DIter]->isPartial()){
			if (uuidItem.count(grid2D[grid2DIter]->getUuid())>0){
				filenameToUuidsPartial[fileName].push_back(grid2D[grid2DIter]->getUuid());

				propertyTreeView = true;
			}
			else{
				QMessageBox msgBox;
				msgBox.setIcon(QMessageBox::Information);
				msgBox.setText(QString(("Partial UUID: " + grid2D[grid2DIter]->getUuid() + " and the complete UUID not found.").c_str()));
				msgBox.exec();
			}
		}
		else{
			uuidToFilename[grid2D[grid2DIter]->getUuid()] = fileName;

			addTreeRepresentation(uuidItem[uuidParent],
					grid2D[grid2DIter]->getTitle().c_str(),
					grid2D[grid2DIter]->getUuid(),
					icon
			);
			propertyTreeView = true;
		}
		if (propertyTreeView){
			std::vector<resqml2::AbstractValuesProperty*> valuesPropertySet = grid2D[grid2DIter]->getValuesPropertySet();
			addTreeProperty(uuidItem[grid2D[grid2DIter]->getUuid()], valuesPropertySet);
		}
	}
}

//----------------------------------------------------------------------------
void PQSelectionPanel::addTreeIjkGrid(const std::string & fileName, std::vector<resqml2_0_1::AbstractIjkGridRepresentation*> ijkGrid)
{
	QIcon icon;
	icon.addFile(QString::fromUtf8(":IjkGrid.png"), QSize(), QIcon::Normal, QIcon::Off);
	for (size_t ijkGridIter = 0; ijkGridIter < ijkGrid.size(); ++ijkGridIter)
	{
		bool propertyTreeView = false;
		if (ijkGrid[ijkGridIter]->getGeometryKind() != resqml2_0_1::AbstractIjkGridRepresentation::NO_GEOMETRY)
		{
			std::string uuidParent = addFeatInterp(fileName, ijkGrid[ijkGridIter]->getInterpretation());
			if (ijkGrid[ijkGridIter]->isPartial())
			{
				if (uuidItem.count(ijkGrid[ijkGridIter]->getUuid())>0)
				{
					filenameToUuidsPartial[fileName].push_back(ijkGrid[ijkGridIter]->getUuid());
					propertyTreeView = true;
				}
				else
				{
					QMessageBox msgBox;
					msgBox.setIcon(QMessageBox::Information);
					msgBox.setText(QString(("Partial UUID: " + ijkGrid[ijkGridIter]->getUuid() + " and the complete UUID not found.").c_str()));
					msgBox.exec();
				}
			}
			else
			{
				uuidToFilename[ijkGrid[ijkGridIter]->getUuid()] = fileName;
				addTreeRepresentation(uuidItem[uuidParent],
						ijkGrid[ijkGridIter]->getTitle().c_str(),
						ijkGrid[ijkGridIter]->getUuid(),
						icon
				);
				propertyTreeView = true;
			}
			if (propertyTreeView)
			{
				std::vector<resqml2::AbstractValuesProperty*> valuesPropertySet = ijkGrid[ijkGridIter]->getValuesPropertySet();
				addTreeProperty(uuidItem[ijkGrid[ijkGridIter]->getUuid()], valuesPropertySet);
			}
		}
	}
}

//----------------------------------------------------------------------------
void PQSelectionPanel::addTreeUnstructuredGrid(const std::string & fileName, std::vector<resqml2_0_1::UnstructuredGridRepresentation*> unstructuredGrid)
{
	QIcon icon;
	icon.addFile(QString::fromUtf8(":UnstructuredGrid.png"), QSize(), QIcon::Normal, QIcon::Off);
	for (size_t unstructuredGridIter = 0; unstructuredGridIter < unstructuredGrid.size(); ++unstructuredGridIter)
	{
		bool propertyTreeView = false;

		std::string uuidParent = addFeatInterp(fileName, unstructuredGrid[unstructuredGridIter]->getInterpretation());
		if (unstructuredGrid[unstructuredGridIter]->isPartial()){
			if (uuidItem.count(unstructuredGrid[unstructuredGridIter]->getUuid())>0){
				filenameToUuidsPartial[fileName].push_back(unstructuredGrid[unstructuredGridIter]->getUuid());

				propertyTreeView = true;
			}
			else{
				QMessageBox msgBox;
				msgBox.setIcon(QMessageBox::Information);
				msgBox.setText(QString(("Partial UUID: " + unstructuredGrid[unstructuredGridIter]->getUuid() + " and the complete UUID not found.").c_str()));
				msgBox.exec();
			}
		}
		else{
			uuidToFilename[unstructuredGrid[unstructuredGridIter]->getUuid()] = fileName;

			addTreeRepresentation(uuidItem[uuidParent],
					unstructuredGrid[unstructuredGridIter]->getTitle().c_str(),
					unstructuredGrid[unstructuredGridIter]->getUuid(),
					icon
			);
			propertyTreeView = true;
		}
		if (propertyTreeView){
			std::vector<resqml2::AbstractValuesProperty*> valuesPropertySet = unstructuredGrid[unstructuredGridIter]->getValuesPropertySet();
			addTreeProperty(uuidItem[unstructuredGrid[unstructuredGridIter]->getUuid()], valuesPropertySet);
		}
	}
}

//----------------------------------------------------------------------------
void PQSelectionPanel::addTreeWellboreTrajectory(const std::string & fileName, std::vector<resqml2_0_1::WellboreTrajectoryRepresentation*> WellboreTrajectory)
{
	QIcon icon;
	icon.addFile(QString::fromUtf8(":WellTraj.png"), QSize(), QIcon::Normal, QIcon::Off);
	for (size_t WellboreTrajectoryIter = 0; WellboreTrajectoryIter < WellboreTrajectory.size(); ++WellboreTrajectoryIter)
	{
		bool propertyTreeView = false;

		std::string uuidParent = addFeatInterp(fileName, WellboreTrajectory[WellboreTrajectoryIter]->getInterpretation());
		if (WellboreTrajectory[WellboreTrajectoryIter]->isPartial()){
			if (uuidItem.count(WellboreTrajectory[WellboreTrajectoryIter]->getUuid())>0){
				filenameToUuidsPartial[fileName].push_back(WellboreTrajectory[WellboreTrajectoryIter]->getUuid());

				propertyTreeView = true;
			}
			else{
				QMessageBox msgBox;
				msgBox.setIcon(QMessageBox::Information);
				msgBox.setText(QString(("Partial UUID: " + WellboreTrajectory[WellboreTrajectoryIter]->getUuid() + " and the complete UUID not found.").c_str()));
				msgBox.exec();
			}
		}
		else{
			uuidToFilename[WellboreTrajectory[WellboreTrajectoryIter]->getUuid()] = fileName;

			addTreeRepresentation(uuidItem[uuidParent],
					WellboreTrajectory[WellboreTrajectoryIter]->getTitle().c_str(),
					WellboreTrajectory[WellboreTrajectoryIter]->getUuid(),
					icon
			);
			propertyTreeView = true;
		}
		if (propertyTreeView){
			std::vector<resqml2::AbstractValuesProperty*> valuesPropertySet = WellboreTrajectory[WellboreTrajectoryIter]->getValuesPropertySet();
			addTreeProperty(uuidItem[WellboreTrajectory[WellboreTrajectoryIter]->getUuid()], valuesPropertySet);
		}
	}
}

//----------------------------------------------------------------------------
void PQSelectionPanel::addTreeSubRepresentation(const std::string & fileName, std::vector<resqml2::SubRepresentation*> subRepresentation)
{
	QIcon icon;
	icon.addFile(QString::fromUtf8(":SubRepresentation.png"), QSize(), QIcon::Normal, QIcon::Off);
	for (auto subRepresentationIter = 0; subRepresentationIter < subRepresentation.size(); ++subRepresentationIter)
	{
		auto propertyTreeView = false;

		auto uuidParent = subRepresentation[subRepresentationIter]->getSupportingRepresentationUuid(0);
		if (uuidItem.count(uuidParent)>0)
		{
			uuidToFilename[subRepresentation[subRepresentationIter]->getUuid()] = fileName;

			addTreeRepresentation(uuidItem[uuidParent],
					subRepresentation[subRepresentationIter]->getTitle().c_str(),
					subRepresentation[subRepresentationIter]->getUuid(),
					icon
			);
			propertyTreeView = true;
		}
		else{
			QMessageBox msgBox;
			msgBox.setIcon(QMessageBox::Information);
			msgBox.setText(QString(("supporting representation UUID: " + uuidParent + " not found.").c_str()));
			msgBox.exec();
		}
		if (propertyTreeView){
			std::vector<resqml2::AbstractValuesProperty*> valuesPropertySet = subRepresentation[subRepresentationIter]->getValuesPropertySet();
			addTreeProperty(uuidItem[subRepresentation[subRepresentationIter]->getUuid()], valuesPropertySet);
		}
	}
}

//----------------------------------------------------------------------------
void PQSelectionPanel::addTreeRepresentation(QTreeWidgetItem *parent,
		QString name, std::string uuid, QIcon icon)
{
	if (uuidItem.count(uuid) < 1)
	{
		QTreeWidgetItem *treeItem = new QTreeWidgetItem();
		treeItem->setText(0, name);
		treeItem->setIcon(0, icon);
		treeItem->setFlags(treeItem->flags() | Qt::ItemIsUserCheckable | Qt::ItemIsSelectable);
		treeItem->setCheckState(0, Qt::Unchecked);

		if (parent == nullptr)
		{
			parent = uuidItem["root"];
		}
		parent->addChild(treeItem);

		uuidItem[uuid] = treeItem;
		itemUuid[treeItem] = uuid;

		uuidCheckable.push_back(uuid);
		filenameToUuids[uuidToFilename[uuid]].push_back(uuid);
	}
	else
	{
		/* TODO -- affichage error
		std::stringstream sstm;
		sstm << " already exists: uuid (" << uuid << ")";
		auto test = fespp[epcDocument[uuid]];
		fespp[epcDocument[uuid]]->displayWarning(sstm.str());
		 */

	}
}

//----------------------------------------------------------------------------
void PQSelectionPanel::addTreeProperty(QTreeWidgetItem *parent, std::vector<resqml2::AbstractValuesProperty*> valuesPropertySet)
{
	QButtonGroup *buttonGroup;
	if (uuidParentGroupButton.count(itemUuid[parent]) < 1)
	{
		buttonGroup = new QButtonGroup();
		buttonGroup->setExclusive(true);
	}
	else{
		buttonGroup = uuidParentGroupButton[itemUuid[parent]];
	}
	QRadioButton *radioButtonInvisible = new QRadioButton("invisible");
	buttonGroup->addButton(radioButtonInvisible, 0);
	radioButtonCount++;
	mapUuidParentButtonInvisible[itemUuid[parent]] = radioButtonInvisible;

	for (size_t i = 0; i < valuesPropertySet.size(); ++i)
	{
		resqml2::AbstractValuesProperty* valuesProperty = valuesPropertySet[i];
		if (valuesProperty->getXmlTag() == "ContinuousProperty" ||
				valuesProperty->getXmlTag() == "DiscreteProperty" ||
				valuesProperty->getXmlTag() == "CategoricalProperty")
		{
			std::string uuid;
			if (valuesProperty->getElementCountPerValue() > 0) {
				uuid = valuesPropertySet[i]->getUuid();

				QTreeWidgetItem *treeItem = new QTreeWidgetItem();
				QRadioButton *radioButton = new QRadioButton(QString(valuesPropertySet[i]->getTitle().c_str()));

				mapRadioButtonNo[radioButtonCount] = uuid;

				buttonGroup->addButton(radioButton, radioButtonCount);
				parent->addChild(treeItem);

				treeWidget->setItemWidget(treeItem, 0, radioButton);
				uuidItem[uuid] = treeItem;
				uuidParentItem[uuid] = parent;
				itemUuid[treeItem] = uuid;
				uuidToFilename[uuid] = uuidToFilename[itemUuid[parent]];

				radioButtonCount++;
				radioButtonToUuid[radioButton] = uuid;
				parent->setData(0, Qt::CheckStateRole, QVariant());
			}
			else
			{
				QTreeWidgetItem *treeItem = new QTreeWidgetItem();
				treeItem->setText(0, QString(valuesPropertySet[i]->getTitle().c_str()));

				parent->addChild(treeItem);
			}
		}
	}
	connect(buttonGroup, SIGNAL(buttonClicked(int)), this, SLOT(checkedRadioButton(int)));
	uuidParentGroupButton[itemUuid[parent]] = buttonGroup;
}

//----------------------------------------------------------------------------
void PQSelectionPanel::deleteFileName(const std::string & fileName)
{
	cout << "PQSelectionPanel::deleteFileName("<< fileName << ") IN\n";
	std::vector<std::string> uuids = filenameToUuids[fileName];
	for (size_t i = 0; i < uuids.size(); ++i)
	{
		cout << " uuid " << i << " = " << uuids[i] << "\n";
		auto uuidTest = uuids[i];
		cout << " 1\n";
		std::vector<std::string>::iterator uuidCheckable_iterator = std::find(uuidCheckable.begin(), uuidCheckable.end(), uuids[i]);
		cout << " 2\n";
		if (uuidCheckable_iterator != uuidCheckable.end())
		{
			cout << " 3\n";
			uuidCheckable.erase(uuidCheckable_iterator);
		}
		cout << " 4\n";
		std::vector<std::string>::iterator uuidVisible_iterator = std::find(uuidVisible.begin(), uuidVisible.end(), uuids[i]);
		cout << " 5\n";
		if (uuidVisible_iterator != uuidVisible.end())
		{
			cout << " 6\n";
			uuidVisible.erase(uuidVisible_iterator);
		}
		cout << " 7\n";
		itemUuid.remove(uuidItem[uuids[i]]);
		cout << " 8\n";
		if (uuidItem[uuids[i]]->childCount() > 0)
		{
			cout << " 9\n";
			deleteUUID(uuidItem[uuids[i]]);
		}
		/*		else
		{
			cout << " 10 = " << uuidItem[uuids[i]]->childCount() << "\n";
			uuidItem[uuids[i]]->setHidden(true);
		}*/
		cout << " 11\n";
		uuidItem.remove(uuids[i]);
		cout << " 12\n";
		uuidToFilename.remove(uuids[i]);
		cout << " 13\n";
		uuidParentItem.remove(uuids[i]);
		cout << " 14\n";
		mapUuidProperty.remove(uuids[i]);
		cout << " 15\n";
		mapUuidWithProperty.remove(uuids[i]);
	}

	cout << "tous les uuids sont traités \n";
	uuids = filenameToUuidsPartial[fileName];
	/*
	for (size_t i = 0; i < uuids.size(); ++i){
		if (partialRepresentation.count(uuids[i]) > 0){
			partialRepresentation.remove(uuids[i]);
		}
	}
	 */

	filenameToUuids.remove(fileName);

	delete uuidItem[fileName];


	cout << "PQSelectionPanel::deleteFileName("<< fileName << ") OUT\n";
}

//****************************************************************************
void PQSelectionPanel::deleteUUID(QTreeWidgetItem *item)
{
	cout <<" void PQSelectionPanel::deleteUUID(QTreeWidgetItem *item) \n";
	cout <<" void PQSelectionPanel::deleteUUID(QTreeWidgetItem *item) child = " <<  item->childCount() << "\n";
	for (int o = 0; item->childCount(); o++)
	{
		cout << "vhild n = " << o << "\n";
		deleteUUID(item->child(0));
	}
	delete item;
}
//********************************* Interfacing ******************************
//----------------------------------------------------------------------------
void PQSelectionPanel::loadUuid(std::string uuid)
{
	pqPipelineSource * source = findPipelineSource("EpcDocument");
	if (source)
	{
		pqActiveObjects *activeObjects = &pqActiveObjects::instance();
		activeObjects->setActiveSource(source);

		PQToolsManager* manager = PQToolsManager::instance();
		auto fesppReader = manager->getFesppReader();

		if (fesppReader)
		{
			pqActiveObjects *activeObjects = &pqActiveObjects::instance();
			activeObjects->setActiveSource(fesppReader);

			// add uuid to property panel
			vtkSMProxy* fesppReaderProxy = fesppReader->getProxy();

			vtkSMPropertyHelper( fesppReaderProxy, "uuidList" ).SetStatus(uuid.c_str(),1);

			fesppReaderProxy->UpdatePropertyInformation();
			fesppReaderProxy->UpdateVTKObjects();
			getpqPropertiesPanel()->update();
			getpqPropertiesPanel()->apply();
		}
	}
}

//----------------------------------------------------------------------------
void PQSelectionPanel::removeUuid(std::string uuid)
{
	pqPipelineSource * source = findPipelineSource("EpcDocument");
	if (source)
	{
		pqActiveObjects *activeObjects = &pqActiveObjects::instance();
		activeObjects->setActiveSource(source);

		PQToolsManager* manager = PQToolsManager::instance();
		auto fesppReader = manager->getFesppReader();

		if (fesppReader)
		{
			pqActiveObjects *activeObjects = &pqActiveObjects::instance();
			activeObjects->setActiveSource(fesppReader);

			// add file to property
			vtkSMProxy* fesppReaderProxy = fesppReader->getProxy();

			vtkSMPropertyHelper( fesppReaderProxy, "uuidList" ).SetStatus(uuid.c_str(),0);

			fesppReaderProxy->UpdatePropertyInformation();
			fesppReaderProxy->UpdateVTKObjects();
			getpqPropertiesPanel()->update();
			getpqPropertiesPanel()->apply();
		}
	}
}

//****************************************************************************

//************************** Pipeline & Server *******************************

//----------------------------------------------------------------------------
pqPipelineSource * PQSelectionPanel::findPipelineSource(const char *SMName)
{
	pqApplicationCore *core = pqApplicationCore::instance();
	pqServerManagerModel *smModel = core->getServerManagerModel();

	QList<pqPipelineSource*> sources = smModel->findItems<pqPipelineSource*>(this->getActiveServer());
	foreach(pqPipelineSource *s, sources)
	{
		if (strcmp(s->getSMName().toStdString().c_str(), SMName) == 0) return s;
	}
	return NULL;
}

//----------------------------------------------------------------------------
pqServer * PQSelectionPanel::getActiveServer()
{
	pqApplicationCore *app = pqApplicationCore::instance();
	pqServerManagerModel *smModel = app->getServerManagerModel();
	pqServer *server = smModel->getItemAtIndex<pqServer*>(0);

	return server;
}

//****************************************************************************//----------------------------------------------------------------------------
void PQSelectionPanel::uuidKO(const std::string & uuid)
{
	uuidItem[uuid]->setDisabled(true);
	for (int idx_child = 0; idx_child < uuidItem[uuid]->childCount(); ++idx_child)
	{
		uuidItem[uuid]->child(idx_child)->setHidden(true);
	}
}
