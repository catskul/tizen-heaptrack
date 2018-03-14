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

#ifndef HISTOGRAMWIDGET_H
#define HISTOGRAMWIDGET_H

#include "gui_config.h"

#include <QWidget>

//!! for debugging
//#define SHOW_TABLES

#ifdef SHOW_TABLES
#include <QTableView>
#endif

#if defined(KChart_FOUND)
namespace KChart {
class Chart;
class BarDiagram;
}
#elif defined(QWT_FOUND)
#include "histogramwidgetqwtplot.h"
#endif

class HistogramModel;

class HistogramWidget : public QWidget
{
    Q_OBJECT
public:
    explicit HistogramWidget(QWidget* parent = nullptr);
    virtual ~HistogramWidget();

    void setModel(HistogramModel* model);

#if defined(QWT_FOUND)
public slots:
    void modelReset();
#endif

private:
#if defined(KChart_FOUND)
    KChart::Chart* m_chart;
    KChart::BarDiagram* m_total;
    KChart::BarDiagram* m_detailed;
#elif defined(QWT_FOUND)
    HistogramWidgetQwtPlot* m_plot;
#endif
#ifdef SHOW_TABLES
    QTableView* m_tableViewTotal;
    QTableView* m_tableViewNoTotal;
#endif
};

#endif // HISTOGRAMWIDGET_H
