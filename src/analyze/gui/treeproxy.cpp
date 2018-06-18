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

#include "treeproxy.h"

TreeProxy::TreeProxy(int functionColumn, int fileColumn, int moduleColumn, QObject* parent)
#ifdef NO_K_LIB
    : QSortFilterProxyModel(parent)
#else
    : KRecursiveFilterProxyModel(parent)
#endif
    , m_functionColumn(functionColumn)
    , m_fileColumn(fileColumn)
    , m_moduleColumn(moduleColumn)
{
#if QT_VERSION >= 0x050A00
    setRecursiveFilteringEnabled(true);
#else
    #pragma message("Qt 5.10+ is required, otherwise text filtering in GUI will not work as expected")
#endif
}

TreeProxy::~TreeProxy() = default;

void TreeProxy::setFunctionFilter(const QString& functionFilter)
{
    m_functionFilter = functionFilter;
    invalidate();
}

void TreeProxy::setFileFilter(const QString& fileFilter)
{
    m_fileFilter = fileFilter;
    invalidate();
}

void TreeProxy::setModuleFilter(const QString& moduleFilter)
{
    m_moduleFilter = moduleFilter;
    invalidate();
}

bool TreeProxy::acceptRow(int sourceRow, const QModelIndex& sourceParent) const
{
    auto source = sourceModel();
    if (!source) {
        return false;
    }
    if (!m_functionFilter.isEmpty()) {
        const auto& function = source->index(sourceRow, m_functionColumn, sourceParent).data().toString();
        if (!function.contains(m_functionFilter, Qt::CaseInsensitive)) {
            return false;
        }
    }
    if (!m_fileFilter.isEmpty()) {
        const auto& file = source->index(sourceRow, m_fileColumn, sourceParent).data().toString();
        if (!file.contains(m_fileFilter, Qt::CaseInsensitive)) {
            return false;
        }
    }
    if (!m_moduleFilter.isEmpty()) {
        const auto& module = source->index(sourceRow, m_moduleColumn, sourceParent).data().toString();
        if (!module.contains(m_moduleFilter, Qt::CaseInsensitive)) {
            return false;
        }
    }
    return true;
}

#ifdef NO_K_LIB
bool TreeProxy::filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const
{
    return acceptRow(sourceRow, sourceParent);
}
#endif
