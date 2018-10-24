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

#ifndef _PQEtpPanel_h
#define _PQEtpPanel_h

#include <QDockWidget>
#include <QComboBox>
#include <QLineEdit>
#include <QTextBrowser>
#include <QPushButton>
#include <QStringList>

#include "etp/vtkEtpDocument.h"


class PQEtpPanel : public QDockWidget
{
	Q_OBJECT
	typedef QDockWidget Superclass;
public:
	PQEtpPanel(const QString &t, QWidget* p = 0, Qt::WindowFlags f=0):
		Superclass(t, p, f) { this->constructor(); }
	PQEtpPanel(QWidget *p=0, Qt::WindowFlags f=0):
		Superclass(p, f) { this->constructor(); }
	
	void etpClientConnect(const std::string & ipAddress, const std::string & port);
	void setConnectionStatus(bool connect) { etp_connect = connect; }
	void setVtkEtpDocuement(vtkEtpDocument* vtkEtp) { etp_document = vtkEtp; }

protected slots:
	void handleButtonSend();
	void handleButtonStatus();
  
private:
	void constructor();

	QComboBox *EtpCommand;
	QLineEdit *EtpCommandArgument;
	QTextBrowser *EtpTextBrowser;
	QPushButton *EtpSendButton;
	QPushButton *EtpStatus_Button;

	QStringList EtpCommand_list;

	bool debug_verif;

	vtkEtpDocument* etp_document;

	bool etp_connect;
};

#endif

