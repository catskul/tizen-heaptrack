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

#include "objecttreeproxy.h"

ObjectTreeProxy::ObjectTreeProxy(int nameColumn, int gcColumn, QObject* parent)
    : KRecursiveFilterProxyModel(parent)
    , m_nameColumn(nameColumn)
    , m_gcColumn(gcColumn)
{
}

ObjectTreeProxy::~ObjectTreeProxy() = default;

void ObjectTreeProxy::setNameFilter(const QString& nameFilter)
{
    m_nameFilter = nameFilter;
    invalidate();
}

void ObjectTreeProxy::setGCFilter(int gcFilter)
{
    m_gcFilter = gcFilter;
    invalidate();
}

bool ObjectTreeProxy::filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const
{
    // TODO: Implement some caching so that if one match is found on the first pass, we can return early results
    // when the subtrees are checked by QSFPM.
    if (acceptRow(sourceRow, sourceParent)) {
        return true;
    }

    QModelIndex source_index = sourceModel()->index(sourceRow, 0, sourceParent);
    Q_ASSERT(source_index.isValid());
    bool accepted = false;

    QModelIndex parent = sourceParent;
    while (parent.isValid()) {
        if (acceptRow(parent.row(), parent.parent()))
            return true;
        parent = parent.parent();
    }

    const int numChildren = sourceModel()->rowCount(source_index);
    for (int row = 0, rows = numChildren; row < rows; ++row) {
        if (filterAcceptsRow(row, source_index)) {
            accepted = true;
            break;
        }
    }

    return accepted;
}

bool ObjectTreeProxy::acceptRow(int sourceRow, const QModelIndex& sourceParent) const
{
    auto source = sourceModel();
    if (!source) {
        return false;
    }

    const auto& name = source->index(sourceRow, m_nameColumn, sourceParent).data().toString();
    if (name.contains(QString::fromStdString("EMPTY_MESSAGE"), Qt::CaseInsensitive))
        return false;

    if (!m_nameFilter.isEmpty()) {
        if (!name.contains(m_nameFilter, Qt::CaseInsensitive)) {
            return false;
        }
    }
    if (m_gcFilter != -1) {
        const int gc = source->index(sourceRow, m_gcColumn, sourceParent).data().toInt() - 1;
        if (gc != m_gcFilter) {
            return false;
        }
    }
    return true;
}
