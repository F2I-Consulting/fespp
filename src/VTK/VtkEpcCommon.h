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

#ifndef __VtkEpcCommon_h
#define __VtkEpcCommon_h

// include system
#include <string>



class VtkEpcCommon
{
public:

	VtkEpcCommon ();
	~VtkEpcCommon();

	enum Resqml2Object {NONE=0, FAULT=1, HORIZON=2};
	enum FesppAttachmentProperty { POINTS = 0, CELLS = 1 };
	enum modeVtkEpc {TreeView=0, Representation=1, Both=2};
	enum Resqml2Type { UNKNOW = -1, EPC_DOC = 0, FEATURE = 1, INTERPRETATION = 2, POLYLINE_SET = 3, TRIANGULATED_SET = 4, POLYLINE = 5, TRIANGULATED = 6, IJK_GRID = 7, GRID_2D = 8, PROPERTY = 9, UNSTRUC_GRID = 10, WELL_TRAJ = 11, PARTIAL = 12, SUB_REP = 13, TIME_SERIES = 14 };



	std::string	getUuid() const;
	void setUuid(const std::string &);

	std::string	getParent() const;
	void setParent(const std::string &);

	std::string	getName() const;
	void setName(const std::string &);

	Resqml2Type	getType() const;
	void setType(const Resqml2Type &);

	Resqml2Type	getParentType() const;
	void setParentType(const Resqml2Type &);

	int	getTimeIndex() const;
	void setTimeIndex(const int &);

	time_t	getTimestamp() const;
	void setTimestamp(const time_t &);

private:
	std::string	uuid;
	std::string	parent;
	std::string	name;
	Resqml2Type myType;
	Resqml2Type parentType;
	int timeIndex;
	time_t timestamp;
};
#endif
