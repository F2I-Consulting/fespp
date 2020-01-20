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
#include "PQEtpPanel.h"
#include "ui_PQEtpPanel.h"

#include <vtkInformation.h>
#include <qmessagebox.h>

//include Fespp
#include "PQSelectionPanel.h"
#include "etp/EtpClientSession.h"

namespace {
	PQSelectionPanel* getPQSelectionPanel()
	{
		PQSelectionPanel *panel = nullptr;
		foreach(QWidget *widget, qApp->topLevelWidgets()) {
			panel = widget->findChild<PQSelectionPanel *>();
			if(panel != nullptr) {
				break;
			}
		}
		return panel;
	}
}

void PQEtpPanel::constructor()
{
	setWindowTitle("Etp");
	QWidget* t_widget = new QWidget(this);
	Ui::panelEtp ui;
	ui.setupUi(t_widget);
	setWidget(t_widget);

	etpStatus_Button = ui.status;
	QIcon icon;
	icon.addFile(QString::fromUtf8(":red_status.png"), QSize(), QIcon::Normal, QIcon::Off);
	etpStatus_Button->setIcon(icon);
	etp_connect = false;
	connect(etpStatus_Button, &QAbstractButton::released, [this]() {
		if (etp_connect) {
			//		delete etp_document;
			etp_connect = false;
			QIcon icon;
			icon.addFile(QString::fromUtf8(":red_status.png"), QSize(), QIcon::Normal, QIcon::Off);
			etpStatus_Button->setIcon(icon);
		}
	});

	// Create treeview button
	etpSendButton = ui.refresh;
	connect(etpSendButton, &QAbstractButton::released, this, &PQEtpPanel::handleButtonRefresh);
}

PQEtpPanel::~PQEtpPanel()
{
}

void PQEtpPanel::handleButtonRefresh()
{
	VtkEtpDocument etp_document(ipAddress, port, VtkEpcCommon::TreeView);

	// Wait for etp connection
	while (etp_document.getClientSession() == nullptr || etp_document.getClientSession()->isEtpSessionClosed()) {
	}

	QIcon icon;
	icon.addFile(QString::fromUtf8(":green_status.png"), QSize(), QIcon::Normal, QIcon::Off);
	etpStatus_Button->setIcon(icon);

	etp_document.createTree();
	if (etpSendButton->text() == "Create TreeView") {
		getPQSelectionPanel()->connectPQEtpPanel();
		etpSendButton->setText("Refresh");
	}

	// The tree creation will automatically close the session when done
	while (etp_document.getClientSession()->isWaitingForAnswer()) {
	}
	// etp_document can now be destroyed without risk
}

//******************************* Etp Document ************************************
void PQEtpPanel::etpClientConnect(const std::string & ipAddress, const std::string & port)
{
	this->ipAddress = ipAddress;
	this->port = port;

	handleButtonRefresh();
}

//----------------------------------------------------------------------------
void PQEtpPanel::setEtpTreeView(std::vector<VtkEpcCommon> tree)
{
	emit refreshTreeView(tree);
}
