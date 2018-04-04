#include "PQMetaDataPanel.h"
#include "ui_PQMetaDataPanel.h"

//#include <time.h>

#include "PQSelectionPanel.h"
#include "common/AbstractObject.h"
#include "common/EpcDocument.h"

#include <vtkInformation.h>

#include <qmessagebox.h>

namespace
{
	PQSelectionPanel* getPQSelectionPanel()
	{
	    // get multi-block inspector panel
	    PQSelectionPanel *panel = 0;
	    foreach(QWidget *widget, qApp->topLevelWidgets())
		{
			panel = widget->findChild<PQSelectionPanel *>();
	
			if(panel)
	        {
			    break;
			}
		}
		return panel;
	}
}

void PQMetaDataPanel::constructor()
{
  setWindowTitle("MetaData");
  QWidget* t_widget = new QWidget(this);
  Ui::panelMetaData ui;
  ui.setupUi(t_widget);
  setWidget(t_widget);

  tableViewModel = new QStandardItemModel(0,0,this);

  tableViewModel->setHorizontalHeaderItem(0,new QStandardItem(QString("name")));
  tableViewModel->setHorizontalHeaderItem(1,new QStandardItem(QString("value")));

  ui.tableView->setModel(tableViewModel);

  connect(getPQSelectionPanel(), SIGNAL(selectionName(std::string, std::string, common::EpcDocument *)), this, SLOT(displayMetaData(std::string, std::string, common::EpcDocument *)));
  this->setVisible(false);
}

void PQMetaDataPanel::displayMetaData(const std::string & fileName, const std::string & uuid, common::EpcDocument *pck)
{
	common::AbstractObject *object = pck->getResqmlAbstractObjectByUuid(uuid);
	
	tableViewModel->setItem(0,0,new QStandardItem(QString("Uuid")));
	tableViewModel->setItem(0,1,new QStandardItem(QString(object->getUuid().c_str())));
	
	tableViewModel->setItem(1,0,new QStandardItem(QString("Title")));
	tableViewModel->setItem(1,1,new QStandardItem(QString(object->getTitle().c_str())));
	tableViewModel->setItem(2,0,new QStandardItem(QString("Editor")));
	tableViewModel->setItem(2,1,new QStandardItem(QString(object->getEditor().c_str())));

	tableViewModel->setItem(3,0,new QStandardItem(QString("Creation")));
//	tableViewModel->setItem(3,1,new QStandardItem(QDateTime::fromTime_t(object->getCreation()).toString(Qt::TextDate));
//	tableViewModel->setItem(3,1,new QStandardItem(QString(ctime(object->getCreation()).c_str())));

	tableViewModel->setItem(4,0,new QStandardItem(QString("Originator")));
	tableViewModel->setItem(4,1,new QStandardItem(QString(object->getOriginator().c_str())));

	tableViewModel->setItem(5,0,new QStandardItem(QString("Description")));
	tableViewModel->setItem(5,1,new QStandardItem(QString(object->getDescription().c_str())));

	tableViewModel->setItem(6,0,new QStandardItem(QString("Last Update")));
//	tableViewModel->setItem(6,1,new QStandardItem(QDateTime::fromTime_t(object->getCreation()).toString(Qt::TextDate));
//	tableViewModel->setItem(6,1,new QStandardItem(QString(ctime(object->getLastUpdate()).c_str())));
	
	tableViewModel->setItem(7,0,new QStandardItem(QString("Format")));
	tableViewModel->setItem(7,1,new QStandardItem(QString(object->getFormat().c_str())));

	tableViewModel->setItem(8,0,new QStandardItem(QString("Descriptive Keywords")));
	tableViewModel->setItem(8,1,new QStandardItem(QString(object->getDescriptiveKeywords().c_str())));

	tableViewModel->setItem(9,0,new QStandardItem(QString("Part Name In Epc Document")));
	tableViewModel->setItem(9,1,new QStandardItem(QString(object->getPartNameInEpcDocument().c_str())));
}
