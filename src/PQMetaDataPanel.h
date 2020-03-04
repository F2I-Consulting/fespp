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
#ifndef _PQMetaDataPanel_h
#define _PQMetaDataPanel_h

#include <QDockWidget>
#include <qstandarditemmodel.h>

#include <fesapi/nsDefinitions.h>

namespace COMMON_NS
{
	class EpcDocument;
}

class PQMetaDataPanel : public QDockWidget
{
	Q_OBJECT
	typedef QDockWidget Superclass;
public:
	PQMetaDataPanel(const QString &t, QWidget* p = 0, Qt::WindowFlags f=0):
		Superclass(t, p, f) { this->constructor(); }
	PQMetaDataPanel(QWidget *p=0, Qt::WindowFlags f=0):
		Superclass(p, f) { this->constructor(); }
	~PQMetaDataPanel();
	
protected slots:
	/**
	* Display metadata in the panel.
	* @return 1 = sucessfull
	*/
	void displayMetaData(const std::string & fileName, const std::string & uuid, COMMON_NS::EpcDocument *);
  
private:
	void constructor();
  
	QStandardItemModel *tableViewModel;
};

#endif
