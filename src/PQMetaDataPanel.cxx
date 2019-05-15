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

PQMetaDataPanel::~PQMetaDataPanel()
{
	delete tableViewModel;
}

void PQMetaDataPanel::displayMetaData(const std::string & fileName, const std::string & uuid, common::EpcDocument *pck)
{
/*	common::AbstractObject *object = pck->getResqmlAbstractObjectByUuid(uuid);
	
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
	tableViewModel->setItem(9,1,new QStandardItem(QString(object->getPartNameInEpcDocument().c_str())));*/
}
