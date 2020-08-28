/* ----------------------------------------------------------------------
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

	enum class FesppAttachmentProperty { POINTS = 0, CELLS = 1 };
	enum class modeVtkEpc {TreeView=0, Representation=1, Both=2};
	enum class Resqml2Type {
		UNKNOW = -1,
		EPC_DOC = 0,
		ETP_DOC = 1,
		INTERPRETATION_1D = 2,
		WELL_TRAJ = 3,
		WELL_MARKER = 4,
		WELL_MARKER_FRAME = 5,
		WELL_FRAME = 6,
		INTERPRETATION_2D = 7,
		POLYLINE_SET = 8,
		TRIANGULATED_SET = 9,
		POLYLINE = 10,
		TRIANGULATED = 11,
		GRID_2D = 12,
		INTERPRETATION_3D = 13,
		IJK_GRID = 14,
		UNSTRUC_GRID = 15,
		PROPERTY = 16,
		TIME_SERIES = 17,
		SUB_REP = 18,
		PARTIAL = 19
	};

	VtkEpcCommon() : uuid(""), parent(""), name(""), myType(Resqml2Type::UNKNOW), parentType(Resqml2Type::UNKNOW), timeIndex(-1), timestamp(0) {}
	VtkEpcCommon (const std::string & uuid, const std::string & parent, const std::string & name, const Resqml2Type & type) :
		uuid(uuid),parent(parent),name(name),myType(type),parentType(Resqml2Type::UNKNOW),timeIndex(-1),timestamp(0) {}
	~VtkEpcCommon() {}


	const std::string& getUuid() const { return uuid; }
	void setUuid(const std::string & value) { uuid = value; }

	const std::string& getParent() const { return parent; }
	void setParent(const std::string & value) { parent = value; }

	const std::string& getName() const { return name; }
	void setName(const std::string & value) { name = value; }

	Resqml2Type	getType() const { return myType; }
	void setType(Resqml2Type value) { myType = value; }

	Resqml2Type	getParentType() const { return parentType; }
	void setParentType(Resqml2Type value) { parentType = value; }

	int	getTimeIndex() const { return timeIndex; }
	void setTimeIndex(int value) { timeIndex = value; }

	time_t	getTimestamp() const { return timestamp; }
	void setTimestamp(time_t value) { timestamp = value; }

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
