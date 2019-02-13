/*-----------------------------------------------------------------------
Copyright F2I-CONSULTING, (2014)

cedric.robert@f2i-consulting.com

This software is a computer program whose purpose is to display data formatted using Energistics standards.

This software is governed by the CeCILL license under French law and
abiding by the rules of distribution of free software.  You can  use, 
modify and/ or redistribute the software under the terms of the CeCILL
license as circulated by CEA, CNRS and INRIA at the following URL
"http://www.cecill.info". 

As a counterpart to the access to the source code and  rights to copy,
modify and redistribute granted by the license, users are provided only
with a limited warranty  and the software's author,  the holder of the
economic rights,  and the successive licensors  have only  limited
liability.  

In this respect, the user's attention is drawn to the risks associated
with loading,  using,  modifying and/or developing or reproducing the
software by the user in light of its specific status of free software,
that may mean  that it is complicated to manipulate,  and  that  also
therefore means  that it is reserved for developers  and  experienced
professionals having in-depth computer knowledge. Users are therefore
encouraged to load and test the software's suitability as regards their
requirements in conditions enabling the security of their systems and/or 
data to be ensured and,  more generally, to use and operate it in the 
same conditions as regards security.  

The fact that you are presently reading this means that you have had
knowledge of the CeCILL license and that you accept its terms.
-----------------------------------------------------------------------*/

#ifndef _PQSelectionPanel_h
#define _PQSelectionPanel_h

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

// include system
//#include <unordered_map>
#include <string>
#include <vector>

#include "VTK/VtkEpcCommon.h"

class pqPipelineSource;
class pqServer;
//class pqOutputPort;
//class pqView;
class VtkEpcDocumentSet;

#include "common/EpcDocument.h"

namespace resqml2
{
	class AbstractFeatureInterpretation;
	class AbstractValuesProperty;
	class SubRepresentation;

}

namespace resqml2_0_1
{
	class PolylineSetRepresentation;
	class AbstractFeatureInterpretation;
	class TriangulatedSetRepresentation;
	class Grid2dRepresentation;
	class AbstractIjkGridRepresentation;
	class UnstructuredGridRepresentation;
	class WellboreTrajectoryRepresentation;
}

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
	void selectionName(const std::string &, const std::string &, common::EpcDocument *);

protected slots:

//#ifdef WITH_ETP
	void setEtpTreeView(std::vector<VtkEpcCommon>);
//#endif
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

private:

	std::string searchSource(const std::string & uuid);

	void populateTreeView(const std::string &  parent, VtkEpcCommon::Resqml2Type parentType, const std::string &  uuid, const std::string &  name, VtkEpcCommon::Resqml2Type type, const std::string & pipeOrigin);
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

	std::vector<std::string> displayUuid;

	// radio-button
	int radioButtonCount;
	QMap<int, std::string> radioButton_to_id;
	QMap<std::string, QRadioButton*> mapUuidParentButtonInvisible;


	QMap<std::string, common::EpcDocument *> pcksave;

	QMap<std::string, QButtonGroup *> uuidParent_to_groupButton;
	QMap<QAbstractButton *, std::string> radioButton_to_uuid;

	QMap<std::string, QMap<time_t, std::string>> ts_timestamp_to_uuid;

	time_t save_time;

	std::vector<std::string> ts_displayed;

	VtkEpcDocumentSet* vtkEpcDocumentSet;

	bool debug_verif;
};

#endif
