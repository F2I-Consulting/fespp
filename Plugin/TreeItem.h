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

#ifndef TREEITEM_H
#define TREEITEM_H

#include <QTreeWidget>

#include "VTK/VtkEpcCommon.h"

class TreeItem : public QTreeWidgetItem
{
public:
	explicit TreeItem(VtkEpcCommon const * vtkEpcCommon, TreeItem* parent) :
		QTreeWidgetItem(UserType), dataObjectInfo(vtkEpcCommon) {
		setText(0, vtkEpcCommon->getName().c_str());
		setFlags(flags() | Qt::ItemIsSelectable);

		QIcon icon;
		switch (vtkEpcCommon->getType()) {
		case VtkEpcCommon::Resqml2Type::PROPERTY: {
			if (parent->checkState(0) != Qt::Checked) {
				parent->setData(0, Qt::CheckStateRole, QVariant());
			}
			setCheckState(0, Qt::Unchecked);
			break;
		}
		case VtkEpcCommon::Resqml2Type::TIME_SERIES: {
			if (parent->checkState(0) != Qt::Checked) {
				parent->setData(0, Qt::CheckStateRole, QVariant());
			}
			setCheckState(0, Qt::Unchecked);
			break;
		}
		case VtkEpcCommon::Resqml2Type::INTERPRETATION_1D: {
			icon.addFile(QString::fromUtf8(":Interpretation_1D.png"));
			break;
		}
		case VtkEpcCommon::Resqml2Type::INTERPRETATION_2D: {
			icon.addFile(QString::fromUtf8(":Interpretation_2D.png"));
			break;
		}
		case VtkEpcCommon::Resqml2Type::INTERPRETATION_3D: {
			icon.addFile(QString::fromUtf8(":Interpretation_3D.png"));
			break;
		}
		case VtkEpcCommon::Resqml2Type::GRID_2D: {
			icon.addFile(QString::fromUtf8(":Grid2D.png"));
			setCheckState(0, Qt::Unchecked);
			break;
		}
		case VtkEpcCommon::Resqml2Type::POLYLINE_SET: {
			icon.addFile(QString::fromUtf8(":Polyline.png"));
			setCheckState(0, Qt::Unchecked);
			break;
		}
		case VtkEpcCommon::Resqml2Type::TRIANGULATED_SET: {
			icon.addFile(QString::fromUtf8(":Triangulated.png"));
			setCheckState(0, Qt::Unchecked);
			break;
		}
		case VtkEpcCommon::Resqml2Type::WELL_TRAJ: {
			icon.addFile(QString::fromUtf8(":WellTraj.png"));
			setCheckState(0, Qt::Unchecked);
			break;
		}
		case VtkEpcCommon::Resqml2Type::WELL_MARKER_FRAME: {
			icon.addFile(QString::fromUtf8(":WellBoreFrameMarker.png"));
			setCheckState(0, Qt::Unchecked);
			break;
		}
		case VtkEpcCommon::Resqml2Type::WELL_MARKER: {
			icon.addFile(QString::fromUtf8(":WellBoreMarker.png"));
			if (parent->checkState(0) != Qt::Checked) {
				parent->setData(0, Qt::CheckStateRole, QVariant());
			}
			setCheckState(0, Qt::Unchecked);
			break;
		}
		case VtkEpcCommon::Resqml2Type::WELL_FRAME: {
			icon.addFile(QString::fromUtf8(":WellBoreFrame.png"));
			setCheckState(0, Qt::Unchecked);
			break;
		}
		case VtkEpcCommon::Resqml2Type::IJK_GRID: {
			icon.addFile(QString::fromUtf8(":IjkGrid.png"));
			setCheckState(0, Qt::Unchecked);
			break;
		}
		case VtkEpcCommon::Resqml2Type::UNSTRUC_GRID: {
			icon.addFile(QString::fromUtf8(":UnstructuredGrid.png"));
			setCheckState(0, Qt::Unchecked);
			break;
		}
		case VtkEpcCommon::Resqml2Type::SUB_REP: {
			icon.addFile(QString::fromUtf8(":SubRepresentation.png"));
			setCheckState(0, Qt::Unchecked);
			break;
		}
		default:
			break;
		}

		setIcon(0, icon);
		parent->addChild(this);
	}
	explicit TreeItem(QTreeWidget *view) :
		QTreeWidgetItem(view, UserType), dataObjectInfo(nullptr) {}
	~TreeItem() {}

	VtkEpcCommon const * getDataObjectInfo() const { return dataObjectInfo; }

	std::string getUuid() const { return dataObjectInfo == nullptr ? text(0).toStdString() : dataObjectInfo->getUuid(); }

private:
	VtkEpcCommon const * dataObjectInfo;

	/**
	* operator to allow tree item sorting
	*/
	bool operator<(const QTreeWidgetItem &other) const {
		int column = treeWidget()->sortColumn();

		auto castOther = static_cast<TreeItem const *>(&other);
		VtkEpcCommon const * metadata = getDataObjectInfo();
		VtkEpcCommon const * otherMetadata = castOther->getDataObjectInfo();
		if (metadata != nullptr && otherMetadata != nullptr &&
			metadata->getType() != otherMetadata->getType()) {
			return metadata->getType() < otherMetadata->getType();
		}

		return text(column).toLower() < other.text(column).toLower();
	}
};

#endif // TREEITEM_H
