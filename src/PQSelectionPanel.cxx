#include <sstream>
#include <qmessagebox.h>
#include <qprogressbar.h>
#include <qlayout.h>
#include <stdexcept>

#include "PQSelectionPanel.h"
#include "ui_PQSelectionPanel.h"
#include "Fespp.h"

// include VTK Resqml2
#include "VTK/VtkEpcDocument.h"
#include "VTK/VtkAbstractObject.h"

// include API Resqml2
#include "resqml2_0_1/PolylineSetRepresentation.h"
#include "resqml2_0_1/TriangulatedSetRepresentation.h"
#include "resqml2_0_1/Horizon.h"
#include "resqml2_0_1/TectonicBoundaryFeature.h"
#include "resqml2_0_1/HorizonInterpretation.h"
#include "resqml2_0_1/FaultInterpretation.h"
#include "resqml2_0_1/PointSetRepresentation.h"
#include "resqml2_0_1/Grid2dRepresentation.h"
#include "resqml2/AbstractObject.h"
#include "resqml2/SubRepresentation.h"
#include "resqml2_0_1/AbstractIjkGridRepresentation.h"
#include "resqml2/AbstractValuesProperty.h"
#include "resqml2_0_1/UnstructuredGridRepresentation.h"
#include "resqml2_0_1/WellboreTrajectoryRepresentation.h"
#include "resqml2_0_1/PropertyKindMapper.h"
#include "resqml2_0_1/SubRepresentation.h"

// include radio button class
#include <QtGui>
#include <QFileInfo>
#include <QIcon>

#include "qtreewidget.h"
//#include "QListWidget.h"
#include <QHeaderView>
#include <pqPropertiesPanel.h>
#include <pqMultiBlockInspectorPanel.h>
#include <qobject.h>
#include <pqView.h>

// include VTK
#include "vtkSMProxy.h"
#include "vtkSMIntVectorProperty.h"

#include <algorithm>
#include <pqPipelineSource.h>
#include <pqApplicationCore.h>
#include <pqServerManagerModel.h>
#include <pqServer.h>
#include <pqOutputPort.h>
#include "pqActiveObjects.h"
#include "vtkSMSessionProxyManager.h"
#include "pqOutputPort.h"
#include <QMenu>
#include <QProgressDialog>
#include <pqCoreUtilities.h>

#include <vtkInformation.h>

