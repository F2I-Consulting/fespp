#include "VtkAbstractObject.h"

//----------------------------------------------------------------------------
VtkAbstractObject::VtkAbstractObject( const std::string & fileName, const std::string & name, const std::string & uuid, const std::string & uuidParent, const int & idProc, const int & maxProc):
	fileName(fileName), name(name), uuid(uuid), uuidParent(uuidParent), idProc(idProc), maxProc(maxProc)
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
int VtkAbstractObject::getIdProc() const
{
	return idProc;
}

//----------------------------------------------------------------------------
int VtkAbstractObject::getMaxProc() const
{
	if(maxProc==0)
		return 1;
	else
		return maxProc;
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
