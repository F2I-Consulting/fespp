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
#include "PQDataLoadManager.h"

#include "PQToolsManager.h"

#include "pqApplicationCore.h"
#include "pqObjectBuilder.h"
#include "pqPipelineSource.h"
#include "vtkSMProperty.h"
#include "vtkSMSourceProxy.h"
#include "pqDataRepresentation.h"
#include "pqSMAdaptor.h"
#include "pqUndoStack.h"

#include <pqPropertiesPanel.h>
#include <pqActiveObjects.h>

#include <pqPipelineRepresentation.h>

#include "vtkSMPropertyHelper.h"
#include <vtkSMStringVectorProperty.h>

#include <QPushButton>

#include "ui_PQDataLoadManager.h"
class PQDataLoadManager::pqUI : public Ui::PQDataLoadManager
{
};

PQDataLoadManager::PQDataLoadManager(QWidget* p, Qt::WindowFlags f /*=0*/)
: QDialog(p, f)
{
	PQToolsManager* manager = PQToolsManager::instance();
	this->Server = manager->getActiveServer();

	this->ui = new PQDataLoadManager::pqUI;
	this->ui->setupUi(this);

	this->ui->epcFile->setServer(this->Server);
	this->ui->epcFile->setForceSingleFile(false);
	this->ui->epcFile->setExtension("epc Files (*.epc)");

	QObject::connect(this->ui->epcFile, SIGNAL(filenamesChanged(const QStringList&)), this,
			SLOT(checkInputValid()));

	QObject::connect(this, SIGNAL(accepted()), this, SLOT(setupPipeline()));

	this->checkInputValid();
}

PQDataLoadManager::~PQDataLoadManager()
{
	delete this->ui;
}

//-----------------------------------------------------------------------------
void PQDataLoadManager::checkInputValid()
{
	auto valid = (this->ui->epcFile->filenames().isEmpty()) ? false : true;
	this->ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(valid);
}

//-----------------------------------------------------------------------------
void PQDataLoadManager::setupPipeline()
{
	pqApplicationCore* core = pqApplicationCore::instance();
	pqObjectBuilder* builder = core->getObjectBuilder();

	PQToolsManager* manager = PQToolsManager::instance();

	BEGIN_UNDO_SET("EPC Data Load");

	pqPipelineSource* fesppReader;

	if (manager->existPipe()) {
		fesppReader = manager->getFesppReader();
	}
	else {
		fesppReader = builder->createReader("sources", "Fespp", QStringList("EpcDocument"), this->Server);
		manager->existPipe(true);
	}
	QStringList epcFiles =	this->ui->epcFile->filenames();

	if (!epcFiles.isEmpty() && fesppReader) 	{

		for (auto i=0; i < epcFiles.length(); ++i){
			pqActiveObjects *activeObjects = &pqActiveObjects::instance();
			activeObjects->setActiveSource(fesppReader);

			vtkSMProxy* fesppReaderProxy = fesppReader->getProxy();
			vtkSMPropertyHelper( fesppReaderProxy, "SubFileName" ).Set(epcFiles[i].toStdString().c_str());

			fesppReaderProxy->UpdateSelfAndAllInputs();

			manager->newFile(epcFiles[i].toStdString().c_str());
		}
	}
	END_UNDO_SET();
}
