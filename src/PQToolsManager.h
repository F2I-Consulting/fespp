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

#ifndef _PQToolsManager_H
#define _PQToolsManager_H

#include <QObject>

class QAction;

class pqPipelineSource;
class pqServer;
class pqView;
class VtkEpcCommon;

class PQToolsManager : public QObject
{
	Q_OBJECT;

public:
	static PQToolsManager* instance();

	~PQToolsManager();

	QAction* actionDataLoadManager();
#ifdef WITH_ETP
	QAction* actionEtpCommand();
#endif

	pqPipelineSource* getFesppReader();

	QWidget* getMainWindow();
	pqServer* getActiveServer();

	bool existPipe();
	void existPipe(bool value);

#ifdef WITH_ETP
	bool etp_existPipe();
	void etp_existPipe(bool value);
#endif

	void newFile(const std::string & fileName);

public slots:
	void showDataLoadManager();
	void showPanelMetadata();
#ifdef WITH_ETP
	void showEtpConnectionManager();
#endif

protected:
	virtual pqPipelineSource* findPipelineSource(const char* SMName);
	void setVisibilityPanelSelection(bool visible);

protected slots:
	/**
	 * When a pipeline source is deleted
	 */
	void deletePipelineSource(pqPipelineSource*);
	void newPipelineSource(pqPipelineSource*, const QStringList &);

private:
	PQToolsManager(QObject* p);

	class pqInternal;
	pqInternal* Internal;

	bool existEpcPipe;
#ifdef WITH_ETP
	bool existEtpPipe;
#endif
	bool panelSelectionVisible;
	bool panelMetadataVisible;
	Q_DISABLE_COPY(PQToolsManager);
};

#endif

