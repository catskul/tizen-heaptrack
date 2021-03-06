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

#include "chartmodel.h"

#include "gui_config.h"

#ifdef KChart_FOUND
#include <KChartGlobal>
#include <KChartLineAttributes>
#endif
#ifdef NO_K_LIB
#include "noklib.h"
#else
#include <KFormat>
#include <KLocalizedString>
#endif

#include "util.h"

#include <QBrush>
#include <QDebug>
#include <QPen>

#include <algorithm>

namespace {
QColor colorForColumn(int column, int columnCount)
{
    return QColor::fromHsv((double(column + 1) / columnCount) * 255, 255, 255);
}
}

ChartModel::ChartModel(Type type, QObject* parent)
    : QAbstractTableModel(parent)
    , m_type(type)
{
    qRegisterMetaType<ChartData>();
}

ChartModel::~ChartModel() = default;

ChartModel::Type ChartModel::type() const
{
    return m_type;
}

QVariant ChartModel::headerData(int section, Qt::Orientation orientation, int role) const
{
//!!    Q_ASSERT(orientation == Qt::Horizontal || section < columnCount());
    if (!((orientation == Qt::Horizontal || section < columnCount())))
    {
        return {};
    }

    if (orientation == Qt::Horizontal) {
#ifdef KChart_FOUND
        if (role == KChart::DatasetPenRole) {
            return QVariant::fromValue(m_columnDataSetPens.at(section));
        } else if (role == KChart::DatasetBrushRole) {
            return QVariant::fromValue(m_columnDataSetBrushes.at(section));
        }
#endif
        if (role == Qt::DisplayRole || role == Qt::ToolTipRole) {
            if (section == 0) {
                return i18n("Elapsed Time");
            }
            switch (m_type) {
            case Allocated:
                return i18n("Memory Allocated");
            case Allocations:
                return i18n("Memory Allocations");
            case Instances:
                return i18n("Number of instances");
            case Consumed:
                return i18n("Memory Consumed");
            case Temporary:
                return i18n("Temporary Allocations");
            }
        }
    }
    return {};
}

QVariant ChartModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid()) {
        return {};
    }
    Q_ASSERT(index.row() >= 0 && index.row() < rowCount(index.parent()));
    Q_ASSERT(index.column() >= 0 && index.column() < columnCount(index.parent()));
    Q_ASSERT(!index.parent().isValid());

#ifdef KChart_FOUND
    if (role == KChart::LineAttributesRole) {
        KChart::LineAttributes attributes;
        attributes.setDisplayArea(true);
        if (index.column() > 1) {
            attributes.setTransparency(127);
        } else {
            attributes.setTransparency(50);
        }
        return QVariant::fromValue(attributes);
    }

    if (role == KChart::DatasetPenRole) {
        return QVariant::fromValue(m_columnDataSetPens.at(index.column()));
    } else if (role == KChart::DatasetBrushRole) {
        return QVariant::fromValue(m_columnDataSetBrushes.at(index.column()));
    }
