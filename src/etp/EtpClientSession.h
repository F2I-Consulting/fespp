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

#ifndef _etpClientSession_h_
#define _etpClientSession_h_

// include FESAPI
#include <etp/ClientSession.h>
#include <common/EpcDocument.h>

//include Fespp
#include "VTK/VtkEpcCommon.h"

class VtkEtpDocument;
class EtpClientSession : public ETP_NS::ClientSession
{
public:
	/**
	 * @param host		The IP address on which the server is listening for etp (websocket) connection
	 * @param port		The port on which the server is listening for etp (websocket) connection
	 * @param requestedProtocols An array of protocol IDs that the client expects to communicate on for this session. If the server does not support all of the protocols, the client may or may not continue with the protocols that are supported.
	 * @param supportedObjects		A list of the Data Objects supported by the client. This list MUST be empty if the client is a customer. This field MUST be supplied if the client is a Store and is requesting a customer role for the server.
	 */
	EtpClientSession(boost::asio::io_context& ioc,
			const std::string & host, const std::string & port,
			const std::vector<Energistics::Etp::v12::Datatypes::SupportedProtocol> & requestedProtocols,
			const std::vector<std::string>& supportedObjects,
			VtkEtpDocument* my_etp_document,
			const VtkEpcCommon::modeVtkEpc & mode);

	~EtpClientSession() {};

	COMMON_NS::EpcDocument epcDoc;
};
#endif
