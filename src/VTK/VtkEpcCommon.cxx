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
#include "VtkEpcCommon.h"


//----------------------------------------------------------------------------
VtkEpcCommon::VtkEpcCommon() :
uuid(""),parent(""),name(""),myType(VtkEpcCommon::UNKNOW),parentType(VtkEpcCommon::UNKNOW),timeIndex(-1),timestamp(0)
{
}

//----------------------------------------------------------------------------
VtkEpcCommon::~VtkEpcCommon()
{

}

//----------------------------------------------------------------------------
std::string VtkEpcCommon::getUuid() const
{
	return uuid;
}

//----------------------------------------------------------------------------
std::string VtkEpcCommon::getParent() const
{
	return parent;
}

//----------------------------------------------------------------------------
std::string VtkEpcCommon::getName() const
{
	return name;
}

//----------------------------------------------------------------------------
VtkEpcCommon::Resqml2Type VtkEpcCommon::getType() const
{
	return myType;
}

//----------------------------------------------------------------------------
VtkEpcCommon::Resqml2Type VtkEpcCommon::getParentType() const
{
	return parentType;
}

//----------------------------------------------------------------------------
int VtkEpcCommon::getTimeIndex() const
{
	return timeIndex;
}

//----------------------------------------------------------------------------
time_t VtkEpcCommon::getTimestamp() const
{
	return timestamp;
}

//----------------------------------------------------------------------------
void VtkEpcCommon::setUuid(const std::string & value)
{
	uuid = value;
}

//----------------------------------------------------------------------------
void VtkEpcCommon::setParent(const std::string & value)
{
	parent = value;
}

//----------------------------------------------------------------------------
void VtkEpcCommon::setName(const std::string & value)
{
	name = value;
}

//----------------------------------------------------------------------------
void VtkEpcCommon::setType(const VtkEpcCommon::Resqml2Type & value)
{
	myType = value;
}

//----------------------------------------------------------------------------
void VtkEpcCommon::setParentType(const VtkEpcCommon::Resqml2Type & value)
{
	parentType = value;
}

//----------------------------------------------------------------------------
void VtkEpcCommon::setTimeIndex(const int & value )
{
	timeIndex = value;
}

//----------------------------------------------------------------------------
void VtkEpcCommon::setTimestamp(const time_t & value)
{
	timestamp = value;
}
