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

#ifndef OBJECTTREEMODEL_H
#define OBJECTTREEMODEL_H

#include <QAbstractItemModel>
#include <QVector>

#include <KFormat>
#include <string.h>

struct ObjectRowData
{
    ObjectRowData()
    :classIndex(0),
     className(),
     gcNum(0),
     allocations(0),
     allocated(0),
     referenced(0),
     parent(nullptr),
     children()
    {}
    uint64_t classIndex;
    std::string className;
    quint32 gcNum;
    uint64_t allocations;
    uint64_t allocated;
    uint64_t referenced;
    const ObjectRowData* parent;
    QVector<ObjectRowData> children;
    bool operator<(const ObjectRowData& rhs) const
    {
        return className < rhs.className;
    }
};

using ObjectTreeData = QVector<ObjectRowData>;
Q_DECLARE_METATYPE(ObjectTreeData)

class ObjectTreeModel : public QAbstractItemModel
{
    Q_OBJECT
public:
    ObjectTreeModel(QObject* parent);
    virtual ~ObjectTreeModel();

    enum Columns
    {
        InstanceCountColumn,
        ShallowSizeColumn,
        ReferencedSizeColumn,
        GCNumColumn,
        ClassNameColumn,
        NUM_COLUMNS
    };

    enum Roles
    {
        SortRole = Qt::UserRole,
        MaxCostRole,
        LocationRole
    };

    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QModelIndex index(int row, int column, const QModelIndex& parent = QModelIndex()) const override;
    QModelIndex parent(const QModelIndex& child) const override;
    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    int columnCount(const QModelIndex& parent = QModelIndex()) const override;

public slots:
    void resetData(const ObjectTreeData& data);
    void clearData();

private:
    /// @return the row resembled by @p index
    const ObjectRowData* toRow(const QModelIndex& index) const;
    /// @return the row number of @p row in its parent
    int rowOf(const ObjectRowData* row) const;

    ObjectTreeData m_data;
    KFormat m_format;
};

#endif // OBJECTTREEMODEL_H
