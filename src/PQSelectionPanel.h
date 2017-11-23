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
#include <qtreewidget.h>
#include <qradiobutton.h>
#include <QMap>
#include <QPointer>

#include <qprogressdialog.h>
#include <qfuturewatcher.h>

// include system
//#include <unordered_map>
#include <string>
#include <vector>

#include <vtkSmartPointer.h> 
#include <vtkMultiBlockDataSet.h>

class pqPipelineSource;
class pqServer;
class Fespp;
class pqOutputPort;
class pqView;

#include "EpcDocument.h"

class VtkEpcDocument;

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

	/**
	* Open an epc document and create the treeWidget in panel.
	* @return informative message
	*/
	VtkEpcDocument* addFileName(const std::string & fileName,Fespp *plugin, common::EpcDocument *pck);

	bool canAddFile(const char* fileName);

	void uuidKO(const std::string & uuid);

	/**
	* method : getOutput
	* variable : --
	* return the vtkMultiBlockDataSet.
	*/
	vtkSmartPointer<vtkMultiBlockDataSet> getOutput(const std::string & FileName) const;

signals:
	/**
	* Signal emit when a item is selected
	*/
	void selectionName(std::string, std::string, common::EpcDocument *);

protected slots:

	/**
	* When a line is selected.
	*/
	void clicSelection(QTreeWidgetItem* item,int column);

		/**
	* Check button.
	*/
	void checkButton();

			/**
	* unCheck button.
	*/
	void uncheckButton();

		/**
	* When a line is selected.
	*/
	void doubleClicSelection(QTreeWidgetItem* item,int column);
	
	/**
	* When a row is checked.
	*/
	void onItemCheckedUnchecked(QTreeWidgetItem*,int);

	/**
	* When a row is checked.
	*/
	void checkedRadioButton(int);

	/**
	* When a pipeline source is deleted
	*/
	void deletePipelineSource(pqPipelineSource*);

	/** 
	* Called to hide the block. the action which emits the signal will
	* contain the block index in its data().
	*/
	void hideBlock();
	
	/** 
	* Called to show the block. the action which emits the signal will
	* contain the block index in its data().
	*/
	void showBlock();

	/**
	* Called to show only the selected block. the action which emits the
	* signal will contain the block index in its data().
	*/
	void showOnlyBlock();

	/**
	* Called to show all blocks.
	*/
	void showAllBlocks();
	
private:
	void deleteUUID(QTreeWidgetItem *item);

	void constructor();
	
	/**
	* delete treeWidget fileName in panel
	*/
	void deleteFileName(const std::string & fileName);

	/**
	* search a pqPipelineSource 
	*/
	virtual pqPipelineSource * findPipelineSource(const char *SMName);

	/**
	* Return the active server
	*/
    virtual pqServer * getActiveServer();

	/**
	* Add the first element of epcDocument tree
	*/
	void addTreeRoot(QString name, QString description);

	/**
	* Add Feature/Interpretation
	*/
    void addTreeChild(QTreeWidgetItem *parent, QString name, std::string uuid);
	
	/**
	* Add Polylines Representation in TreeView
	*/
	void addTreePolylines(const std::string & fileName, std::vector<resqml2_0_1::PolylineSetRepresentation*> polylines, VtkEpcDocument * vtkEpcDocument);
	
	/**
	* Add triangulated Representation in TreeView
	*/
	void addTreeTriangulated(const std::string & fileName, std::vector<resqml2_0_1::TriangulatedSetRepresentation*> triangulated, VtkEpcDocument * vtkEpcDocument);

	/**
	* Add grid2D Representation in TreeView
	*/
	void addTreeGrid2D(const std::string & fileName, std::vector<resqml2_0_1::Grid2dRepresentation*> grid2D, VtkEpcDocument * vtkEpcDocument);

	/**
	* Add ijkGrid Representation in TreeView
	*/
	void addTreeIjkGrid(const std::string & fileName, std::vector<resqml2_0_1::AbstractIjkGridRepresentation*> ijkGrid, VtkEpcDocument * vtkEpcDocument);

	/**
	* Add unstructuredGrid Representation in TreeView
	*/
	void addTreeUnstructuredGrid(const std::string & fileName, std::vector<resqml2_0_1::UnstructuredGridRepresentation*> unstructuredGrid, VtkEpcDocument * vtkEpcDocument);

	/**
	* Add WellboreTrajectoryRepresentation in TreeView
	*/
	void addTreeWellboreTrajectory(const std::string & fileName, std::vector<resqml2_0_1::WellboreTrajectoryRepresentation*> wellCubicParmLineTraj, VtkEpcDocument * vtkEpcDocument);

	/**
	* Add SubRepresentation in TreeView
	*/
	void addTreeSubRepresentation(const std::string & fileName, std::vector<resqml2::SubRepresentation*> subRepresentation, VtkEpcDocument * vtkEpcDocument);

	/**
	* Add Feature and Interpretation in TreeView
	*/
	std::string addFeatInterp(const std::string & fileName, resqml2::AbstractFeatureInterpretation * interpretation, VtkEpcDocument * vtkEpcDocument);
	/**
	* Add Representation in TreeView
	*/
    void addTreeRepresentation(QTreeWidgetItem *parent, QString name, std::string uuid, QIcon icon);

	/**
	* Add the Properties
	*/
	void addTreeProperty(QTreeWidgetItem *parent, std::vector<resqml2::AbstractValuesProperty*> valuesPropertySet, VtkEpcDocument * vtkEpcDocument);

	/**
	* Load representation/property uuid's
	*/
	void loadUuid(std::string uuid);
	
	/**
	* Remove representation/property uuid's
	*/
	void removeUuid(std::string uuid, VtkEpcDocument *);
	
	QTreeWidget *treeWidget;
	QPushButton *checkAllButton;
	QPushButton *uncheckAllButton;
	QProgressBar *progressBar;

	unsigned int indexFile;
	std::vector<std::string> allFileName;
	QMap<VtkEpcDocument *, Fespp*> fespp;
	QMap<std::string, std::string> uuidToFilename;
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
	QMap<int, std::string> mapRadioButtonNo;
	QMap<std::string, QRadioButton*> mapUuidParentButtonInvisible;

	// hide/show block
	std::string pickedBlocks;
	std::vector<std::string> uuidVisible;
	std::vector<std::string> uuidInvisible;

	std::vector<std::string> uuidCheckable;

	QPointer<pqOutputPort> OutputPort;

	QMap<std::string, common::EpcDocument *> pcksave;

	QProgressDialog dlg;
	QFutureWatcher<void> futureWatcher;

	QMap<std::string, VtkEpcDocument *> epcDocument;
	QMap<std::string, VtkEpcDocument *> partialRepresentation;
	QMap<std::string, QButtonGroup *> uuidParentGroupButton;
	QMap<QAbstractButton *, std::string> radioButtonToUuid;
};

#endif
