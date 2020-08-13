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

// include system
#include <algorithm>
#include <sstream>
#include <stdexcept>

// include Qt
#include <QDateTime>
#include <QIcon>
#include <QHeaderView>
#include <QMenu>
#include <QList>

// include ParaView
#include <pqPropertiesPanel.h>
#include <pqPipelineSource.h>
#include <pqApplicationCore.h>
#include <pqServerManagerModel.h>
#include <pqServer.h>
#include <pqActiveObjects.h>

// include VTK
#include <vtkSMProxy.h>
#include <vtkSMPropertyHelper.h>

#include "PQToolsManager.h"
#ifdef WITH_ETP
#include "PQEtpPanel.h"
#endif

#include "TreeItem.h"
#include "VTK/VtkEpcDocumentSet.h"
#include "VTK/VtkEpcDocument.h"

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
	treeWidget->header()->setSectionResizeMode(0, QHeaderView::Stretch);
	treeWidget->expandToDepth(0);
	treeWidget->header()->close();

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

	save_time = 0;

	vtkEpcDocumentSet = new VtkEpcDocumentSet(0, 0, VtkEpcCommon::modeVtkEpc::TreeView);
	etpCreated = false;
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
		pickedBlocksEtp = static_cast<TreeItem*>(treeWidget->itemAt(pos))->getUuid();
		auto it = std::find (list_uri_etp.begin(), list_uri_etp.end(), pickedBlocksEtp);
		if (it != uri_subscribe.end() && !list_uri_etp.empty()) {  // subscribe
			menu->addAction(QString("subscribe/unsubscribe"), this, &PQSelectionPanel::subscribe_slot);
			menu->exec(treeWidget->mapToGlobal(pos));
		}
		else {
			for (const auto& file_name : getAllOpenedEpcFileNames()) {
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
	for (const auto& uuid : uuidsWellbore.keys()) {
		uuidItem[uuid]->setCheckState(0, select ? Qt::Checked : Qt::Unchecked);
		uuidsWellbore[uuid] = select;
	}
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
void PQSelectionPanel::clicSelection(QTreeWidgetItem* item, int) {
	auto pickedBlocks = static_cast<TreeItem*>(item)->getUuid();
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

void PQSelectionPanel::recursiveParentUncheck(QTreeWidgetItem* item)
{
	if (item == nullptr) {
		return;
	}

	TreeItem* parent = static_cast<TreeItem*>(item->parent());
	if (parent == nullptr || (parent->getDataObjectInfo() != nullptr && parent->getDataObjectInfo()->getType() == VtkEpcCommon::Resqml2Type::WELL_TRAJ)) {
		return;
	}

	const int siblingTreeWidgetItemCount = parent->childCount();
	for (int siblingIndex = 0; siblingIndex < siblingTreeWidgetItemCount; ++siblingIndex) {
		if (parent->child(siblingIndex)->checkState(0) == Qt::Checked) {
			// Do not uncheck the parent since one if its children is still checked
			return;
		}
	}

	// Uncheck the parent and operate the recursive operation
	parent->setCheckState(0, Qt::Unchecked);
	if (siblingTreeWidgetItemCount > 0) {
		parent->setData(0, Qt::CheckStateRole, QVariant());
	}
	if (parent->getDataObjectInfo() != nullptr) {
		toggleUuid(parent->getUuid(), false);
	}

	recursiveParentUncheck(parent);
}

void PQSelectionPanel::recursiveChildrenUncheck(QTreeWidgetItem* item)
{
	if (item == nullptr) {
		return;
	}

	const int childCount = item->childCount();
	for (int childIndex = 0; childIndex < childCount; ++childIndex) {
		auto child = item->child(childIndex);
		child->setCheckState(0, Qt::Unchecked);
		toggleUuid(static_cast<TreeItem*>(child)->getUuid(), false);
		recursiveChildrenUncheck(child);
	}
	if (static_cast<TreeItem*>(item)->getDataObjectInfo() == nullptr ||
		static_cast<TreeItem*>(item)->getDataObjectInfo()->getType() != VtkEpcCommon::Resqml2Type::WELL_TRAJ) {
		if (childCount > 0) {
			item->setData(0, Qt::CheckStateRole, QVariant());
		}
	}
}

//----------------------------------------------------------------------------
void PQSelectionPanel::onItemCheckedUnchecked(QTreeWidgetItem * item, int)
{
	const QSignalBlocker blocker(treeWidget);
	const std::string uuid = static_cast<TreeItem*>(item)->getUuid();
	if (item->checkState(0) == Qt::Checked) {
		while (item->parent() != nullptr && item->parent()->checkState(0) == Qt::Unchecked) {
			item = item->parent();
			if (static_cast<TreeItem*>(item)->getDataObjectInfo() != nullptr &&
				static_cast<TreeItem*>(item)->getDataObjectInfo()->getType() == VtkEpcCommon::Resqml2Type::WELL_TRAJ) {
				toggleUuid(static_cast<TreeItem*>(item)->getUuid(), true);
			}
			item->setCheckState(0, Qt::Checked);
		}
		toggleUuid(uuid, true);
	}
	else if (item->checkState(0) == Qt::Unchecked) {
		recursiveChildrenUncheck(item);
		recursiveParentUncheck(item);

		toggleUuid(uuid, false);
	}
}

//----------------------------------------------------------------------------
void PQSelectionPanel::deleteTreeView() {
	treeWidget->clear();

	uuidToFilename.clear();

	uuidItem.clear();

	pcksave.clear();

	if (vtkEpcDocumentSet != nullptr) {
		delete vtkEpcDocumentSet;
		vtkEpcDocumentSet = new VtkEpcDocumentSet(0, 0, VtkEpcCommon::modeVtkEpc::TreeView);
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
			for (const auto& ts : ts_displayed) {
				toggleUuid(ts_timestamp_to_uuid[ts][save_time], false);
				toggleUuid(ts_timestamp_to_uuid[ts][time], true);
			}
		}
		else {
			// load time properties
			for (const auto& ts : ts_displayed) {
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
		ts_displayed.erase(std::find(ts_displayed.begin(),
			ts_displayed.end(), uuid));
	}

	QList<time_t> list;
	QStringList large_list;

	for (const auto &uuid_displayed : ts_displayed) {
		foreach(time_t t, ts_timestamp_to_uuid[uuid_displayed].keys())
			list << t;
	}

	qSort(list.begin(), list.end());

	for (const auto& it : list) {
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

std::vector<std::string> PQSelectionPanel::getAllOpenedEpcFileNames() const
{
	std::vector<std::string> result;
	for (const auto& vtkEpcDoc : vtkEpcDocumentSet->getAllVtkEpcDocuments()) {
		result.push_back(vtkEpcDoc->getFileName());
	}
	return result;
}

void PQSelectionPanel::addFileName(const std::string & fileName) {
	// Block the signal of the tree to prevent some slots to be called during tree creation.
	// This is only for performance reason
	const QSignalBlocker blocker(treeWidget);

	const std::vector<std::string> allOpenedEpcFileNames = getAllOpenedEpcFileNames();
	if (std::find(allOpenedEpcFileNames.begin(), allOpenedEpcFileNames.end(), fileName) == allOpenedEpcFileNames.end()) {
		vtkEpcDocumentSet->addEpcDocument(fileName);
		uuidToFilename[fileName] = fileName;

		std::unordered_map<std::string, std::string> name_to_uuid;
		for (auto vtkEpcCommon : vtkEpcDocumentSet->getAllVtkEpcCommons()) {
			uuidToPipeName[vtkEpcCommon->getUuid()] = "EpcDocument";
			if (vtkEpcCommon->getTimeIndex() < 0) {
				populateTreeView(vtkEpcCommon);
			}
			else {
				if (name_to_uuid.find(vtkEpcCommon->getName()) == name_to_uuid.end()) {
					populateTreeView(vtkEpcCommon);
					name_to_uuid[vtkEpcCommon->getName()] = vtkEpcCommon->getUuid();
				}

				ts_timestamp_to_uuid[name_to_uuid[vtkEpcCommon->getName()]][vtkEpcCommon->getTimestamp()] =
						vtkEpcCommon->getUuid();
			}
		}
		treeWidget->sortItems(0, Qt::SortOrder::AscendingOrder);
	}
}

void PQSelectionPanel::checkUuid(const std::string & uuid) {
	const QSignalBlocker blocker(treeWidget);
	uuidItem[uuid]->setCheckState(0, Qt::Checked);
}

void PQSelectionPanel::populateTreeView(VtkEpcCommon const * vtkEpcCommon)
{
	const auto uuid = vtkEpcCommon->getUuid();
	if (!uuid.empty() && uuidItem[uuid] == nullptr) {
		auto parent = vtkEpcCommon->getParent();

		TreeItem* parentTreeWidgetItem = uuidItem[parent];

		// Check if the root tree item must be created
		if (parentTreeWidgetItem == nullptr) {
			if (vtkEpcCommon->getParentType() == VtkEpcCommon::Resqml2Type::PARTIAL) {
				vtkOutputWindowDisplayDebugText(("Impossible to resolve the partial object " + vtkEpcCommon->getParent() + "\n").c_str());
				return;
			}

			parentTreeWidgetItem = new TreeItem(treeWidget);
			parentTreeWidgetItem->setExpanded(true);
			parentTreeWidgetItem->setText(0, parent.c_str());

			// register the root tree widget item into the tree model maps
			uuidItem[parent] = parentTreeWidgetItem;
		}

		// Create the tree item
		TreeItem* treeItem = new TreeItem(vtkEpcCommon, parentTreeWidgetItem);
		if (vtkEpcCommon->getType() == VtkEpcCommon::Resqml2Type::WELL_TRAJ) {
			uuidsWellbore.insert(uuid, false);
		}

		// register the tree widget item into the tree model maps
		uuidItem[uuid] = treeItem;
	}
}

//********************************* Interfacing ******************************
//----------------------------------------------------------------------------
std::string PQSelectionPanel::searchSource(const std::string & uuid)
{
	return uuidToPipeName[uuid];
}

//----------------------------------------------------------------------------
void PQSelectionPanel::toggleUuid(const std::string & uuid, bool load)
{
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
			auto propertiesPanel = getpqPropertiesPanel();
			propertiesPanel->update();
			propertiesPanel->apply();
		}
	}
	if (uuidsWellbore.contains(uuid)) {
		uuidsWellbore[uuid] = load;
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
