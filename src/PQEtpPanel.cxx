#include "PQEtpPanel.h"
#include "ui_PQEtpPanel.h"

//include Fespp

#include <vtkInformation.h>
#include <qmessagebox.h>

#include "PQSelectionPanel.h"

#include <etp/EtpClientSession.h>

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

	this->etpStatus_Button = ui.status;
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
	while (etp_document.getClientSession() == nullptr) {
	}
	while (etp_document.getClientSession()->isEtpSessionClosed()) {
	}

	QIcon icon;
	icon.addFile(QString::fromUtf8(":green_status.png"), QSize(), QIcon::Normal, QIcon::Off);
	etpStatus_Button->setIcon(icon);

	etp_document.createTree();
	if (etpSendButton->text() == "Create TreeView"){
		getPQSelectionPanel()->connectPQEtpPanel();
		etpSendButton->setText("Refresh");
	}

	// The tree creation will automatically close the session when done
	while (etp_document.inloading()) {
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
