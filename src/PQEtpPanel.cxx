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
		PQSelectionPanel *panel = 0;
		foreach(QWidget *widget, qApp->topLevelWidgets()) {
			panel = widget->findChild<PQSelectionPanel *>();
			if(panel) {
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

	EtpStatus_Button = ui.status;
	QIcon icon;
	icon.addFile(QString::fromUtf8(":red_status.png"), QSize(), QIcon::Normal, QIcon::Off);
	EtpStatus_Button->setIcon(icon);
	etp_connect = false;
	connect(EtpStatus_Button, &QAbstractButton::released, [this]() {
		if (etp_connect) {
			//		delete etp_document;
			etp_connect = false;
			QIcon icon;
			icon.addFile(QString::fromUtf8(":red_status.png"), QSize(), QIcon::Normal, QIcon::Off);
			EtpStatus_Button->setIcon(icon);
		}
	});

	// Create treeview button
	EtpSendButton = ui.refresh;
	connect(EtpSendButton, &QAbstractButton::released, this, &PQEtpPanel::handleButtonRefresh);
}

PQEtpPanel::~PQEtpPanel()
{
}

void PQEtpPanel::handleButtonRefresh()
{
	VtkEtpDocument etp_document(ipAddress, port, VtkEpcCommon::TreeView);

	// Wait for etp connection
	cout << "av attente getClientSession()\n";
	while (etp_document.getClientSession() == nullptr) {
	}
	cout << "av attente getClientSession() OK\n";
	while (etp_document.getClientSession()->isEtpSessionClosed()) {
	}
	cout << "av attente getClientSession()->isEtpSessionClosed() OK\n";

	QIcon icon;
	icon.addFile(QString::fromUtf8(":green_status.png"), QSize(), QIcon::Normal, QIcon::Off);
	EtpStatus_Button->setIcon(icon);

	etp_document.createTree();
	if (EtpSendButton->text() == "Create TreeView"){
		getPQSelectionPanel()->connectPQEtpPanel();
		EtpSendButton->setText("Refresh");
	}

	// The tree creation will automatically close the session when done
	cout << "av attente etp_document.getClientSession()->isWebSocketSessionClosed()\n";
	while (etp_document.inloading()) {
	}
	cout << "av attente etp_document.getClientSession()->isWebSocketSessionClosed() OK\n";
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
