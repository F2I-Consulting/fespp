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

#include "resqml2/TimeSeries.h"

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

#include "VTK/VtkEpcDocumentSet.h"

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
#if QT_VERSION >= 0x050000
	treeWidget->header()->setSectionResizeMode(0, QHeaderView::Stretch);
#else
	treeWidget->header()->setResizeMode(0, QHeaderView::Stretch);
#endif
	treeWidget->expandToDepth(0);
	treeWidget->header()->close();

	//	connect(ui.treeWidget, SIGNAL(itemClicked(QTreeWidgetItem*, int)), this, SLOT(clicSelection(QTreeWidgetItem*, int)));
	connect(ui.treeWidget, SIGNAL(itemChanged(QTreeWidgetItem*, int)), this, SLOT(onItemCheckedUnchecked(QTreeWidgetItem*, int)));

	radioButtonCount = 0;

	indexFile = 0;

	//addTreeRoot("root", "epcdocument");
	vtkEpcDocumentSet = new VtkEpcDocumentSet(0, 0, VtkEpcTools::TreeView);
}

//******************************* ACTIONS ************************************

//----------------------------------------------------------------------------
void PQSelectionPanel::clicSelection(QTreeWidgetItem* item, int column)
{
	unsigned int cpt;
	auto pickedBlocks = itemUuid[item];
	if (pickedBlocks!="root")
	{
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
	auto uuid = itemUuid[item];

	if (!(uuid == ""))
	{
		if (item->checkState(0) == Qt::Checked)
		{
			this->loadUuid(uuid);
		}
		else
		{
			// Property exist
			if (uuidItem[uuid]->childCount() > 0 && mapUuidWithProperty[uuid] != "")
			{
				auto uuidProperty = mapUuidWithProperty[uuid];
				this->removeUuid(uuidProperty);

				mapUuidWithProperty[uuid] = "";
				mapUuidParentButtonInvisible[uuid]->setChecked(true);
				uuidItem[uuid]->setData(0, Qt::CheckStateRole, QVariant());
			}

			this->removeUuid(uuid);

		}
	}

}

//----------------------------------------------------------------------------
void PQSelectionPanel::deleteTreeView()
{
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

	pcksave.clear();
	uuidParentGroupButton.clear();
	radioButtonToUuid.clear();

	delete vtkEpcDocumentSet;
	vtkEpcDocumentSet = new VtkEpcDocumentSet(0, 0, VtkEpcTools::TreeView);

	//addTreeRoot("root", "epcdocument");

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
//----------------------------------------------------------------------------
void PQSelectionPanel::addFileName(const std::string & fileName)
{
	if (std::find(allFileName.begin(), allFileName.end(),fileName)==allFileName.end())
	{
		vtkEpcDocumentSet->addEpcDocument(fileName);
		allFileName.push_back(fileName);
		uuidToFilename[fileName] = fileName;

		auto treeView = vtkEpcDocumentSet->getTreeView();
		for (auto &feuille : treeView)
		{
			if (feuille.timeIndex==0)
			{
				populateTreeView(feuille.parent, feuille.parentType, feuille.uuid, feuille.name, feuille.myType);
			}
		}
	}
}

void PQSelectionPanel::populateTreeView(std::string parent, VtkEpcTools::Resqml2Type parentType, std::string uuid, std::string name, VtkEpcTools::Resqml2Type type)
{
	if (!uuidItem[uuid]){
		if (parentType==VtkEpcTools::Resqml2Type::PARTIAL && !uuidItem[parent])
		{
		}
		else
		{
			if (!uuidItem[parent])
			{
				QTreeWidgetItem *treeItem = new QTreeWidgetItem(treeWidget);

				treeItem->setExpanded(true);
				treeItem->setText(0, parent.c_str());
				treeItem->setText(1, "");
				uuidItem[parent] = treeItem;
				itemUuid[treeItem] = parent;
			}

			QIcon icon;
			if (type==VtkEpcTools::PROPERTY || type==VtkEpcTools::TIME_SERIES)
			{
				addTreeProperty(uuidItem[parent], parent, name.c_str(), uuid);
			}
			else
			{
				switch (type)
				{
				case VtkEpcTools::Resqml2Type::GRID_2D:
				{
					icon.addFile(QString::fromUtf8(":Grid2D.png"), QSize(), QIcon::Normal, QIcon::Off);
					break;
				}
				case VtkEpcTools::Resqml2Type::POLYLINE_SET:
				{
					icon.addFile(QString::fromUtf8(":Polyline.png"), QSize(), QIcon::Normal, QIcon::Off);
					break;
				}
				case VtkEpcTools::Resqml2Type::TRIANGULATED_SET:
				{
					icon.addFile(QString::fromUtf8(":Triangulated.png"), QSize(), QIcon::Normal, QIcon::Off);
					break;
				}
				case VtkEpcTools::Resqml2Type::WELL_TRAJ:
				{
					icon.addFile(QString::fromUtf8(":WellTraj.png"), QSize(), QIcon::Normal, QIcon::Off);
					break;
				}
				case VtkEpcTools::Resqml2Type::IJK_GRID:
				{
					icon.addFile(QString::fromUtf8(":IjkGrid.png"), QSize(), QIcon::Normal, QIcon::Off);
					break;
				}
				case VtkEpcTools::Resqml2Type::UNSTRUC_GRID:
				{
					icon.addFile(QString::fromUtf8(":UnstructuredGrid.png"), QSize(), QIcon::Normal, QIcon::Off);
					break;
				}
				case VtkEpcTools::Resqml2Type::SUB_REP:
				{
					icon.addFile(QString::fromUtf8(":SubRepresentation.png"), QSize(), QIcon::Normal, QIcon::Off);
					break;
				}
				default:
					break;
				}

				QTreeWidgetItem *treeItem = new QTreeWidgetItem();

				treeItem->setText(0, name.c_str());
				treeItem->setIcon(0, icon);
				treeItem->setCheckState(0, Qt::Unchecked);
				treeItem->setFlags(treeItem->flags() | Qt::ItemIsUserCheckable | Qt::ItemIsSelectable);


				uuidItem[parent]->addChild(treeItem);

				uuidItem[uuid] = treeItem;
				itemUuid[treeItem] = uuid;

				filenameToUuids[uuidToFilename[uuid]].push_back(uuid);
			}
		}
	}

}

//----------------------------------------------------------------------------
void PQSelectionPanel::addTreeProperty(QTreeWidgetItem *parent, std::string parentUuid,QString name, std::string uuid)
{
	auto etape =0;
	QButtonGroup *buttonGroup;
	if (uuidParentGroupButton.count(parentUuid) < 1)
	{
		buttonGroup = new QButtonGroup();
		buttonGroup->setExclusive(true);
		QRadioButton *radioButtonInvisible = new QRadioButton("invisible");
		buttonGroup->addButton(radioButtonInvisible, radioButtonCount++);
		mapUuidParentButtonInvisible[parentUuid] = radioButtonInvisible;
	}
	else
	{
		buttonGroup = uuidParentGroupButton[itemUuid[parent]];
	}
	QTreeWidgetItem *treeItem = new QTreeWidgetItem();
	QRadioButton *radioButton = new QRadioButton(name);
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
	/*
		else
		{
			QTreeWidgetItem *treeItem = new QTreeWidgetItem();
			treeItem->setText(0, QString(valuesPropertySet[i]->getTitle().c_str()));

			parent->addChild(treeItem);
		}
	 */
	connect(buttonGroup, SIGNAL(buttonClicked(int)), this, SLOT(checkedRadioButton(int)));
	uuidParentGroupButton[itemUuid[parent]] = buttonGroup;
}
//****************************************************************************
void PQSelectionPanel::deleteUUID(QTreeWidgetItem *item)
{
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
	return nullptr;
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
