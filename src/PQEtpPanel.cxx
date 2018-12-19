#include "PQEtpPanel.h"
#include "ui_PQEtpPanel.h"

//include Fespp

#include <vtkInformation.h>
#include <qmessagebox.h>

#include "PQSelectionPanel.h"



namespace
{
PQEtpPanel* getPQEtpPanel()
{
	// get multi-block inspector panel
	PQEtpPanel *panel = 0;
	foreach(QWidget *widget, qApp->topLevelWidgets())
	{
		panel = widget->findChild<PQEtpPanel *>();

		if(panel)
		{
			break;
		}
	}
	return panel;
}

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

//void startEtpDocument(std::string ipAddress, std::string port)
//{
//	auto etp_document = new VtkEtpDocument(ipAddress, port, VtkEpcCommon::TreeView);
//	getPQEtpPanel()->setVtkEtpDocuement(etp_document);
//}
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
	connect(EtpStatus_Button, SIGNAL (released()), this, SLOT (handleButtonStatus()));

	EtpSendButton = ui.refresh;
	connect(EtpSendButton, SIGNAL (released()), this, SLOT (handleButtonRefresh()));


}

//******************************* ACTIONS ************************************
//----------------------------------------------------------------------------
void PQEtpPanel::handleButtonStatus()
{
	if (etp_connect)
	{
		delete etp_document;
		etp_connect=false;
		QIcon icon;
		icon.addFile(QString::fromUtf8(":red_status.png"), QSize(), QIcon::Normal, QIcon::Off);
		EtpStatus_Button->setIcon(icon);
	}
}
void PQEtpPanel::handleButtonRefresh()
{
	cout << "PQEtpPanel::handleButtonRefresh()" << endl;
	if (etp_connect)
	{
		etp_document->createTree();
		getPQSelectionPanel()->connectPQEtpPanel();
		EtpSendButton->setText("Refresh");
		etp_connect=false;
		QIcon icon;
	}


}

//******************************* Etp Document ************************************
void PQEtpPanel::etpClientConnect(const std::string & ipAddress, const std::string & port)
{
	etp_document = new VtkEtpDocument(ipAddress, port, VtkEpcCommon::TreeView);

	etp_connect=true;
	QIcon icon;
	icon.addFile(QString::fromUtf8(":green_status.png"), QSize(), QIcon::Normal, QIcon::Off);
	EtpStatus_Button->setIcon(icon);
}


void PQEtpPanel::setEtpTreeView(std::vector<VtkEpcCommon*> tree)
{

		cout << "emit=================================================\n";
	emit refreshTreeView(tree);
}

