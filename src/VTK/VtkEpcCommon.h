/*-----------------------------------------------------------------------
Licensed to the Apache Software Foundation (ASF) under one
or more contributor license agreements.  See the NOTICE file
distributed with this work for additional information
regarding copyright ownership.  The ASF licenses this file
to you under the Apache License, Version 2.0 (the
"License"; you may not use this file except in compliance
with the License.  You may obtain a copy of the License at

  http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing,
software distributed under the License is distributed on an
"AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
KIND, either express or implied.  See the License for the
specific language governing permissions and limitations
under the License.
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

	enum FesppAttachmentProperty { POINTS = 0, CELLS = 1 };
	enum modeVtkEpc {TreeView=0, Representation=1, Both=2};
	enum Resqml2Type { UNKNOW = -1, EPC_DOC = 0, ETP_DOC = 1, /*FEATURE = 1,*/ INTERPRETATION_1D = 2, INTERPRETATION_2D = 3, INTERPRETATION_3D = 4, POLYLINE_SET = 5, TRIANGULATED_SET = 6, POLYLINE = 7, TRIANGULATED = 8, IJK_GRID = 9, GRID_2D = 16, PROPERTY = 15, UNSTRUC_GRID = 10, WELL_TRAJ = 11, PARTIAL = 12, SUB_REP = 13, TIME_SERIES = 14 };

	std::string	getUuid() const;
	void setUuid(const std::string &);

	std::string	getParent() const;
	void setParent(const std::string &);

	std::string	getName() const;
	void setName(const std::string &);

	Resqml2Type	getType() const;
	void setType(Resqml2Type);

	Resqml2Type	getParentType() const;
	void setParentType(Resqml2Type);

	int	getTimeIndex() const;
	void setTimeIndex(int);

	time_t	getTimestamp() const;
	void setTimestamp(time_t);

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
