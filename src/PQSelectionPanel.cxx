/*-----------------------------------------------------------------------
Licensed to the Apache Software Foundation (ASF) under one
or more contributor license agreements.  See the NOTICE file
distributed with this work for additional information
regarding copyright ownership.  The ASF licenses this file
to you under the Apache License, Version 2.0 (the
"License"; you may not use this file except in compliance
with the License.  You may obtain a copy of the License at

  http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing,
software distributed under the License is distributed on an
"AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
KIND, either express or implied.  See the License for the
specific language governing permissions and limitations
under the License.
-----------------------------------------------------------------------*/
#include "ui_PQSelectionPanel.h"
#include "PQSelectionPanel.h"
#include "PQToolsManager.h"
#ifdef WITH_ETP
#include "PQEtpPanel.h"
#endif

// include API Resqml2
#include <fesapi/resqml2/AbstractValuesProperty.h>
#include <fesapi/resqml2/TimeSeries.h>
#include <fesapi/resqml2_0_1/AbstractIjkGridRepresentation.h>
#include <fesapi/resqml2_0_1/FaultInterpretation.h>
#include <fesapi/resqml2_0_1/Grid2dRepresentation.h>
#include <fesapi/resqml2_0_1/Horizon.h>
#include <fesapi/resqml2_0_1/HorizonInterpretation.h>
#include <fesapi/resqml2_0_1/PointSetRepresentation.h>
#include <fesapi/resqml2_0_1/PolylineSetRepresentation.h>
#include <fesapi/resqml2_0_1/SubRepresentation.h>
#include <fesapi/resqml2_0_1/TectonicBoundaryFeature.h>
#include <fesapi/resqml2_0_1/TriangulatedSetRepresentation.h>
#include <fesapi/resqml2_0_1/UnstructuredGridRepresentation.h>
#include <fesapi/resqml2_0_1/WellboreTrajectoryRepresentation.h>
#include <fesapi/resqml2_0_1/PropertyKindMapper.h>

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
#include <qbuttongroup.h>

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

namespace {
#ifdef WITH_ETP
PQEtpPanel* getPQEtpPanel() {
	PQEtpPanel *panel = nullptr;
	foreach(QWidget *widget, qApp->topLevelWidgets())
	{
		panel = widget->findChild<PQEtpPanel *>();

		if (panel!=nullptr) {
			break;
		}
	}
	return panel;
}
#endif

pqPropertiesPanel* getpqPropertiesPanel() {
	pqPropertiesPanel *panel = nullptr;
	foreach(QWidget *widget, qApp->topLevelWidgets())
	{
		panel = widget->findChild<pqPropertiesPanel *>();

		if (panel!=nullptr) {
			break;
		}
	}
	return panel;
}

}

