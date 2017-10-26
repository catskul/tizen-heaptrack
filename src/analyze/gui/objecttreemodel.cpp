/*
 * Copyright 2015-2017 Milian Wolff <mail@milianw.de>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "objecttreemodel.h"

#include <QDebug>
#include <QTextStream>

#include <KFormat>
#include <KLocalizedString>

#include <cmath>

namespace {

int indexOf(const ObjectRowData* row, const ObjectTreeData& siblings)
{
    Q_ASSERT(siblings.data() <= row);
    Q_ASSERT(siblings.data() + siblings.size() > row);
    return row - siblings.data();
}

const ObjectRowData* rowAt(const ObjectTreeData& rows, int row)
{
    Q_ASSERT(rows.size() > row);
    return rows.data() + row;
}

/// @return the parent row containing @p index
const ObjectRowData* toParentRow(const QModelIndex& index)
{
    return static_cast<const ObjectRowData*>(index.internalPointer());
}
}

ObjectTreeModel::ObjectTreeModel(QObject* parent)
    : QAbstractItemModel(parent)
{
    qRegisterMetaType<ObjectTreeData>();
}

ObjectTreeModel::~ObjectTreeModel()
{
}

QVariant ObjectTreeModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation != Qt::Horizontal || section < 0 || section >= NUM_COLUMNS) {
        return {};
    }
    if (role == Qt::InitialSortOrderRole) {
        if (section == InstanceCountColumn) {
            return Qt::DescendingOrder;
        }
    }
    if (role == Qt::DisplayRole) {
        switch (static_cast<Columns>(section)) {
        case ClassNameColumn:
            return i18n("Class Name");
        case InstanceCountColumn:
            return i18n("Instances");
        case ShallowSizeColumn:
            return i18n("Shallow Size");
        case ReferencedSizeColumn:
            return i18n("Referenced Size");
        case GCNumColumn:
            return i18n("GC #");
        case NUM_COLUMNS:
            break;
        }
    } else if (role == Qt::ToolTipRole) {
        switch (static_cast<Columns>(section)) {
        case ClassNameColumn:
            return i18n("<qt>The name of the class.</qt>");
        case InstanceCountColumn:
            return i18n("<qt>Total number of instances in this reference chain.</qt>");
        case GCNumColumn:
            return i18n("<qt>GC number after which the snapshot has been taken.</qt>");
        case ShallowSizeColumn:
            return i18n("<qt>Total size of objects of this type in a given reference chain.</qt>");
        case ReferencedSizeColumn:
            return i18n("<qt>Total size of objects of this type referenced by other objects in a given reference chain.</qt>");
        case NUM_COLUMNS:
            break;
        }
    }
    return {};
}

QVariant ObjectTreeModel::data(const QModelIndex& index, int role) const
{
    if (index.row() < 0 || index.column() < 0 || index.column() > NUM_COLUMNS) {
        return {};
    }

    const auto row = toRow(index);

    if (role == Qt::DisplayRole || role == SortRole) {
        switch (static_cast<Columns>(index.column())) {
        case ClassNameColumn:
            if (row->className.empty())
                return i18n("<gcroot>");
            return i18n(row->className.c_str());
        case InstanceCountColumn:
            return static_cast<qint64>(row->allocations);
        case ShallowSizeColumn:
            if (role == SortRole) {
                return static_cast<qint64>(row->allocated);
            }
            return m_format.formatByteSize(row->allocated, 1, KFormat::JEDECBinaryDialect);
        case ReferencedSizeColumn:
            if (role == SortRole) {
                return static_cast<qint64>(row->referenced);
            }
            return m_format.formatByteSize(row->referenced, 1, KFormat::JEDECBinaryDialect);
        case GCNumColumn:
            return static_cast<quint64>(row->gcNum);
        case NUM_COLUMNS:
            break;
        }
    } else if (role == Qt::ToolTipRole) {
        QString tooltip;
        QTextStream stream(&tooltip);
        stream << "<qt><pre style='font-family:monospace;'>";
        switch (static_cast<Columns>(index.column())) {
            case ClassNameColumn:
                stream << i18n("The name of the class.");
                break;
            case InstanceCountColumn:
                stream << i18n("Total number of instances in this reference chain.");
                break;
            case GCNumColumn:
                stream << i18n("GC number after which the snapshot has been taken.");
                break;
            case ShallowSizeColumn:
                stream << i18n("Total size of objects of this type in a given reference chain.");
                break;
            case ReferencedSizeColumn:
                stream << i18n("Total size of objects of this type referenced by other objects in a given reference chain.");
                break;
            case NUM_COLUMNS:
                break;
            }
        stream << "</pre></qt>";
        return tooltip;
    }
    return {};
}

QModelIndex ObjectTreeModel::index(int row, int column, const QModelIndex& parent) const
{
    if (row < 0 || column < 0 || column >= NUM_COLUMNS || row >= rowCount(parent)) {
        return QModelIndex();
    }
    return createIndex(row, column, const_cast<void*>(reinterpret_cast<const void*>(toRow(parent))));
}

QModelIndex ObjectTreeModel::parent(const QModelIndex& child) const
{
    if (!child.isValid()) {
        return {};
    }
    const auto parent = toParentRow(child);
    if (!parent) {
        return {};
    }
    return createIndex(rowOf(parent), 0, const_cast<void*>(reinterpret_cast<const void*>(parent->parent)));
}

int ObjectTreeModel::rowCount(const QModelIndex& parent) const
{
    if (!parent.isValid()) {
        return m_data.size();
    } else if (parent.column() != 0) {
        return 0;
    }
    auto row = toRow(parent);
    Q_ASSERT(row);
    return row->children.size();
}

int ObjectTreeModel::columnCount(const QModelIndex& /*parent*/) const
{
    return NUM_COLUMNS;
}

void ObjectTreeModel::resetData(const ObjectTreeData& data)
{
    beginResetModel();
    m_data = data;
    endResetModel();
}

void ObjectTreeModel::clearData()
{
    beginResetModel();
    m_data = {};
    endResetModel();
}

const ObjectRowData* ObjectTreeModel::toRow(const QModelIndex& index) const
{
    if (!index.isValid()) {
        return nullptr;
    }
    if (const auto parent = toParentRow(index)) {
        return rowAt(parent->children, index.row());
    } else {
        return rowAt(m_data, index.row());
    }
}

int ObjectTreeModel::rowOf(const ObjectRowData* row) const
{
    if (auto parent = row->parent) {
        return indexOf(row, parent->children);
    } else {
        return indexOf(row, m_data);
    }
}
