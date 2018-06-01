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

#ifndef TREEPROXY_H
#define TREEPROXY_H

#ifdef NO_K_LIB
#include <QSortFilterProxyModel>
#else
#include <KRecursiveFilterProxyModel>
#endif

class TreeProxy final : public
#ifdef NO_K_LIB
    QSortFilterProxyModel
#else
    KRecursiveFilterProxyModel
#endif
{
    Q_OBJECT
public:
    explicit TreeProxy(int functionColumn, int fileColumn, int moduleColumn, QObject* parent = nullptr);
    virtual ~TreeProxy();

public slots:
    void setFunctionFilter(const QString& functionFilter);
    void setFileFilter(const QString& fileFilter);
    void setModuleFilter(const QString& moduleFilter);

private:
#ifdef NO_K_LIB
    bool acceptRow(int source_row, const QModelIndex& source_parent) const;
    bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const override;
#else
    bool acceptRow(int source_row, const QModelIndex& source_parent) const override;
#endif

    int m_functionColumn;
    int m_fileColumn;
    int m_moduleColumn;

    QString m_functionFilter;
    QString m_fileFilter;
    QString m_moduleFilter;
};

#endif // TREEPROXY_H
