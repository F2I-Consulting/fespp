#include "VtkAbstractObject.h"

//----------------------------------------------------------------------------
VtkAbstractObject::VtkAbstractObject( const std::string & fileName, const std::string & name, const std::string & uuid, const std::string & uuidParent):
	fileName(fileName), name(name), uuid(uuid), uuidParent(uuidParent)
{
}

//----------------------------------------------------------------------------
std::string VtkAbstractObject::getFileName() const
{
	return fileName;
}

//----------------------------------------------------------------------------
std::string VtkAbstractObject::getName() const
{
	return name;
}

//----------------------------------------------------------------------------
std::string VtkAbstractObject::getUuid() const
{
	return uuid;
}

//----------------------------------------------------------------------------
std::string VtkAbstractObject::getParent() const
{
	return uuidParent;
}

//----------------------------------------------------------------------------
void VtkAbstractObject::setFileName(const std::string & newFileName) 
{
	fileName = newFileName;
}

//----------------------------------------------------------------------------
void VtkAbstractObject::setName(const std::string & newName) 
{
	name = newName;
}

//----------------------------------------------------------------------------
void VtkAbstractObject::setUuid(const std::string & newUuid) 
{
	uuid = newUuid;
}

//----------------------------------------------------------------------------
void VtkAbstractObject::setParent(const std::string & newUuidParent)
{
	uuidParent = newUuidParent;
}
