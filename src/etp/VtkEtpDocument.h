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

#ifndef __vtkEtpDocument_h
#define __vtkEtpDocument_h

#include <string>
#include <vector>
#include <list>
// include VTK
#include <vtkSmartPointer.h>

#include "VTK/VtkResqml2MultiBlockDataSet.h"

#include <etp/ProtocolHandlers/DirectedDiscoveryHandlers.h>

#include "VTK/VtkEpcCommon.h"

// include system
#if (defined(_WIN32) && _MSC_VER >= 1600)
#include <unordered_map>
#else
#include <tr1/unordered_map>
#endif

class EtpClientSession;

class VtkIjkGridRepresentation;
class VtkUnstructuredGridRepresentation;
class VtkPartialRepresentation;
class VtkGrid2DRepresentation;
class VtkPolylineRepresentation;
class VtkTriangulatedRepresentation;
class VtkSetPatch;
class VtkWellboreTrajectoryRepresentation;

class VtkEtpDocument  : public VtkResqml2MultiBlockDataSet
{

public:

	/**
	 * Constructor
	 */
	VtkEtpDocument (const std::string & ipAddress, const std::string & port, const VtkEpcCommon::modeVtkEpc & mode);
	/**
	 * Destructor
	 */
	~VtkEtpDocument();

	/**
	 * method : remove
	 * variable : std::string uuid
	 * delete uuid representation.
	 */
	void remove(const std::string & uuid);

	void receive_resources_tree(const std::string & rec_uri,
			const std::string & rec_contentType,
			const std::string & rec_name,
			Energistics::Etp::v12::Datatypes::Object::ResourceKind & rec_resourceType,
			const int32_t & rec_sourceCount,
			const int32_t & rec_targetCount,
			const int32_t & rec_contentCount,
			const int64_t & rec_lastChanged);

	void setClientSession(EtpClientSession * session) {client_session = session;}

	void createTree();
	void attach();

	/**
	 * method : get TreeView
	 * variable :
	 */
	std::vector<VtkEpcCommon*> getTreeView() const;

	/**
	 * method : visualize
	 * variable : std::string uuid
	 * create uuid representation.
	 */
	void visualize(const std::string & uuid);

	/**
	 * method : remove
	 * variable : std::string uuid
	 * delete uuid representation.
	 */
	void unvisualize(const std::string & uuid);

	/**
	 * method : getOutput
	 * variable : --
	 * return the vtkMultiBlockDataSet for each epcdocument.
	 */
	vtkSmartPointer<vtkMultiBlockDataSet> getVisualization() const;

	long getAttachmentPropertyCount(const std::string & uuid, const VtkEpcCommon::FesppAttachmentProperty propertyUnit) ;

protected:

	VtkEpcCommon* firstLeafOfTreeView();
private:

	int64_t push_command(const std::string & command);

	void addPropertyTreeVtk(const std::string & uuid, const std::string & parent, const std::string & name);


	void createTreeVtk(const std::string & uuid, const std::string & parent, const std::string & name, const VtkEpcCommon::Resqml2Type & resqmlType);

	std::list<int> number_response_wait_queue;
	std::list<VtkEpcCommon*> response_queue;
	std::list<std::string> command_queue;

	EtpClientSession * client_session;

	bool treeViewMode;
	bool representationMode;

	std::vector<VtkEpcCommon*> treeView; // Tree

	int64_t last_id;

#if _MSC_VER < 1600
	std::tr1::unordered_map<std::string, VtkIjkGridRepresentation*> uuidToVtkIjkGridRepresentation;
#else
		std::unordered_map<std::string, VtkIjkGridRepresentation*> uuidToVtkIjkGridRepresentation;
#endif


};
#endif