#endif

    if (role != Qt::DisplayRole && role != Qt::ToolTipRole) {
        return {};
    }

    const auto& data = m_data.rows.at(index.row());

    int column = index.column();
    if (role != Qt::ToolTipRole && column % 2 == 0) {
        return data.timeStamp;
    }
    column = column / 2;
    Q_ASSERT(column < ChartRows::MAX_NUM_COST);

    const auto cost = data.cost[column];
    if (role == Qt::ToolTipRole) {
        const QString time = Util::formatTime(data.timeStamp);
        auto byteCost = [cost]() -> QString
        {
            const auto formatted = Util::formatByteSize(cost, 1);
            if (cost > 1024) {
                return i18nc("%1: the formatted byte size, e.g. \"1.2KB\", %2: the raw byte size, e.g. \"1300\"",
                             "<b>%1</b> (%2 bytes)", formatted, cost);
            } else {
                return QString("<b>%1</b>").arg(formatted);
            }
        };
        if (column == 0) {
            switch (m_type) {
            case Allocations:
                return i18n("<qt><b>%1</b> allocations in total after <b>%2</b></qt>", cost, time);
            case Temporary:
                return i18n("<qt><b>%1</b> temporary allocations in total after <b>%2</b></qt>", cost, time);
            case Instances:
                return i18n("<qt><b>%1</b> number of instances in total after <b>%2</b></qt>", cost, time);
            case Consumed:
                return i18n("<qt>%1 consumed in total after <b>%2</b></qt>",
                            byteCost(), time);
            case Allocated:
                return i18n("<qt>%1 allocated in total after <b>%2</b></qt>",
                            byteCost(), time);
            }
        } else {
            auto label = m_data.labels.value(column);
#ifdef QWT_FOUND
            label = Util::wrapLabel(label, 96, 0, "&nbsp;<br>");
#else
            label = label.toHtmlEscaped(); // Qt wraps text in tooltips itself
#endif
            switch (m_type) {
            case Allocations:
                return i18n("<qt><b>%2</b> allocations after <b>%3</b> from: "
                            "<p style='margin-left:10px;'>%1</p></qt>",
                            label, cost, time);
            case Temporary:
                return i18n("<qt><b>%2</b> temporary allocations after <b>%3</b> from: "
                            "<p style='margin-left:10px'>%1</p></qt>",
                            label, cost, time);
            case Instances:
                return i18n("<qt><b>%2</b> number of instances after <b>%3</b> from: "
                            "<p style='margin-left:10px'>%1</p></qt>",
                            label, cost, time);
            case Consumed:
                return i18n("<qt>%2 consumed after <b>%3</b> from: "
                            "<p style='margin-left:10px'>%1</p></qt>",
                            label, byteCost(), time);
            case Allocated:
                return i18n("<qt>%2 allocated after <b>%3</b> from: "
                            "<p style='margin-left:10px'>%1</p></qt>",
                            label, byteCost(), time);
            }
        }
        return {};
    }

    return cost;
}

int ChartModel::columnCount(const QModelIndex& /*parent*/) const
{
    return m_data.labels.size() * 2;
}

int ChartModel::rowCount(const QModelIndex& parent) const
{
    if (parent.isValid()) {
        return 0;
    } else {
        return m_data.rows.size();
    }
}

void ChartModel::resetData(const ChartData& data)
{
    Q_ASSERT(m_data.labels.size() < ChartRows::MAX_NUM_COST);
    beginResetModel();
    m_data = data;
    m_timestamps.clear();
    const int rows = rowCount();
    m_timestamps.reserve(rows);
    for (int row = 0; row < rows; ++row)
    {
        m_timestamps.append(m_data.rows[row].timeStamp);
    }
    m_columnDataSetBrushes.clear();
    m_columnDataSetPens.clear();
    const int columns = columnCount();
    for (int i = 0; i < columns; ++i) {
        auto color = colorForColumn(i, columns);
#ifdef QWT_FOUND
        color.setAlpha(i > 1 ? 127 : 50);
#endif
        m_columnDataSetBrushes << QBrush(color);
        m_columnDataSetPens << QPen(color);
    }
    endResetModel();
}

void ChartModel::clearData()
{
    beginResetModel();
    m_data = {};
    m_timestamps = {};
    m_columnDataSetBrushes = {};
    m_columnDataSetPens = {};
    endResetModel();
}

qint64 ChartModel::getTimestamp(int row) const
{
    return m_timestamps[row];
}

qint64 ChartModel::getCost(int row, int column) const
{
    return m_data.rows[row].cost[column / 2];
}

QString ChartModel::getColumnLabel(int column) const
{
    return m_data.labels.value(column / 2);
}

const QPen& ChartModel::getColumnDataSetPen(int column) const
{
    return m_columnDataSetPens[column];
}

const QBrush& ChartModel::getColumnDataSetBrush(int column) const
{
    return m_columnDataSetBrushes[column];
}

int ChartModel::getRowForTimestamp(qreal timestamp) const
{
    // check if 'timestamp' is not greater than the maximum available timestamp
    int lastIndex = m_timestamps.size() - 1;
    if (!((lastIndex >= 0) && (timestamp <= m_timestamps[lastIndex])))
    {
        return -1;
    }
    int result;
    // find the first element that is greater than 'timestamp'
    auto up_it = std::upper_bound(m_timestamps.begin(), m_timestamps.end(), (qint64)timestamp);
    if (up_it != m_timestamps.end())
    {
        result = up_it - m_timestamps.begin() - 1;
    }
    else // all elements are not greater than 'timestamp'
    {
        result = lastIndex; // due to check in the beginning of the function
    }
    return result;
}
