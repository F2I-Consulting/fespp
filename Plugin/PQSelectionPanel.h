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
#ifndef _PQSelectionPanel_h
#define _PQSelectionPanel_h

// include system
#include <string>
#include <vector>
#include <set>
#include <unordered_map>

#include <vtkSMProxy.h>

#include <QDockWidget>
#include <qprogressbar.h>
#include <qpushbutton.h>
#include <QTimer>
#include <QSlider>
#include <QComboBox>
#include <QLabel>
#include <QStringList>
#include <qradiobutton.h>
#include <QMap>
#include <QHash>
#include <QPointer>

#include <qprogressdialog.h>
#include <qfuturewatcher.h>

#include <pqTreeWidget.h>
#include <pqTreeWidgetItem.h>

#include "VTK/VtkEpcCommon.h"

namespace COMMON_NS
{
	class EpcDocument;
}

class pqPipelineSource;
class pqServer;
class VtkEpcDocumentSet;
class TreeItem;
class vtkPVXMLElement;
class vtkSMProxyLocator;

class PQSelectionPanel : public QDockWidget
{
  Q_OBJECT
  typedef QDockWidget Superclass;
  
public:
	PQSelectionPanel(const QString &t, QWidget* p = 0, Qt::WindowFlags f=0):
    Superclass(t, p, f) { this->constructor(); }
	
	PQSelectionPanel(QWidget *p=0, Qt::WindowFlags f=0):
    Superclass(p, f) { this->constructor(); }

	~PQSelectionPanel();

	/**
	* Open an epc document and create the treeWidget in panel.
	* @return informative message
	*/
	void addFileName(const std::string & fileName);

	void checkUuid(const std::string & uuid);

	void uuidKO(const std::string & uuid);

	/**
	* When a pipeline source is deleted
	*/
	void deleteTreeView();

#ifdef WITH_ETP
	void connectPQEtpPanel();
#endif

protected slots:

#ifdef WITH_ETP
	void setEtpTreeView(std::vector<VtkEpcCommon>);
#endif

	/**
	* When a row is checked.
	*/
	void onItemCheckedUnchecked(QTreeWidgetItem* item, int);

	void handleButtonAfter();
	void handleButtonBefore();
	void handleButtonPlay();
	void handleButtonPause();
	void handleButtonStop();

	void updateTimer();

	void timeChangedComboBox(int);
	void sliderMoved(int);

	//*************
	// right click
	//*************
	void treeCustomMenu(const QPoint &);
	/**
	* Toogle the status of all wells in the treeview
	*
	* @param select true if we want to select all wells, false otherwise
	*/
	void toggleAllWells(bool select);
	void subscribe_slot();
	void subscribeChildren_slot();

	void loadState(vtkPVXMLElement*, vtkSMProxyLocator*);
	void saveState(vtkPVXMLElement*);

#ifdef WITH_TEST
	void handleButtonTest();
#endif

private:

	std::string searchSource(const std::string & uuid);

	void populateTreeView(VtkEpcCommon const * vtkEpcCommon);
	void updateTimeSeries(const std::string & uuid, bool isnew);

	void constructor();
	
	/**
	* search a pqPipelineSource 
	*/
	virtual pqPipelineSource * findPipelineSource(const char *SMName);

	/**
	* Return the active server
	*/
    virtual pqServer * getActiveServer();

	/**
	* Recursively uncheck all parents of an item (i.e. if all sibling items are also unchecked).
	*/
	void recursiveParentUncheck(QTreeWidgetItem* item, vtkSMProxy* fesppReaderProxy);

	/**
	* Recursively uncheck all children of an item
	*/
	void recursiveChildrenUncheck(QTreeWidgetItem* item, vtkSMProxy* fesppReaderProxy);

	/**
	* @return all names of the currently opened EPC files in the plugin.
	*/
	std::vector<std::string> getAllOpenedFiles() const;
	
	pqTreeWidget *treeWidget;
	QPushButton *button_Time_After;
	QPushButton *button_Time_Before;
	QPushButton *button_Time_Play;
	QPushButton *button_Time_Pause;
	QPushButton *button_Time_Stop;
	QSlider *slider_Time_Step;
	QComboBox *time_series;
	QTimer *timer;
	bool time_Changed;

	std::unordered_map<std::string, std::string> uuidToPipeName;

	// Map getting a present tree widget item from its uuid
	std::unordered_map<std::string, TreeItem *> uuidItem;

	std::unordered_map<std::string, QMap<time_t, std::string>> ts_timestamp_to_uuid;

	std::set<std::string> uuid_checked;

	QMap<std::string, bool> uuidsWellbore;

	time_t save_time;

	std::vector<std::string> ts_displayed;

	VtkEpcDocumentSet* vtkEpcDocumentSet;
	bool etpCreated;

	// var: subscribe etp
	std::string pickedBlocksEtp;
	std::vector<std::string> uri_subscribe;
	std::vector<std::string> list_uri_etp;

#ifdef WITH_TEST
	QPushButton *button_test_perf;
#endif

};

#endif
