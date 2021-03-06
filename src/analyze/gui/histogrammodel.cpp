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

#include "histogrammodel.h"

#include "gui_config.h"

#ifdef KChart_FOUND
#include <KChartGlobal>
#endif

#ifdef NO_K_LIB
#include "noklib.h"
#else
#include <KFormat>
#include <KLocalizedString>
#endif

#include "util.h"

#include <QBrush>
#include <QColor>
#include <QPen>

#include <limits>

namespace {
QColor colorForColumn(int column, int columnCount)
{
    return QColor::fromHsv((double(column) / columnCount) * 255, 255, 255);
}
}

HistogramModel::HistogramModel(QObject* parent)
    : QAbstractTableModel(parent)
{
    qRegisterMetaType<HistogramData>("HistogramData");
}

HistogramModel::~HistogramModel() = default;

QVariant HistogramModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Vertical && role == Qt::DisplayRole && section >= 0 && section < m_data.size()) {
        return m_data.at(section).sizeLabel;
    }
    return {};
}

QVariant HistogramModel::data(const QModelIndex& index, int role) const
{
    if (!hasIndex(index.row(), index.column(), index.parent())) {
        return {};
    }
#ifdef KChart_FOUND
    if (role == KChart::DatasetBrushRole) {
        return QVariant::fromValue(QBrush(colorForColumn(index.column(), columnCount())));
    } else if (role == KChart::DatasetPenRole) {
        return QVariant::fromValue(QPen(Qt::black));
    }
#endif

    if (role != Qt::DisplayRole && role != Qt::ToolTipRole) {
        return {};
    }

    const auto& row = m_data.at(index.row());
    const auto& column = row.columns[index.column()];
    if (role == Qt::ToolTipRole) {
        if (index.column() == 0) {
            return i18n("<b>%1</b> allocations in total", column.allocations);
        }
        if (!column.location) {
            return {};
        }
        QString tooltip;
        if (!column.location->file.isEmpty()) {
            tooltip = i18n("%1 allocations from %2 at %3:%4 in %5", column.allocations, column.location->function,
                            column.location->file, column.location->line, column.location->module);
        }
        else
        {
            tooltip = i18n("%1 allocations from %2 in %3", column.allocations, column.location->function,
                           column.location->module);
        }
#ifdef QWT_FOUND
        tooltip = Util::wrapLabel(tooltip, 96, 0, "&nbsp;<br>");
#else
        tooltip = tooltip.toHtmlEscaped(); // Qt wraps text in tooltips itself
#endif
        // enclose first word in <b> and </b> tags
        int i = tooltip.indexOf(' ');
        if (i >= 0)
        {
            tooltip = "<b>" + tooltip.left(i) + "</b>" + tooltip.mid(i);
        }
        return tooltip;
    }
    return column.allocations;
}

int HistogramModel::columnCount(const QModelIndex& parent) const
{
    if (parent.isValid()) {
        return 0;
    }
    return HistogramRow::NUM_COLUMNS;
}

int HistogramModel::rowCount(const QModelIndex& parent) const
{
    if (parent.isValid()) {
        return 0;
    }
    return m_data.size();
}

void HistogramModel::resetData(const HistogramData& data)
{
    beginResetModel();
    m_data = data;
    endResetModel();
}

void HistogramModel::clearData()
{
    beginResetModel();
    m_data = {};
    endResetModel();
}

QColor HistogramModel::getColumnColor(int column) const
{
    return colorForColumn(column, columnCount());
}

LocationData::Ptr HistogramModel::getLocationData(int row, int column) const
{
    const auto& rowData = m_data.at(row);
    const auto& columnData = rowData.columns[column];
    return columnData.location;
}