#include <QList>


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

	pqMultiBlockInspectorPanel* getpqMultiBlockInspectorPanel()
	{
		// get multi-block inspector panel
		pqMultiBlockInspectorPanel *panel = 0;
		foreach(QWidget *widget, qApp->topLevelWidgets())
		{
			panel = widget->findChild<pqMultiBlockInspectorPanel *>();

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
	treeWidget->header()->setResizeMode(0, QHeaderView::Stretch);
	treeWidget->expandToDepth(0);
	treeWidget->header()->close();

	connect(ui.treeWidget, SIGNAL(itemClicked(QTreeWidgetItem*, int)), this, SLOT(clicSelection(QTreeWidgetItem*, int)));
	connect(ui.treeWidget, SIGNAL(itemChanged(QTreeWidgetItem*, int)), this, SLOT(onItemCheckedUnchecked(QTreeWidgetItem*, int)));
	connect(ui.treeWidget, SIGNAL(itemDoubleClicked(QTreeWidgetItem*, int)), this, SLOT(doubleClicSelection(QTreeWidgetItem*, int)));

	progressBar = ui.progressBar;

	checkAllButton = ui.checkAllButton;
	uncheckAllButton = ui.unCheckAllButton;
	connect(checkAllButton, SIGNAL(clicked()), this, SLOT(checkButton()));
	connect(uncheckAllButton, SIGNAL(clicked()), this, SLOT(uncheckButton()));

	connect(getpqPropertiesPanel(), SIGNAL(deleteRequested(pqPipelineSource*)), this, SLOT(deletePipelineSource(pqPipelineSource*)));

	radioButtonCount = 0;

	dlg.setLabelText(QString("Progressing..."));
	dlg.setWindowFlags(Qt::Window | Qt::WindowTitleHint | Qt::CustomizeWindowHint);
	dlg.setCancelButton(0);
	dlg.setMinimum(0);
	dlg.setMaximum(0);

	connect(&futureWatcher, SIGNAL(finished()), &dlg, SLOT(reset()));

	indexFile = 0;

	addTreeRoot("root", "epcdocument");
}

//******************************* ACTIONS ************************************

//----------------------------------------------------------------------------
void PQSelectionPanel::checkButton()
{
	progressBar->setMinimum(0);
	progressBar->setMaximum(uuidCheckable.size());

	for (unsigned int i = 0; i < uuidCheckable.size(); ++i)
	{
		progressBar->setValue(i);
		uuidItem[uuidCheckable[i]]->setCheckState(0, Qt::Checked);
	}
	progressBar->reset();
}

//----------------------------------------------------------------------------
void PQSelectionPanel::uncheckButton()
{
	progressBar->setMinimum(0);
	progressBar->setMaximum(uuidCheckable.size());

	for (unsigned int i = 0; i < uuidCheckable.size(); ++i)
	{
		progressBar->setValue(i);
		uuidItem[uuidCheckable[i]]->setCheckState(0, Qt::Unchecked);
	}
	progressBar->reset();
}

//----------------------------------------------------------------------------
void PQSelectionPanel::clicSelection(QTreeWidgetItem* item, int column)
{
	pickedBlocks = itemUuid[item];

	if (column == 1)
	{
		std::vector<std::string>::iterator it_Visible = std::find(uuidVisible.begin(), uuidVisible.end(), pickedBlocks);
		std::vector<std::string>::iterator it_Invisible = std::find(uuidInvisible.begin(), uuidInvisible.end(), pickedBlocks);

		bool visibilityOn = false;
		bool visibilityOff = false;

		if (it_Visible != uuidVisible.end())
			visibilityOn = true;
		if (it_Invisible != uuidInvisible.end())
			visibilityOff = true;

		if (visibilityOn)
		{
			uuidVisible.erase(it_Visible);
			uuidInvisible.push_back(pickedBlocks);

			this->hideBlock();
		}
		if (visibilityOff)
		{
			uuidVisible.push_back(pickedBlocks);
			uuidInvisible.erase(it_Invisible);

			this->showBlock();
		}
	}
	else
	{
		QFileInfo file(uuidToFilename[pickedBlocks].c_str());
		pqPipelineSource * source = findPipelineSource(file.fileName().toStdString().c_str());
		if (source)
		{
			pqActiveObjects *activeObjects = &pqActiveObjects::instance();
			activeObjects->setActiveSource(source);
		}
		if (!(uuidToFilename[pickedBlocks] == pickedBlocks))
			emit selectionName(uuidToFilename[pickedBlocks], pickedBlocks, pcksave[uuidToFilename[pickedBlocks]]);
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
				QIcon icon;
				icon.addFile(QString::fromUtf8(":pqEyeball16.png"), QSize(), QIcon::Normal, QIcon::On);
				uuidItem[uuid]->setIcon(1, icon);

				this->loadUuid(uuid);
			}
			else
			{
				bool removeOK = false;
				// Property exist
				if (uuidItem[uuid]->childCount() > 0 && mapUuidWithProperty[uuid] != "")
				{
					std::string uuidOld = mapUuidWithProperty[uuid];
					this->removeUuid(uuidOld, epcDocument[uuidOld]);

					mapUuidWithProperty[uuid] = "";
					mapUuidParentButtonInvisible[uuid]->setChecked(true);
					uuidItem[uuid]->setData(0, Qt::CheckStateRole, QVariant());
				}

				std::vector<std::string>::iterator it_Visible = std::find(uuidVisible.begin(), uuidVisible.end(), uuid);
				std::vector<std::string>::iterator it_Invisible = std::find(uuidInvisible.begin(), uuidInvisible.end(), uuid);
				if (it_Visible != uuidVisible.end())
				{
					removeOK = true;
					uuidVisible.erase(std::find(uuidVisible.begin(), uuidVisible.end(), uuid));
				}
				if (it_Invisible != uuidInvisible.end())
				{
					removeOK = true;
					uuidInvisible.erase(std::find(uuidInvisible.begin(), uuidInvisible.end(), uuid));
				}

				if (removeOK)
				{
					QIcon icon;
					uuidItem[uuid]->setIcon(1, icon);
					this->removeUuid(uuid, epcDocument[uuid]);
				}
			}
		}
	}
	progressBar->setFormat("");
	progressBar->setMinimum(0);
	progressBar->setMaximum(100);
	progressBar->show();
}

//----------------------------------------------------------------------------
void PQSelectionPanel::doubleClicSelection(QTreeWidgetItem* item, int column)
{
	pickedBlocks = itemUuid[item];
	if (column == 1)
	{
		if (!(uuidItem[pickedBlocks]->icon(1).isNull()))
		{
			QPoint position = QCursor::pos();

			QMenu * menu = new QMenu();
			QAction * hide = menu->addAction(QString("Hide"));
			this->connect(hide, SIGNAL(triggered()), this, SLOT(hideBlock()));
			QAction *show = menu->addAction(QString("Show"));
			this->connect(show, SIGNAL(triggered()), this, SLOT(showBlock()));
			QAction *showOnly = menu->addAction(QString("Show Only"));
			this->connect(showOnly, SIGNAL(triggered()), this, SLOT(showOnlyBlock()));
			QAction *showAll = menu->addAction(QString("Show All"));
			this->connect(showAll, SIGNAL(triggered()), this, SLOT(showAllBlocks()));

			menu->popup(position);
		}
	}
}

