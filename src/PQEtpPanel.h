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
#ifndef _PQEtpPanel_h
#define _PQEtpPanel_h

#include <etp/VtkEtpDocument.h>
#include <QDockWidget>
#include <QComboBox>
#include <QLineEdit>
#include <QTextBrowser>
#include <QPushButton>
#include <QStringList>

class PQEtpPanel : public QDockWidget
{
	Q_OBJECT
	typedef QDockWidget Superclass;
public:
	PQEtpPanel(const QString &t, QWidget* p = 0, Qt::WindowFlags f=0):
		Superclass(t, p, f) { this->constructor(); }
	PQEtpPanel(QWidget *p=0, Qt::WindowFlags f=0):
		Superclass(p, f) { this->constructor(); }

	~PQEtpPanel();
	
	void etpClientConnect(const std::string & ipAddress, const std::string & port);

	/**
	* Emit a signal indicating to refresh the tree view
	*/
	void setEtpTreeView(std::vector<VtkEpcCommon>);

signals:
	void refreshTreeView(std::vector<VtkEpcCommon>);

protected slots:
	void handleButtonRefresh();

private:
	void constructor();

	QPushButton *etpSendButton;
	QPushButton *etpStatus_Button;

	std::string ipAddress_;
	std::string port_;

	bool etp_connect;
};

#endif

