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

#include <QDockWidget>
#include <qprogressbar.h>
#include <qpushbutton.h>
#include <QTimer>
#include <QSlider>
#include <QComboBox>
#include <QLabel>
#include <QStringList>
#include <qtreewidget.h>
#include <qradiobutton.h>
#include <QMap>
#include <QHash>
#include <QPointer>

#include <qprogressdialog.h>
#include <qfuturewatcher.h>

#include <fesapi/nsDefinitions.h>

#include "VTK/VtkEpcCommon.h"

namespace COMMON_NS
{
	class EpcDocument;
}

class pqPipelineSource;
class pqServer;
class VtkEpcDocumentSet;

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

	void uuidKO(const std::string & uuid);

	/**
	* When a pipeline source is deleted
	*/
	void deleteTreeView();

#ifdef WITH_ETP
	void connectPQEtpPanel();
#endif

signals:
	/**
	* Signal emit when a item is selected
	*/
	void selectionName(const std::string &, const std::string &, COMMON_NS::EpcDocument*);

protected slots:

#ifdef WITH_ETP
	void setEtpTreeView(std::vector<VtkEpcCommon>);
#endif
	/**
	* When a line is selected.
	*/
	void clicSelection(QTreeWidgetItem* item,int column);

	/**
	* When a row is checked.
	*/
	void onItemCheckedUnchecked(QTreeWidgetItem*,int);

	/**
	* When a row is checked.
	*/
	void checkedRadioButton(int);

	void handleButtonAfter();
	void handleButtonBefore();
	void handleButtonPlay();
	void handleButtonPause();
	void handleButtonStop();

	void updateTimer();

	void timeChangedComboBox(int);
	void sliderMoved(int);

	// right click
	void treeCustomMenu(const QPoint &);
	void selectAllWell();
	void unselectAllWell();
	void subscribe_slot();
	void subscribeChildren_slot();

#ifdef WITH_TEST
	void handleButtonTest();
#endif

private:

	std::string searchSource(const std::string & uuid);

	void populateTreeView(const std::string &  parent, VtkEpcCommon::Resqml2Type parentType, const std::string &  uuid, const std::string &  name, VtkEpcCommon::Resqml2Type type);
	void updateTimeSeries(const std::string & uuid, bool isnew);
	void deleteUUID(QTreeWidgetItem *item);

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
	* Add the Properties
	*/
	void addTreeProperty(QTreeWidgetItem *parent, const std::string & parentUUid, const std::string & name, const std::string & uuid);

	/**
	* Load representation/property uuid's
	*/
	void loadUuid(const std::string & uuid);
	
	/**
	* Remove representation/property uuid's
	*/
	void removeUuid(const std::string & uuid);
	
	QTreeWidget *treeWidget;
	QPushButton *button_Time_After;
	QPushButton *button_Time_Before;
	QPushButton *button_Time_Play;
	QPushButton *button_Time_Pause;
	QPushButton *button_Time_Stop;
	QSlider *slider_Time_Step;
	QComboBox *time_series;
	QTimer *timer;
	int timerCount;

	bool time_Changed;

	unsigned int indexFile;
	std::vector<std::string> allFileName;
	QMap<std::string, std::string> uuidToFilename;
	QMap<std::string, std::string> uuidToPipeName;
	QMap<std::string, std::vector<std::string> > filenameToUuids;
	QMap<std::string, std::vector<std::string> > filenameToUuidsPartial;

	QMap<std::string, QTreeWidgetItem *> uuidItem;
	QMap<QTreeWidgetItem*,std::string> itemUuid;
	QMap<std::string, QTreeWidgetItem *> uuidParentItem;
	
	// properties
	QMap< std::string, std::vector<std::string> > mapUuidProperty;
	
	QMap<std::string, std::string> mapUuidWithProperty;

	// radio-button
	int radioButtonCount;
	QMap<int, std::string> radioButton_to_id;
	QMap<std::string, QRadioButton*> mapUuidParentButtonInvisible;
	bool canLoad;

	QMap<std::string, COMMON_NS::EpcDocument *> pcksave;

	QMap<std::string, QButtonGroup *> uuidParent_to_groupButton;
	QMap<QAbstractButton *, std::string> radioButton_to_uuid;

	QMap<std::string, QMap<time_t, std::string>> ts_timestamp_to_uuid;

	QMap<std::string, bool> uuidsWellbore;

	time_t save_time;

	std::vector<std::string> ts_displayed;

	VtkEpcDocumentSet* vtkEpcDocumentSet;

	bool debug_verif;
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