//----------------------------------------------------------------------------
void PQSelectionPanel::deletePipelineSource(pqPipelineSource* pipe)
{
	for (unsigned int fileNameInd = 0; fileNameInd < allFileName.size(); ++fileNameInd)
	{
		QFileInfo file(allFileName[fileNameInd].c_str());
		const char* toto = file.fileName().toStdString().c_str();
		pqPipelineSource * source = findPipelineSource(file.fileName().toStdString().c_str());
		if (!source)
		{
			this->deleteFileName(allFileName[fileNameInd]);

		}
	}
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
			this->removeUuid(uuidOld, epcDocument[uuidParent]);
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
VtkEpcDocument* PQSelectionPanel::addFileName(const std::string & fileName, Fespp *plugin, common::EpcDocument *pck)
{
	allFileName.push_back(fileName);
	uuidToFilename[fileName] = fileName;

	pcksave[fileName] = pck;

	VtkEpcDocument *vtkEpcDocument = new VtkEpcDocument(fileName, pck);

	epcDocument[fileName] = vtkEpcDocument;
	fespp[vtkEpcDocument] = plugin;

	// add treeView polylines representation
	addTreePolylines(fileName, pck->getFaultPolylineSetRepSet(), vtkEpcDocument);
	addTreePolylines(fileName, pck->getHorizonPolylineSetRepSet(), vtkEpcDocument);

	// add treeView triangulated representation
	addTreeTriangulated(fileName, pck->getAllTriangulatedSetRepSet(), vtkEpcDocument);

	// add treeView Grid2D representation
	addTreeGrid2D(fileName, pck->getHorizonGrid2dRepSet(), vtkEpcDocument);

	// add treeView ijkGrid representation
	addTreeIjkGrid(fileName, pck->getIjkGridRepresentationSet(), vtkEpcDocument);

	// add treeView UnstrucutredGrid representation
	addTreeUnstructuredGrid(fileName, pck->getUnstructuredGridRepresentationSet(), vtkEpcDocument);

	// add treeView UnstrucutredGrid representation
	addTreeWellboreTrajectory(fileName, pck->getWellboreCubicParamLineTrajRepSet(), vtkEpcDocument);

	// add treeView Sub-representation
	addTreeSubRepresentation(fileName, pck->getSubRepresentationSet(), vtkEpcDocument);

	plugin->visualize(fileName, "first");

	progressBar->reset();

	++indexFile;
	return vtkEpcDocument;
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
std::string PQSelectionPanel::addFeatInterp(const std::string & fileName, resqml2::AbstractFeatureInterpretation * interpretation, VtkEpcDocument * vtkEpcDocument)
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
void PQSelectionPanel::addTreePolylines(const std::string & fileName, std::vector<resqml2_0_1::PolylineSetRepresentation*> polylines, VtkEpcDocument * vtkEpcDocument)
{
	QIcon icon;
	icon.addFile(QString::fromUtf8(":Polyline.png"), QSize(), QIcon::Normal, QIcon::Off);
	for (unsigned int polylineIter = 0; polylineIter < polylines.size(); ++polylineIter)
	{
		bool propertyTreeView = false;

		std::string uuidParent = addFeatInterp(fileName, polylines[polylineIter]->getInterpretation(), vtkEpcDocument);
		if (polylines[polylineIter]->isPartial()) {
			if (epcDocument.count(polylines[polylineIter]->getUuid())>0){
				vtkEpcDocument->createTreeVtkPartialRep(polylines[polylineIter]->getUuid(), uuidParent, epcDocument[polylines[polylineIter]->getUuid()]);
				partialRepresentation[polylines[polylineIter]->getUuid()] = vtkEpcDocument;
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
			vtkEpcDocument->createTreeVtk(polylines[polylineIter]->getUuid(), uuidParent, polylines[polylineIter]->getTitle().c_str(), VtkAbstractObject::Resqml2Type::POLYLINE_SET);
			epcDocument[polylines[polylineIter]->getUuid()] = vtkEpcDocument;
			propertyTreeView = true;
		}
		if (propertyTreeView){
			std::vector<resqml2::AbstractValuesProperty*> valuesPropertySet = polylines[polylineIter]->getValuesPropertySet();
			addTreeProperty(uuidItem[polylines[polylineIter]->getUuid()], valuesPropertySet, vtkEpcDocument);
		}
	}
}

//----------------------------------------------------------------------------
void PQSelectionPanel::addTreeTriangulated(const std::string & fileName, std::vector<resqml2_0_1::TriangulatedSetRepresentation*> triangulated, VtkEpcDocument * vtkEpcDocument)
{
	QIcon icon;
	icon.addFile(QString::fromUtf8(":Triangulated.png"), QSize(), QIcon::Normal, QIcon::Off);
	for (unsigned int triangulatedIter = 0; triangulatedIter < triangulated.size(); ++triangulatedIter)
	{
		bool propertyTreeView = false;

		std::string uuidParent = addFeatInterp(fileName, triangulated[triangulatedIter]->getInterpretation(), vtkEpcDocument);
		if (triangulated[triangulatedIter]->isPartial()){
			if (epcDocument.count(triangulated[triangulatedIter]->getUuid())>0){
				vtkEpcDocument->createTreeVtkPartialRep(triangulated[triangulatedIter]->getUuid(), uuidParent, epcDocument[triangulated[triangulatedIter]->getUuid()]);
				partialRepresentation[triangulated[triangulatedIter]->getUuid()] = vtkEpcDocument;
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
			vtkEpcDocument->createTreeVtk(triangulated[triangulatedIter]->getUuid(), uuidParent, triangulated[triangulatedIter]->getTitle().c_str(), VtkAbstractObject::Resqml2Type::TRIANGULATED_SET);
			epcDocument[triangulated[triangulatedIter]->getUuid()] = vtkEpcDocument;
			propertyTreeView = true;
		}
		if (propertyTreeView){
			std::vector<resqml2::AbstractValuesProperty*> valuesPropertySet = triangulated[triangulatedIter]->getValuesPropertySet();
			addTreeProperty(uuidItem[triangulated[triangulatedIter]->getUuid()], valuesPropertySet, epcDocument[triangulated[triangulatedIter]->getUuid()]);
		}
	}
}

//----------------------------------------------------------------------------
void PQSelectionPanel::addTreeGrid2D(const std::string & fileName, std::vector<resqml2_0_1::Grid2dRepresentation*> grid2D, VtkEpcDocument * vtkEpcDocument)
{
	QIcon icon;
	icon.addFile(QString::fromUtf8(":Grid2D.png"), QSize(), QIcon::Normal, QIcon::Off);
	for (unsigned int grid2DIter = 0; grid2DIter < grid2D.size(); ++grid2DIter)
	{
		bool propertyTreeView = false;

		std::string uuidParent = addFeatInterp(fileName, grid2D[grid2DIter]->getInterpretation(), vtkEpcDocument);
		if (grid2D[grid2DIter]->isPartial()){
			if (epcDocument.count(grid2D[grid2DIter]->getUuid())>0){
				vtkEpcDocument->createTreeVtkPartialRep(grid2D[grid2DIter]->getUuid(), uuidParent, epcDocument[grid2D[grid2DIter]->getUuid()]);
				partialRepresentation[grid2D[grid2DIter]->getUuid()] = vtkEpcDocument;
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
			vtkEpcDocument->createTreeVtk(grid2D[grid2DIter]->getUuid(), uuidParent, grid2D[grid2DIter]->getTitle().c_str(), VtkAbstractObject::Resqml2Type::GRID_2D);
			epcDocument[grid2D[grid2DIter]->getUuid()] = vtkEpcDocument;
			propertyTreeView = true;
		}
		if (propertyTreeView){
			std::vector<resqml2::AbstractValuesProperty*> valuesPropertySet = grid2D[grid2DIter]->getValuesPropertySet();
			addTreeProperty(uuidItem[grid2D[grid2DIter]->getUuid()], valuesPropertySet, epcDocument[grid2D[grid2DIter]->getUuid()]);
		}
	}
}

//----------------------------------------------------------------------------
void PQSelectionPanel::addTreeIjkGrid(const std::string & fileName, std::vector<resqml2_0_1::AbstractIjkGridRepresentation*> ijkGrid, VtkEpcDocument * vtkEpcDocument)
{
	QIcon icon;
	icon.addFile(QString::fromUtf8(":IjkGrid.png"), QSize(), QIcon::Normal, QIcon::Off);
	for (unsigned int ijkGridIter = 0; ijkGridIter < ijkGrid.size(); ++ijkGridIter)
	{
		bool propertyTreeView = false;

		if (ijkGrid[ijkGridIter]->getGeometryKind() != resqml2_0_1::AbstractIjkGridRepresentation::NO_GEOMETRY)
		{
			std::string uuidParent = addFeatInterp(fileName, ijkGrid[ijkGridIter]->getInterpretation(), vtkEpcDocument);
			if (ijkGrid[ijkGridIter]->isPartial()){
				if (epcDocument.count(ijkGrid[ijkGridIter]->getUuid())>0){
					vtkEpcDocument->createTreeVtkPartialRep(ijkGrid[ijkGridIter]->getUuid(), uuidParent, epcDocument[ijkGrid[ijkGridIter]->getUuid()]);
					partialRepresentation[ijkGrid[ijkGridIter]->getUuid()] = vtkEpcDocument;
					filenameToUuidsPartial[fileName].push_back(ijkGrid[ijkGridIter]->getUuid());

					propertyTreeView = true;
				}
				else{
					QMessageBox msgBox;
					msgBox.setIcon(QMessageBox::Information);
					msgBox.setText(QString(("Partial UUID: " + ijkGrid[ijkGridIter]->getUuid() + " and the complete UUID not found.").c_str()));
					msgBox.exec();
				}
			}
			else{
				uuidToFilename[ijkGrid[ijkGridIter]->getUuid()] = fileName;

				addTreeRepresentation(uuidItem[uuidParent],
					ijkGrid[ijkGridIter]->getTitle().c_str(),
					ijkGrid[ijkGridIter]->getUuid(),
					icon
					);
				vtkEpcDocument->createTreeVtk(ijkGrid[ijkGridIter]->getUuid(), uuidParent, ijkGrid[ijkGridIter]->getTitle().c_str(), VtkAbstractObject::Resqml2Type::IJK_GRID);
				epcDocument[ijkGrid[ijkGridIter]->getUuid()] = vtkEpcDocument;
				propertyTreeView = true;
			}
			if (propertyTreeView){
				std::vector<resqml2::AbstractValuesProperty*> valuesPropertySet = ijkGrid[ijkGridIter]->getValuesPropertySet();
				addTreeProperty(uuidItem[ijkGrid[ijkGridIter]->getUuid()], valuesPropertySet, vtkEpcDocument);
			}
		}
	}
}

//----------------------------------------------------------------------------
void PQSelectionPanel::addTreeUnstructuredGrid(const std::string & fileName, std::vector<resqml2_0_1::UnstructuredGridRepresentation*> unstructuredGrid, VtkEpcDocument * vtkEpcDocument)
{
	QIcon icon;
	icon.addFile(QString::fromUtf8(":UnstructuredGrid.png"), QSize(), QIcon::Normal, QIcon::Off);
	for (unsigned int unstructuredGridIter = 0; unstructuredGridIter < unstructuredGrid.size(); ++unstructuredGridIter)
	{
		bool propertyTreeView = false;

		std::string uuidParent = addFeatInterp(fileName, unstructuredGrid[unstructuredGridIter]->getInterpretation(), vtkEpcDocument);
		if (unstructuredGrid[unstructuredGridIter]->isPartial()){
			if (epcDocument.count(unstructuredGrid[unstructuredGridIter]->getUuid())>0){
				vtkEpcDocument->createTreeVtkPartialRep(unstructuredGrid[unstructuredGridIter]->getUuid(), uuidParent, epcDocument[unstructuredGrid[unstructuredGridIter]->getUuid()]);
				partialRepresentation[unstructuredGrid[unstructuredGridIter]->getUuid()] = vtkEpcDocument;
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
			vtkEpcDocument->createTreeVtk(unstructuredGrid[unstructuredGridIter]->getUuid(), uuidParent, unstructuredGrid[unstructuredGridIter]->getTitle().c_str(), VtkAbstractObject::Resqml2Type::UNSTRUC_GRID);
			epcDocument[unstructuredGrid[unstructuredGridIter]->getUuid()] = vtkEpcDocument;
			propertyTreeView = true;
		}
		if (propertyTreeView){
			std::vector<resqml2::AbstractValuesProperty*> valuesPropertySet = unstructuredGrid[unstructuredGridIter]->getValuesPropertySet();
			addTreeProperty(uuidItem[unstructuredGrid[unstructuredGridIter]->getUuid()], valuesPropertySet, vtkEpcDocument);
		}
	}
}

//----------------------------------------------------------------------------
void PQSelectionPanel::addTreeWellboreTrajectory(const std::string & fileName, std::vector<resqml2_0_1::WellboreTrajectoryRepresentation*> WellboreTrajectory, VtkEpcDocument * vtkEpcDocument)
{
	QIcon icon;
	icon.addFile(QString::fromUtf8(":WellTraj.png"), QSize(), QIcon::Normal, QIcon::Off);
	for (unsigned int WellboreTrajectoryIter = 0; WellboreTrajectoryIter < WellboreTrajectory.size(); ++WellboreTrajectoryIter)
	{
		bool propertyTreeView = false;

		std::string uuidParent = addFeatInterp(fileName, WellboreTrajectory[WellboreTrajectoryIter]->getInterpretation(), vtkEpcDocument);
		if (WellboreTrajectory[WellboreTrajectoryIter]->isPartial()){
			if (epcDocument.count(WellboreTrajectory[WellboreTrajectoryIter]->getUuid())>0){
				vtkEpcDocument->createTreeVtkPartialRep(WellboreTrajectory[WellboreTrajectoryIter]->getUuid(), uuidParent, epcDocument[WellboreTrajectory[WellboreTrajectoryIter]->getUuid()]);
				partialRepresentation[WellboreTrajectory[WellboreTrajectoryIter]->getUuid()] = vtkEpcDocument;
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
			vtkEpcDocument->createTreeVtk(WellboreTrajectory[WellboreTrajectoryIter]->getUuid(), uuidParent, WellboreTrajectory[WellboreTrajectoryIter]->getTitle().c_str(), VtkAbstractObject::Resqml2Type::WELL_TRAJ);
			epcDocument[WellboreTrajectory[WellboreTrajectoryIter]->getUuid()] = vtkEpcDocument;
			propertyTreeView = true;
		}
		if (propertyTreeView){
			std::vector<resqml2::AbstractValuesProperty*> valuesPropertySet = WellboreTrajectory[WellboreTrajectoryIter]->getValuesPropertySet();
			addTreeProperty(uuidItem[WellboreTrajectory[WellboreTrajectoryIter]->getUuid()], valuesPropertySet, epcDocument[WellboreTrajectory[WellboreTrajectoryIter]->getUuid()]);
		}
	}
}

//----------------------------------------------------------------------------
void PQSelectionPanel::addTreeSubRepresentation(const std::string & fileName, std::vector<resqml2::SubRepresentation*> subRepresentation, VtkEpcDocument * vtkEpcDocument)
{
	QIcon icon;
	icon.addFile(QString::fromUtf8(":SubRepresentation.png"), QSize(), QIcon::Normal, QIcon::Off);
	for (auto subRepresentationIter = 0; subRepresentationIter < subRepresentation.size(); ++subRepresentationIter)
	{
		auto propertyTreeView = false;

		auto uuidParent = subRepresentation[subRepresentationIter]->getSupportingRepresentationUuid(0);
		if (epcDocument.count(uuidParent)>0)
		{
			uuidToFilename[subRepresentation[subRepresentationIter]->getUuid()] = fileName;

			addTreeRepresentation(uuidItem[uuidParent],
				subRepresentation[subRepresentationIter]->getTitle().c_str(),
				subRepresentation[subRepresentationIter]->getUuid(),
				icon
				);
			vtkEpcDocument->createTreeVtk(subRepresentation[subRepresentationIter]->getUuid(), uuidParent, subRepresentation[subRepresentationIter]->getTitle().c_str(), VtkAbstractObject::Resqml2Type::SUB_REP);
			epcDocument[subRepresentation[subRepresentationIter]->getUuid()] = vtkEpcDocument;
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
			addTreeProperty(uuidItem[subRepresentation[subRepresentationIter]->getUuid()], valuesPropertySet, epcDocument[subRepresentation[subRepresentationIter]->getUuid()]);
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
		std::stringstream sstm;
		sstm << " already exists: uuid (" << uuid << ")";
		auto test = fespp[epcDocument[uuid]];
		fespp[epcDocument[uuid]]->displayWarning(sstm.str());
	}
}

//----------------------------------------------------------------------------
void PQSelectionPanel::addTreeProperty(QTreeWidgetItem *parent, std::vector<resqml2::AbstractValuesProperty*> valuesPropertySet, VtkEpcDocument * vtkEpcDocument)
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

	for (unsigned int i = 0; i < valuesPropertySet.size(); ++i)
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

				vtkEpcDocument->createTreeVtk(uuid, itemUuid[parent], valuesPropertySet[i]->getTitle().c_str(), VtkAbstractObject::Resqml2Type::PROPERTY);
				epcDocument[uuid] = vtkEpcDocument;

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
	std::vector<std::string> uuids = filenameToUuids[fileName];
	for (unsigned int i = 0; i < uuids.size(); ++i)
	{
		auto uuidTest = uuids[i];

		std::vector<std::string>::iterator uuidCheckable_iterator = std::find(uuidCheckable.begin(), uuidCheckable.end(), uuids[i]);
		if (uuidCheckable_iterator != uuidCheckable.end())
		{
			uuidCheckable.erase(uuidCheckable_iterator);
		}
		std::vector<std::string>::iterator uuidInvisible_iterator = std::find(uuidInvisible.begin(), uuidInvisible.end(), uuids[i]);
		if (uuidInvisible_iterator != uuidInvisible.end())
		{
			uuidInvisible.erase(uuidInvisible_iterator);
		}
		std::vector<std::string>::iterator uuidVisible_iterator = std::find(uuidVisible.begin(), uuidVisible.end(), uuids[i]);
		if (uuidVisible_iterator != uuidVisible.end())
		{
			uuidVisible.erase(uuidVisible_iterator);
		}

		itemUuid.remove(uuidItem[uuids[i]]);
		if (uuidItem[uuids[i]]->childCount() > 0) { deleteUUID(uuidItem[uuids[i]]); }
		else { uuidItem[uuids[i]]->setHidden(true); }
		uuidItem.remove(uuids[i]);
		uuidToFilename.remove(uuids[i]);
		uuidParentItem.remove(uuids[i]);
		mapUuidProperty.remove(uuids[i]);
		mapUuidWithProperty.remove(uuids[i]);
	}
	uuids = filenameToUuidsPartial[fileName];
	for (unsigned int i = 0; i < uuids.size(); ++i){
		if (partialRepresentation.count(uuids[i]) > 0){
			partialRepresentation[uuids[i]]->deleteTreeVtkPartial(uuids[i]);
			partialRepresentation.remove(uuids[i]);
		}
	}

	fespp.remove(epcDocument[fileName]);
	filenameToUuids.remove(fileName);
	delete uuidItem[fileName];


	std::vector<std::string>::iterator allFileName_iterator = std::find(allFileName.begin(), allFileName.end(), fileName);
	if (allFileName_iterator != allFileName.end())
	{
		allFileName.erase(allFileName_iterator);
	}
}

//****************************************************************************
void PQSelectionPanel::deleteUUID(QTreeWidgetItem *item)
{
	auto test = item->childCount();
	for (int o = 0; item->childCount(); o++)
	{
		deleteUUID(item->child(0));
	}
	delete item;
}
//********************************* Interfacing ******************************
//----------------------------------------------------------------------------
void PQSelectionPanel::loadUuid(std::string uuid)
{
	QFileInfo file(uuidToFilename[uuid].c_str());
	pqPipelineSource * source = findPipelineSource(file.fileName().toStdString().c_str());
	if (source)
	{
		pqActiveObjects *activeObjects = &pqActiveObjects::instance();
		activeObjects->setActiveSource(source);

		try{
			epcDocument[uuid]->visualize(uuid);
			foreach(Fespp * fessp_current, fespp)
				fessp_current->visualize(uuidToFilename[uuid], uuid);

		}
		catch (const std::exception & e)
		{
			this->uuidKO(uuid);
			std::stringstream sstm;
			sstm << "EXCEPTION fesapi.dll:" << e.what();
			fespp[epcDocument[uuid]]->displayError(sstm.str());
		}

	}
}

//----------------------------------------------------------------------------
void PQSelectionPanel::removeUuid(std::string uuid, VtkEpcDocument * epcDocumentDisplay)
{
	QFileInfo file(uuidToFilename[uuid].c_str());
	pqPipelineSource * source = findPipelineSource(file.fileName().toStdString().c_str());
	if (source)
	{
		pqActiveObjects *activeObjects = &pqActiveObjects::instance();
		activeObjects->setActiveSource(source);

		try
		{
			epcDocument[uuid]->remove(uuid);
			fespp[epcDocumentDisplay]->RemoveUuid(uuid);
		}
		catch (const std::exception & e)
		{
			std::stringstream sstm;
			sstm << "EXCEPTION fesapi.dll:" << e.what();
			fespp[epcDocument[uuid]]->displayError(sstm.str());
		}
	}
}

//****************************************************************************

//******************************** VISIBILITY *********************************

//-----------------------------------------------------------------------------
void PQSelectionPanel::hideBlock()
{
	pqMultiBlockInspectorPanel *panel = getpqMultiBlockInspectorPanel();
	if (panel)
	{
		QFileInfo file(uuidToFilename[pickedBlocks].c_str());
		pqPipelineSource * source = findPipelineSource(file.fileName().toStdString().c_str());
		if (source)
		{
			pqActiveObjects *activeObjects = &pqActiveObjects::instance();
			activeObjects->setActiveSource(source);
		}

		QString blockName = "init";
		unsigned int index = 0;
		while (blockName != "")
		{
			blockName = panel->lookupBlockName(index);
			if (pickedBlocks == blockName.toStdString())
			{
				panel->setBlockVisibility(index, false);
			}
			++index;
		}

		QIcon icon;
		icon.addFile(QString::fromUtf8(":pqEyeballd16.png"), QSize(), QIcon::Normal, QIcon::On);
		uuidItem[pickedBlocks]->setIcon(1, icon);
	}
}

//-----------------------------------------------------------------------------
void PQSelectionPanel::showBlock()
{
	pqMultiBlockInspectorPanel *panel = getpqMultiBlockInspectorPanel();
	if (panel)
	{
		QFileInfo file(uuidToFilename[pickedBlocks].c_str());
		pqPipelineSource * source = findPipelineSource(file.fileName().toStdString().c_str());
		if (source)
		{
			pqActiveObjects *activeObjects = &pqActiveObjects::instance();
			activeObjects->setActiveSource(source);
		}

		QString blockName = "init";
		unsigned int index = 0;
		while (blockName != "")
		{
			blockName = panel->lookupBlockName(index);
			if (pickedBlocks == blockName.toStdString())
			{
				panel->setBlockVisibility(index, true);
			}
			++index;
		}

		QIcon icon;
		icon.addFile(QString::fromUtf8(":pqEyeball16.png"), QSize(), QIcon::Normal, QIcon::On);
		uuidItem[pickedBlocks]->setIcon(1, icon);
	}
}

//-----------------------------------------------------------------------------
void PQSelectionPanel::showOnlyBlock()
{
	pqMultiBlockInspectorPanel *panel = getpqMultiBlockInspectorPanel();
	if (panel)
	{
		for (unsigned int fileIndex = 0; fileIndex < allFileName.size(); ++fileIndex)
		{
			QFileInfo file(allFileName[fileIndex].c_str());
			pqPipelineSource * source = findPipelineSource(file.fileName().toStdString().c_str());
			if (source)
			{
				pqActiveObjects *activeObjects = &pqActiveObjects::instance();
				activeObjects->setActiveSource(source);
			}

			QString blockName = "init";
			unsigned int index = 0;
			while (blockName != "")
			{
				blockName = panel->lookupBlockName(index);
				if (pickedBlocks == blockName.toStdString())
				{
					QList<unsigned int> allIndex;
					allIndex << index;
					panel->showOnlyBlocks(allIndex);
				}
				++index;
			}
		}

		for (std::vector<std::string>::iterator it_Visible = uuidVisible.begin(); it_Visible != uuidVisible.end(); ++it_Visible)
		{
			if (*it_Visible != pickedBlocks)
			{
				uuidInvisible.push_back(*it_Visible);
			}
		}
		uuidVisible.clear();
		uuidVisible.push_back(pickedBlocks);
		QIcon icon;
		icon.addFile(QString::fromUtf8(":pqEyeball16.png"), QSize(), QIcon::Normal, QIcon::On);
		uuidItem[pickedBlocks]->setIcon(1, icon);

		QIcon iconInvi;
		iconInvi.addFile(QString::fromUtf8(":pqEyeballd16.png"), QSize(), QIcon::Normal, QIcon::On);
		for (unsigned int i = 0; i < uuidInvisible.size(); ++i)
		{
			uuidItem[uuidInvisible[i]]->setIcon(1, iconInvi);
		}

	}
}

//-----------------------------------------------------------------------------
void PQSelectionPanel::showAllBlocks()
{
	pqMultiBlockInspectorPanel *panel = getpqMultiBlockInspectorPanel();
	if (panel)
	{
		for (unsigned int fileIndex = 0; fileIndex < allFileName.size(); ++fileIndex)
		{
			QFileInfo file(allFileName[fileIndex].c_str());
			pqPipelineSource * source = findPipelineSource(file.fileName().toStdString().c_str());
			if (source)
			{
				pqActiveObjects *activeObjects = &pqActiveObjects::instance();
				activeObjects->setActiveSource(source);
			}
			panel->showAllBlocks();
		}

		for (std::vector<std::string>::iterator it_Invisible = uuidInvisible.begin(); it_Invisible != uuidInvisible.end(); ++it_Invisible)
		{
			uuidVisible.push_back(*it_Invisible);
		}
		uuidInvisible.clear();

		QIcon icon;
		icon.addFile(QString::fromUtf8(":pqEyeball16.png"), QSize(), QIcon::Normal, QIcon::On);
		for (unsigned int i = 0; i < uuidVisible.size(); ++i)
		{
			uuidItem[uuidVisible[i]]->setIcon(1, icon);
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

//****************************************************************************
vtkSmartPointer<vtkMultiBlockDataSet> PQSelectionPanel::getOutput(const std::string & FileName) const
{
	return epcDocument[FileName]->getOutput();
}
