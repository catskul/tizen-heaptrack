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

#ifndef OBJECTTREEPROXY_H
#define OBJECTTREEPROXY_H

#ifdef NO_K_LIB
#include <QSortFilterProxyModel>
#else
#include <KRecursiveFilterProxyModel>
#endif

class ObjectTreeProxy final : public
#ifdef NO_K_LIB
    QSortFilterProxyModel
#else
    KRecursiveFilterProxyModel
#endif
{
    Q_OBJECT
public:
    explicit ObjectTreeProxy(int nameColumn, int gcColumn, QObject* parent = nullptr);
    virtual ~ObjectTreeProxy();

public slots:
    void setNameFilter(const QString& nameFilter);
    void setGCFilter(int gcFilter);

private:
#ifdef NO_K_LIB
    bool acceptRow(int source_row, const QModelIndex& source_parent) const;
#else
    bool acceptRow(int source_row, const QModelIndex& source_parent) const override;
#endif
    bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const override;

    int m_nameColumn;
    int m_gcColumn;

    QString m_nameFilter;
    int m_gcFilter;
};

#endif // OBJECTTREEPROXY_H