//----------------------------------------------------------------------------
void PQSelectionPanel::constructor() {
	setWindowTitle("Selection widget");
	QWidget* t_widget = new QWidget(this);
	Ui::panelSelection ui;
	ui.setupUi(t_widget);
	setWidget(t_widget);
	treeWidget = ui.treeWidget;

	treeWidget->setStyleSheet(
			"QWidget::branch:has-siblings:!adjoins-item{border-image: url(:vline.png) 0;}"
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
	treeWidget->setContextMenuPolicy(Qt::CustomContextMenu);
	connect(ui.treeWidget, &QTreeWidget::customContextMenuRequested,
		this, &PQSelectionPanel::treeCustomMenu);

	connect(ui.treeWidget, &QTreeWidget::itemChanged,
		this, &PQSelectionPanel::onItemCheckedUnchecked);

	//*** timeplayer buttons
	button_Time_After = ui.button_Time_After;
	button_Time_Before = ui.button_Time_Before;
	button_Time_Play = ui.button_Time_Play;
	button_Time_Pause = ui.button_Time_Pause;
	button_Time_Stop = ui.button_Time_Stop;

	button_Time_After->setIcon(QIcon(QString::fromUtf8(":time-after.png")));
	button_Time_Before->setIcon(QIcon(QString::fromUtf8(":time-before.png")));
	button_Time_Play->setIcon(QIcon(QString::fromUtf8(":time-play.png")));
	button_Time_Pause->setIcon(QIcon(QString::fromUtf8(":time-pause.png")));
	button_Time_Stop->setIcon(QIcon(QString::fromUtf8(":time-stop.png")));

	connect(button_Time_After, &QAbstractButton::released,
		this, &PQSelectionPanel::handleButtonAfter);
	connect(button_Time_Before, &QAbstractButton::released,
		this, &PQSelectionPanel::handleButtonBefore);
	connect(button_Time_Play, &QAbstractButton::released,
		this, &PQSelectionPanel::handleButtonPlay);
	connect(button_Time_Pause, &QAbstractButton::released,
		this, &PQSelectionPanel::handleButtonPause);
	connect(button_Time_Stop, &QAbstractButton::released,
		this, &PQSelectionPanel::handleButtonStop);
	//***

	//*** combobox of the various Time_series
	time_series = ui.time_series;
	time_series->addItems(QStringList());

	connect(time_series, QOverload<int>::of(&QComboBox::currentIndexChanged),
		this, &PQSelectionPanel::timeChangedComboBox);
	//***

	//*** Timer Play option
	timer = new QTimer(this);

	connect(timer, &QTimer::timeout, this, &PQSelectionPanel::updateTimer);
	//***

	//*** slider of the various Time_series
	slider_Time_Step = ui.slider_Time_Step;
	slider_Time_Step->setTickInterval(10);
	slider_Time_Step->setSingleStep(1);
	slider_Time_Step->setMinimum(0);

	connect(slider_Time_Step, &QSlider::sliderMoved,
		this, &PQSelectionPanel::sliderMoved);

	//***

	radioButtonCount = 0;

	save_time = 0;

	vtkEpcDocumentSet = new VtkEpcDocumentSet(0, 0, VtkEpcCommon::TreeView);
	etpCreated = false;
	canLoad = false;
}

PQSelectionPanel::~PQSelectionPanel() {
	delete timer;
	delete vtkEpcDocumentSet;
}

//******************************* ACTIONS ************************************

//----------------------------------------------------------------------------
void PQSelectionPanel::treeCustomMenu(const QPoint & pos)
{
	if (treeWidget->itemAt(pos) != nullptr) {
		auto menu = new QMenu;
		pickedBlocksEtp = itemUuid[treeWidget->itemAt(pos)];
		auto it = std::find (list_uri_etp.begin(), list_uri_etp.end(), pickedBlocksEtp);
		if (it != uri_subscribe.end() && !list_uri_etp.empty()) {  // subscribe
			menu->addAction(QString("subscribe/unsubscribe"), this, &PQSelectionPanel::subscribe_slot);
			menu->exec(treeWidget->mapToGlobal(pos));
		}
		else {
			for (const auto& file_name : allFileName) {
				if (file_name == pickedBlocksEtp) {
					if (uuidsWellbore.keys(true).size() == uuidsWellbore.size()) {
						menu->addAction(QString("Unselect all wellbores"), this, [this] { toggleAllWells(false); });
					}
					else {
						menu->addAction(QString("Select all wellbores"), this, [this] { toggleAllWells(true); });
					}
					menu->exec(treeWidget->mapToGlobal(pos));
					break;
				}
			}
		}
	}
}

//----------------------------------------------------------------------------
void PQSelectionPanel::toggleAllWells(bool select)
{
	toggleUuid("allWell-" + pickedBlocksEtp, select);
	canLoad = false;
	for (const auto& uuid : uuidsWellbore.keys()) {
		uuidItem[uuid]->setCheckState(0, select ? Qt::Checked : Qt::Unchecked);
		uuidsWellbore[uuid] = select;
	}
	canLoad = true;
}

//----------------------------------------------------------------------------
void PQSelectionPanel::subscribe_slot() {
	auto it = std::find (uri_subscribe.begin(), uri_subscribe.end(), pickedBlocksEtp);
	if (it == uri_subscribe.end()) {  // subscribe
		uri_subscribe.push_back(pickedBlocksEtp);
		uuidItem[pickedBlocksEtp]->setIcon(1, QIcon(QString::fromUtf8(":pqEyeball16.png")));
	}
	else {  // unsubscribe
		uri_subscribe.erase(it);
		uuidItem[pickedBlocksEtp]->setIcon(1, QIcon());
	}
}

//----------------------------------------------------------------------------
void PQSelectionPanel::subscribeChildren_slot() {
	uuidItem[pickedBlocksEtp]->setIcon(1, QIcon(QString::fromUtf8(":pqEyeball16.png")));
}

//----------------------------------------------------------------------------
void PQSelectionPanel::clicSelection(QTreeWidgetItem* item, int column) {
	auto pickedBlocks = itemUuid[item];
	if (pickedBlocks != "root") {
		pqPipelineSource * source = findPipelineSource(searchSource(pickedBlocks).c_str());
		if (source != nullptr) {
			pqActiveObjects::instance().setActiveSource(source);
		}
		if (uuidToFilename[pickedBlocks] != pickedBlocks) {
			emit selectionName(uuidToFilename[pickedBlocks], pickedBlocks,
					pcksave[uuidToFilename[pickedBlocks]]);
		}
	}
}

//----------------------------------------------------------------------------
void PQSelectionPanel::onItemCheckedUnchecked(QTreeWidgetItem * item, int column)
{
	std::string uuid = itemUuid[item];
	if (!uuid.empty()) {
		if (item->checkState(0) == Qt::Checked) {
			toggleUuid(uuid, true);
		}
		else if (item->checkState(0) == Qt::Unchecked) {
			// Property exist
			if (uuidItem[uuid]->childCount() > 0 && mapUuidWithProperty[uuid] != "") {
				toggleUuid(mapUuidWithProperty[uuid], false);

				mapUuidWithProperty[uuid] = "";
				mapUuidParentButtonInvisible[uuid]->setChecked(true);
				uuidItem[uuid]->setData(0, Qt::CheckStateRole, QVariant());
			}
			toggleUuid(uuid, false);
		}
	}
}

//----------------------------------------------------------------------------
void PQSelectionPanel::deleteTreeView() {
	treeWidget->clear();

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

	radioButton_to_id.clear();

	mapUuidParentButtonInvisible.clear();

	pcksave.clear();
	uuidParent_to_groupButton.clear();
	radioButton_to_uuid.clear();

	if (vtkEpcDocumentSet != nullptr) {
		delete vtkEpcDocumentSet;
		vtkEpcDocumentSet = new VtkEpcDocumentSet(0, 0, VtkEpcCommon::TreeView);
	}
}

//----------------------------------------------------------------------------
void PQSelectionPanel::checkedRadioButton(int rbNo) {
	emit button_Time_Pause->released();
	if (radioButton_to_id.contains(rbNo)) {
		std::string uuid = radioButton_to_id[rbNo];
		std::string uuidParent = itemUuid[uuidParentItem[uuid]];

		canLoad = searchSource(uuidParent) == "EtpDocument";

		if (uuidParentItem[uuid]->checkState(0) != Qt::Checked) {
			uuidParentItem[uuid]->setCheckState(0, Qt::Checked);
		}

		std::string uuidOld = mapUuidWithProperty[uuidParent];
		canLoad = true;

		if (!uuidOld.empty()) {
			if (ts_timestamp_to_uuid.count(uuidOld) > 0) {
				time_t time = QDateTime::fromString(time_series->currentText()).toTime_t();

				toggleUuid(ts_timestamp_to_uuid[uuidOld][time], false);

				updateTimeSeries(uuidOld, false);
			}
			else {
				toggleUuid(uuidOld, false);
			}
		}
		mapUuidWithProperty[uuidParent] = uuid;

		if (ts_timestamp_to_uuid.count(uuid) > 0) {
			updateTimeSeries(uuid, true);

			time_t time = QDateTime::fromString(time_series->currentText()).toTime_t();

			toggleUuid(ts_timestamp_to_uuid[uuid][time], true);
		}
		else {
			toggleUuid(uuid, true);
		}
	}
}

//----------------------------------------------------------------------------
void PQSelectionPanel::handleButtonAfter() {
	auto idx = time_series->currentIndex();
	if (++idx < time_series->count()) {
		// change the TimeStamp
		time_series->setCurrentIndex(idx);
	}
	else {
		time_Changed = false;
	}
}

//----------------------------------------------------------------------------
void PQSelectionPanel::handleButtonBefore() {
	auto idx = time_series->currentIndex();
	if (idx != 0) {
		--idx;
		// change the TimeStamp
		time_series->setCurrentIndex(idx);
	}
}

//----------------------------------------------------------------------------
void PQSelectionPanel::handleButtonPlay() {
	timer->start(1000);
	time_Changed = true;
}

//----------------------------------------------------------------------------
void PQSelectionPanel::handleButtonPause() {
	timer->stop();
}

//----------------------------------------------------------------------------
void PQSelectionPanel::handleButtonStop() {
	time_series->setCurrentIndex(0);
	timer->stop();
}

//----------------------------------------------------------------------------
void PQSelectionPanel::updateTimer() {
	if (time_Changed) {
		emit button_Time_After->released();
	} else {
		timer->stop();
	}
}

//----------------------------------------------------------------------------
void PQSelectionPanel::timeChangedComboBox(int) {
	slider_Time_Step->setValue(time_series->currentIndex());

	time_t time = QDateTime::fromString(time_series->currentText()).toSecsSinceEpoch();

	if (time != save_time) {
		// remove old time properties
		if (save_time != 0) {
			for (auto &ts : ts_displayed) {
				toggleUuid(ts_timestamp_to_uuid[ts][save_time], false);
				toggleUuid(ts_timestamp_to_uuid[ts][time], true);
			}
		}
		else {
			// load time properties
			for (auto &ts : ts_displayed) {
				toggleUuid(ts_timestamp_to_uuid[ts][time], true);
			}
		}
		save_time = time;
	}
}

//----------------------------------------------------------------------------
void PQSelectionPanel::sliderMoved(int value) {
	time_series->setCurrentIndex(value);
}

//----------------------------------------------------------------------------
void PQSelectionPanel::updateTimeSeries(const std::string & uuid, bool newUuid) {
	if (newUuid) {
		ts_displayed.push_back(uuid);
	}
	else {
		auto index_to_delete = std::find(ts_displayed.begin(),
				ts_displayed.end(), uuid);
		ts_displayed.erase(index_to_delete);
	}

	QList<time_t> list;
	QStringList large_list;

	for (const auto &uuid_displayed : ts_displayed) {
		foreach(time_t t, ts_timestamp_to_uuid[uuid_displayed].keys())
			list << t;
	}

	qSort(list.begin(), list.end());

	for (auto &it : list) {
		QDateTime dt;
		dt.setSecsSinceEpoch(it);
		large_list << dt.toString();
	}

	large_list.removeDuplicates();

	disconnect(time_series, QOverload<int>::of(&QComboBox::currentIndexChanged),
		this, &PQSelectionPanel::timeChangedComboBox);
	time_series->clear();
	time_series->addItems(large_list);
	connect(time_series, QOverload<int>::of(&QComboBox::currentIndexChanged),
		this, &PQSelectionPanel::timeChangedComboBox);

	slider_Time_Step->setMaximum(large_list.size());
}

//****************************************************************************

//********************************* TreeView *********************************
//----------------------------------------------------------------------------
void PQSelectionPanel::addFileName(const std::string & fileName) {
	if (std::find(allFileName.begin(), allFileName.end(), fileName) == allFileName.end()) {
		vtkEpcDocumentSet->addEpcDocument(fileName);
		allFileName.push_back(fileName);
		uuidToFilename[fileName] = fileName;

		QMap<std::string, std::string> name_to_uuid;
		auto treeView = vtkEpcDocumentSet->getTreeView();
		for (auto &leaf : treeView) {
			uuidToPipeName[leaf.getUuid()] = "EpcDocument";
			if (leaf.getTimeIndex() < 0) {
				populateTreeView(leaf.getParent(), leaf.getParentType(),
						leaf.getUuid(), leaf.getName(),
						leaf.getType());
			}
			else {
				if (name_to_uuid.count(leaf.getName()) <= 0) {
					populateTreeView(leaf.getParent(),
							leaf.getParentType(), leaf.getUuid(),
							leaf.getName(), leaf.getType());
					name_to_uuid[leaf.getName()] = leaf.getUuid();
				}

				ts_timestamp_to_uuid[name_to_uuid[leaf.getName()]][leaf.getTimestamp()] =
						leaf.getUuid();
			}
		}
	}
}

void PQSelectionPanel::populateTreeView(const std::string & parent,
		VtkEpcCommon::Resqml2Type parentType, const std::string & uuid,
		const std::string & name, VtkEpcCommon::Resqml2Type type) {
	canLoad = false;
	if (!uuid.empty()) {
		if (uuidItem[uuid] == nullptr) {
			if (parentType == VtkEpcCommon::Resqml2Type::PARTIAL
					&& !uuidItem[parent]) {
			}
			else {
				if (uuidItem[parent] == nullptr) {
					QTreeWidgetItem *treeItem = new QTreeWidgetItem(treeWidget);

					treeItem->setExpanded(true);
					treeItem->setText(0, parent.c_str());
					treeItem->setText(1, "");
					uuidItem[parent] = treeItem;
					itemUuid[treeItem] = parent;
				}

				if (type == VtkEpcCommon::PROPERTY
						|| type == VtkEpcCommon::TIME_SERIES) {
					addTreeProperty(uuidItem[parent], parent, name, uuid);
				} else {

					QTreeWidgetItem *treeItem = new QTreeWidgetItem();

					treeItem->setText(0, name.c_str());
					treeItem->setFlags(
							treeItem->flags() | Qt::ItemIsSelectable);

					QIcon icon;
					switch (type) {
					case VtkEpcCommon::Resqml2Type::INTERPRETATION_1D: {
						icon.addFile(QString::fromUtf8(":Interpretation_1D.png"), QSize(),
								QIcon::Normal, QIcon::Off);
						break;
					}
					case VtkEpcCommon::Resqml2Type::INTERPRETATION_2D: {
						icon.addFile(QString::fromUtf8(":Interpretation_2D.png"), QSize(),
								QIcon::Normal, QIcon::Off);
						break;
					}
					case VtkEpcCommon::Resqml2Type::INTERPRETATION_3D: {
						icon.addFile(QString::fromUtf8(":Interpretation_3D.png"), QSize(),
								QIcon::Normal, QIcon::Off);
						break;
					}
					case VtkEpcCommon::Resqml2Type::GRID_2D: {
						icon.addFile(QString::fromUtf8(":Grid2D.png"), QSize(),
								QIcon::Normal, QIcon::Off);
						treeItem->setCheckState(0, Qt::Unchecked);
						break;
					}
					case VtkEpcCommon::Resqml2Type::POLYLINE_SET: {
						icon.addFile(QString::fromUtf8(":Polyline.png"),
								QSize(), QIcon::Normal, QIcon::Off);
						treeItem->setCheckState(0, Qt::Unchecked);
						break;
					}
					case VtkEpcCommon::Resqml2Type::TRIANGULATED_SET: {
						icon.addFile(QString::fromUtf8(":Triangulated.png"),
								QSize(), QIcon::Normal, QIcon::Off);
						treeItem->setCheckState(0, Qt::Unchecked);
						break;
					}
					case VtkEpcCommon::Resqml2Type::WELL_TRAJ: {
						icon.addFile(QString::fromUtf8(":WellTraj.png"),
								QSize(), QIcon::Normal, QIcon::Off);
						treeItem->setCheckState(0, Qt::Unchecked);
						uuidsWellbore.insert(uuid, false);
						break;
					}
					case VtkEpcCommon::Resqml2Type::WELL_MARKER_FRAME: {
						icon.addFile(QString::fromUtf8(":WellBoreFrameMarker.png"),
								QSize(), QIcon::Normal, QIcon::Off);
						treeItem->setCheckState(0, Qt::Unchecked);
						break;
					}
					case VtkEpcCommon::Resqml2Type::WELL_MARKER: {
						icon.addFile(QString::fromUtf8(":WellBoreMarker.png"),
								QSize(), QIcon::Normal, QIcon::Off);
						treeItem->setCheckState(0, Qt::Unchecked);
						break;
					}
					case VtkEpcCommon::Resqml2Type::WELL_FRAME: {
						icon.addFile(QString::fromUtf8(":WellBoreFrame.png"),
								QSize(), QIcon::Normal, QIcon::Off);
						treeItem->setCheckState(0, Qt::Unchecked);
						break;
					}
					case VtkEpcCommon::Resqml2Type::IJK_GRID: {
						icon.addFile(QString::fromUtf8(":IjkGrid.png"), QSize(),
								QIcon::Normal, QIcon::Off);
						treeItem->setCheckState(0, Qt::Unchecked);
						break;
					}
					case VtkEpcCommon::Resqml2Type::UNSTRUC_GRID: {
						icon.addFile(QString::fromUtf8(":UnstructuredGrid.png"),
								QSize(), QIcon::Normal, QIcon::Off);
						treeItem->setCheckState(0, Qt::Unchecked);
						break;
					}
					case VtkEpcCommon::Resqml2Type::SUB_REP: {
						icon.addFile(
								QString::fromUtf8(":SubRepresentation.png"),
								QSize(), QIcon::Normal, QIcon::Off);
						treeItem->setCheckState(0, Qt::Unchecked);
						break;
					}
					default:
						break;
					}

					treeItem->setIcon(0, icon);

					uuidItem[parent]->addChild(treeItem);

					uuidItem[uuid] = treeItem;
					itemUuid[treeItem] = uuid;

					filenameToUuids[uuidToFilename[uuid]].push_back(uuid);
				}
			}
		}
	}
	canLoad = true;
}

//----------------------------------------------------------------------------
void PQSelectionPanel::addTreeProperty(QTreeWidgetItem *parent,
		const std::string & parentUuid, const std::string & name,
		const std::string & uuid) {
	QButtonGroup *buttonGroup;
	if (uuidParent_to_groupButton.count(parentUuid) < 1) {
		buttonGroup = new QButtonGroup();
		buttonGroup->setExclusive(true);
		QRadioButton *radioButtonInvisible = new QRadioButton("invisible");
		buttonGroup->addButton(radioButtonInvisible, radioButtonCount++);
		mapUuidParentButtonInvisible[parentUuid] = radioButtonInvisible;
		connect(buttonGroup, QOverload<int>::of(&QButtonGroup::buttonClicked),
			this, &PQSelectionPanel::checkedRadioButton);
	}
	else {
		buttonGroup = uuidParent_to_groupButton[itemUuid[parent]];
	}
	QTreeWidgetItem *treeItem = new QTreeWidgetItem();
	QRadioButton *radioButton = new QRadioButton(QString::fromStdString(name));
	radioButton_to_id[radioButtonCount] = uuid;
	buttonGroup->addButton(radioButton, radioButtonCount);
	parent->addChild(treeItem);
	treeWidget->setItemWidget(treeItem, 0, radioButton);
	uuidItem[uuid] = treeItem;
	uuidParentItem[uuid] = parent;
	itemUuid[treeItem] = uuid;
	uuidToFilename[uuid] = uuidToFilename[itemUuid[parent]];

	radioButtonCount++;
	radioButton_to_uuid[radioButton] = uuid;

	if (parent->checkState(0)!=Qt::Checked) {
		parent->setData(0, Qt::CheckStateRole, QVariant());
	}

	uuidParent_to_groupButton[itemUuid[parent]] = buttonGroup;
}

//********************************* Interfacing ******************************
//----------------------------------------------------------------------------
std::string PQSelectionPanel::searchSource(const std::string & uuid) {
	return uuidToPipeName[uuid];
}

//----------------------------------------------------------------------------
void PQSelectionPanel::toggleUuid(const std::string & uuid, bool load) {
	if (canLoad) {
		const std::string pipe_name = uuid.find("allWell-") != std::string::npos
			? "EpcDocument"
			: searchSource(uuid);
		pqPipelineSource * source = findPipelineSource(pipe_name.c_str());
		if (source != nullptr) {
			pqActiveObjects *activeObjects = &pqActiveObjects::instance();
			activeObjects->setActiveSource(source);

			auto fesppReader = PQToolsManager::instance()->getFesppReader(pipe_name);

			if (fesppReader != nullptr) {
				activeObjects->setActiveSource(fesppReader);

				// add uuid to property panel
				vtkSMProxy* fesppReaderProxy = fesppReader->getProxy();

				vtkSMPropertyHelper(fesppReaderProxy, "UuidList").SetStatus(
						uuid.c_str(), load ? 1 : 0);

				fesppReaderProxy->UpdatePropertyInformation();
				fesppReaderProxy->UpdateVTKObjects();
				getpqPropertiesPanel()->update();
				getpqPropertiesPanel()->apply();
			}
		}
		if (uuidsWellbore.contains(uuid)) {
			uuidsWellbore[uuid] = load;
		}
	}
}

//****************************************************************************

//************************** Pipeline & Server *******************************

//----------------------------------------------------------------------------
pqPipelineSource * PQSelectionPanel::findPipelineSource(const char *SMName) {
	pqApplicationCore *core = pqApplicationCore::instance();
	pqServerManagerModel *smModel = core->getServerManagerModel();

	QList<pqPipelineSource*> sources = smModel->findItems<pqPipelineSource*>(
			getActiveServer());
	foreach(pqPipelineSource *s, sources)
	{
		if (strcmp(s->getSMName().toStdString().c_str(), SMName) == 0)
			return s;
	}
	return nullptr;
}

//----------------------------------------------------------------------------
pqServer * PQSelectionPanel::getActiveServer() {
	return pqApplicationCore::instance()->getServerManagerModel()->getItemAtIndex<pqServer*>(0);
}

//----------------------------------------------------------------------------
void PQSelectionPanel::uuidKO(const std::string & uuid) {
	uuidItem[uuid]->setDisabled(true);
	for (int idx_child = 0; idx_child < uuidItem[uuid]->childCount(); ++idx_child) {
		uuidItem[uuid]->child(idx_child)->setHidden(true);
	}
}

//*************************************
//*     ETP
//*************************************
#ifdef WITH_ETP
//----------------------------------------------------------------------------
void PQSelectionPanel::setEtpTreeView(std::vector<VtkEpcCommon> treeView) {
	QMap<std::string, std::string> name_to_uuid;
	for (auto &leaf : treeView) {
		uuidToPipeName[leaf.getUuid()] = "EtpDocument";
		if (leaf.getTimeIndex() < 0) {
			list_uri_etp.push_back(leaf.getUuid());
			populateTreeView(leaf.getParent(), leaf.getParentType(),
					leaf.getUuid(), leaf.getName(), leaf.getType());
		}
		else {
			if (name_to_uuid.count(leaf.getName()) <= 0) {
				list_uri_etp.push_back(leaf.getUuid());
				populateTreeView(leaf.getParent(), leaf.getParentType(),
						leaf.getUuid(), leaf.getName(),
						leaf.getType());
				name_to_uuid[leaf.getName()] = leaf.getUuid();
			}
			ts_timestamp_to_uuid[name_to_uuid[leaf.getName()]][leaf.getTimestamp()] =
					leaf.getUuid();
		}
	}

	if (!etpCreated) {
		pqPipelineSource * source = findPipelineSource("EtpDocument");
		if (source != nullptr) {
			pqActiveObjects::instance().setActiveSource(source);

			auto fesppReader = PQToolsManager::instance()->getFesppReader("EtpDocument");
			if (fesppReader) {
				pqActiveObjects *activeObjects = &pqActiveObjects::instance();
				activeObjects->setActiveSource(fesppReader);

				// add file to property
				vtkSMProxy* fesppReaderProxy = fesppReader->getProxy();

				vtkSMPropertyHelper(fesppReaderProxy, "UuidList").SetStatus(
						"connect", 0);
				etpCreated = true;

				fesppReaderProxy->UpdatePropertyInformation();
				fesppReaderProxy->UpdateVTKObjects();
				getpqPropertiesPanel()->update();
				getpqPropertiesPanel()->apply();
			}
		}
	}
}

void PQSelectionPanel::connectPQEtpPanel() {
	qRegisterMetaType<std::vector<VtkEpcCommon> >("std::vector<VtkEpcCommon>");
	connect(getPQEtpPanel(), &PQEtpPanel::refreshTreeView, this,
			&PQSelectionPanel::setEtpTreeView);
}
#endif
