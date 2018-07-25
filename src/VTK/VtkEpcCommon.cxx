#include "VtkEpcCommon.h"


//----------------------------------------------------------------------------
VtkEpcCommon::VtkEpcCommon()
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
