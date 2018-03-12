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

#ifndef CHARTWIDGET_H
#define CHARTWIDGET_H

#include "gui_config.h"

#include <QWidget>

//!! for debugging
//#define SHOW_TABLES

#ifdef SHOW_TABLES
#include <QTableView>
#endif

class ChartModel;

#if defined(KChart_FOUND)
namespace KChart {
class Chart;
}
#elif defined(QWT_FOUND)
#include "chartwidgetqwtplot.h"
class QAction;
#endif

class QAbstractItemModel;

class ChartWidget : public QWidget
{
    Q_OBJECT
public:
    explicit ChartWidget(QWidget* parent = nullptr);
    virtual ~ChartWidget();

    void setModel(ChartModel* model, bool minimalMode = false);

    QSize sizeHint() const override;

#if defined(QWT_FOUND)
    void updateIfOptionsChanged();

public slots:
    void modelReset();
#ifndef QT_NO_CONTEXTMENU
protected:
    virtual void contextMenuEvent(QContextMenuEvent *event) override;
#endif
    // workaround for handling the context menu shortcuts
    virtual void keyPressEvent(QKeyEvent *event) override;
#endif // QWT_FOUND

private:
#if defined(KChart_FOUND)
    KChart::Chart* m_chart;
#elif defined(QWT_FOUND)
private slots:
    void resetZoom();
    void toggleShowTotal(bool checked);
    void toggleShowLegend(bool checked);
    void toggleShowSymbols(bool checked);
    void toggleShowVLines(bool checked);
private:
    ChartWidgetQwtPlot* m_plot;

    QAction* m_resetZoomAction;
    QAction* m_showTotalAction;
    QAction* m_showLegendAction;
    QAction* m_showSymbolsAction;
    QAction* m_showVLinesAction;

    static ChartWidgetQwtPlot::Options globalOptions;
#endif
#ifdef SHOW_TABLES
    QTableView* m_tableViewTotal;
    QTableView* m_tableViewNoTotal;
#endif
};

#endif // CHARTWIDGET_H
