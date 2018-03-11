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

#include "chartwidget.h"

#include <QVBoxLayout>

#if defined(KChart_FOUND)
#include <KChartChart>
#include <KChartPlotter>

#include <KChartBackgroundAttributes>
#include <KChartCartesianCoordinatePlane>
#include <KChartDataValueAttributes>
#include <KChartFrameAttributes.h>
#include <KChartGridAttributes>
#include <KChartHeaderFooter>
#include <KChartLegend>
#elif defined(QWT_FOUND)
#include <qwt_plot.h>
#include <qwt_plot_curve.h>
#include <qwt_plot_grid.h>
#include <qwt_plot_marker.h>
#include <qwt_symbol.h>
#include <qwt_legend.h>
#include <qwt_scale_draw.h>
#include <QAction>
#include <QContextMenuEvent>
#include <QMenu>
#include <QRegularExpression>
#endif

#ifdef NO_K_LIB
#include "noklib.h"
#else
#include <KColorScheme>
#include <KFormat>
#include <KLocalizedString>
#endif

#include "chartmodel.h"
#include "chartproxy.h"
#include "util.h"

#ifdef KChart_FOUND
using namespace KChart;

namespace {
class TimeAxis : public CartesianAxis
{
    Q_OBJECT
public:
    explicit TimeAxis(AbstractCartesianDiagram* diagram = nullptr)
        : CartesianAxis(diagram)
    {
    }

    const QString customizedLabel(const QString& label) const override
    {
        return Util::formatTime(label.toLongLong());
    }
};

class SizeAxis : public CartesianAxis
{
    Q_OBJECT
public:
    explicit SizeAxis(AbstractCartesianDiagram* diagram = nullptr)
        : CartesianAxis(diagram)
    {
    }

    const QString customizedLabel(const QString& label) const override
    {
        return Util::formatByteSize(label.toDouble(), 1);
    }
};
}
#elif defined(QWT_FOUND)
class TimeScaleDraw: public QwtScaleDraw
{
    virtual QwtText label(double value) const
    {
        return Util::formatTime((qint64)value);
    }
};

class SizeScaleDraw: public QwtScaleDraw
{
    virtual QwtText label(double value) const
    {
        return Util::formatByteSize(value, 1);
    }
};

bool ChartWidget::globalShowTotal = true;
bool ChartWidget::globalShowLegend = true;
bool ChartWidget::globalShowSymbols = false;
bool ChartWidget::globalShowVLines = false;
#endif

ChartWidget::ChartWidget(QWidget* parent)
    : QWidget(parent)
#if defined(KChart_FOUND)
    , m_chart(new Chart(this))
#elif defined(QWT_FOUND)
    , m_model(nullptr)
    , m_plot(new QwtPlot(this))
    , m_showTotal(globalShowTotal)
    , m_showLegend(globalShowLegend)
    , m_showSymbols(globalShowSymbols)
    , m_showVLines(globalShowVLines)
#endif
#ifdef SHOW_TABLES
    , m_tableViewTotal(new QTableView(this))
    , m_tableViewNoTotal(new QTableView(this))
#endif
{
    auto layout = new QVBoxLayout(this);
#if defined(KChart_FOUND)
    layout->addWidget(m_chart);
#elif defined(QWT_FOUND)
    layout->addWidget(m_plot);
    m_vLinePen.setStyle(Qt::DashLine);
    m_vLinePen.setColor(Qt::gray);

    m_showTotalAction = new QAction(i18n("Show Total"), this);
    m_showTotalAction->setStatusTip(i18n("Show the total amount curve"));
    m_showTotalAction->setCheckable(true);
    connect(m_showTotalAction, &QAction::triggered, this, &ChartWidget::toggleShowTotal);

    m_showLegendAction = new QAction(i18n("Show Legend"), this);
    m_showLegendAction->setStatusTip(i18n("Show the chart legend"));
    m_showLegendAction->setCheckable(true);
    connect(m_showLegendAction, &QAction::triggered, this, &ChartWidget::toggleShowLegend);

    m_showSymbolsAction = new QAction(i18n("Show Symbols"), this);
    m_showSymbolsAction->setStatusTip(i18n("Show symbols (chart data points)"));
    m_showSymbolsAction->setCheckable(true);
    connect(m_showSymbolsAction, &QAction::triggered, this, &ChartWidget::toggleShowSymbols);

    m_showVLinesAction = new QAction(i18n("Show Vertical Lines"), this);
    m_showVLinesAction->setStatusTip(i18n("Show vertical lines corresponding to timestamps"));
    m_showVLinesAction->setCheckable(true);
    connect(m_showVLinesAction, &QAction::triggered, this, &ChartWidget::toggleShowVLines);

    // shortcuts don't work under Windows (Qt 5.10.0) so using a workaround (manual processing
    // in keyPressEvent)

    m_showTotalAction->setShortcut(QKeySequence(Qt::ALT | Qt::Key_T));
    m_showLegendAction->setShortcut(QKeySequence(Qt::ALT | Qt::Key_L));
    m_showSymbolsAction->setShortcut(QKeySequence(Qt::ALT | Qt::Key_S));
    m_showVLinesAction->setShortcut(QKeySequence(Qt::ALT | Qt::Key_V));
#if QT_VERSION >= 0x050A00
    m_showTotalAction->setShortcutVisibleInContextMenu(true);
    m_showLegendAction->setShortcutVisibleInContextMenu(true);
    m_showSymbolsAction->setShortcutVisibleInContextMenu(true);
    m_showVLinesAction->setShortcutVisibleInContextMenu(true);
#endif
    setFocusPolicy(Qt::StrongFocus);
#endif
#ifdef SHOW_TABLES
    auto hLayout = new QHBoxLayout();
    hLayout->addWidget(m_tableViewTotal);
    hLayout->addWidget(m_tableViewNoTotal);
    layout->addLayout(hLayout);
    layout->setStretch(0, 100);
    layout->setStretch(1, 100);
#endif
    setLayout(layout);

#ifdef KChart_FOUND
    auto* coordinatePlane = dynamic_cast<CartesianCoordinatePlane*>(m_chart->coordinatePlane());
    Q_ASSERT(coordinatePlane);
    coordinatePlane->setRubberBandZoomingEnabled(true);
    coordinatePlane->setAutoAdjustGridToZoom(true);
#endif
}

ChartWidget::~ChartWidget() = default;

void ChartWidget::setModel(ChartModel* model, bool minimalMode)
{
#ifdef KChart_FOUND
    auto* coordinatePlane = dynamic_cast<CartesianCoordinatePlane*>(m_chart->coordinatePlane());
    Q_ASSERT(coordinatePlane);
    foreach (auto diagram, coordinatePlane->diagrams()) {
        coordinatePlane->takeDiagram(diagram);
        delete diagram;
    }

    if (minimalMode) {
        KChart::GridAttributes grid;
        grid.setSubGridVisible(false);
        coordinatePlane->setGlobalGridAttributes(grid);
    }
#endif

    switch (model->type()) {
    case ChartModel::Consumed:
        setToolTip(i18n("<qt>Shows the heap memory consumption over time.</qt>"));
        break;
    case ChartModel::Instances:
        setToolTip(i18n("<qt>Shows the number of instances allocated from specific functions over time.</qt>"));
        break;
    case ChartModel::Allocated:
        setToolTip(i18n("<qt>Displays total memory allocated over time. "
                        "This value ignores deallocations and just measures heap "
                        "allocation throughput.</qt>"));
        break;
    case ChartModel::Allocations:
        setToolTip(i18n("<qt>Shows number of memory allocations over time.</qt>"));
        break;
    case ChartModel::Temporary:
        setToolTip(i18n("<qt>Shows number of temporary memory allocations over time. "
                        "A temporary allocation is one that is followed immediately by its "
                        "corresponding deallocation, without other allocations happening "
                        "in-between.</qt>"));
        break;
    }

#if defined(KChart_FOUND) || defined(SHOW_TABLES)
    ChartProxy *totalProxy, *proxy;
#endif
#if defined(KChart_FOUND)
    {
        auto totalPlotter = new Plotter(this);
        totalPlotter->setAntiAliasing(true);
        totalProxy = new ChartProxy(true, this);
        totalProxy->setSourceModel(model);
        totalPlotter->setModel(totalProxy);
        totalPlotter->setType(Plotter::Stacked);

#ifdef NO_K_LIB
        QPalette pal;
        const QPen foreground(pal.color(QPalette::Active, QPalette::Foreground));
#else
        KColorScheme scheme(QPalette::Active, KColorScheme::Window);
        const QPen foreground(scheme.foreground().color());
#endif
        auto bottomAxis = new TimeAxis(totalPlotter);
        auto axisTextAttributes = bottomAxis->textAttributes();
        axisTextAttributes.setPen(foreground);
        bottomAxis->setTextAttributes(axisTextAttributes);
        auto axisTitleTextAttributes = bottomAxis->titleTextAttributes();
        axisTitleTextAttributes.setPen(foreground);
        auto fontSize = axisTitleTextAttributes.fontSize();
        fontSize.setCalculationMode(KChartEnums::MeasureCalculationModeAbsolute);
        if (minimalMode) {
            fontSize.setValue(font().pointSizeF() - 2);
        } else {
            fontSize.setValue(font().pointSizeF() + 2);
        }
        axisTitleTextAttributes.setFontSize(fontSize);
        bottomAxis->setTitleTextAttributes(axisTitleTextAttributes);
        bottomAxis->setTitleText(model->headerData(0).toString());
        bottomAxis->setPosition(CartesianAxis::Bottom);
        totalPlotter->addAxis(bottomAxis);

        CartesianAxis* rightAxis = (model->type() == ChartModel::Allocations
                                    || model->type() == ChartModel::Instances
                                    || model->type() == ChartModel::Temporary)
            ? new CartesianAxis(totalPlotter)
            : new SizeAxis(totalPlotter);
        rightAxis->setTextAttributes(axisTextAttributes);
        rightAxis->setTitleTextAttributes(axisTitleTextAttributes);
        rightAxis->setTitleText(model->headerData(1).toString());
        rightAxis->setPosition(CartesianAxis::Right);
        totalPlotter->addAxis(rightAxis);

        coordinatePlane->addDiagram(totalPlotter);
    }

    {
        auto plotter = new Plotter(this);
        plotter->setAntiAliasing(true);
        plotter->setType(Plotter::Stacked);

        proxy = new ChartProxy(false, this);
        proxy->setSourceModel(model);
        plotter->setModel(proxy);
        coordinatePlane->addDiagram(plotter);
    }
#elif defined(QWT_FOUND)
    connect(model, SIGNAL(modelReset()), this, SLOT(modelReset()));
    m_model = model;

    m_plot->setCanvasBackground(Qt::white);
    m_plot->enableAxis(QwtPlot::yRight);
    m_plot->enableAxis(QwtPlot::yLeft, false);

#ifdef SHOW_TABLES
    totalProxy = new ChartProxy(true, this);
    totalProxy->setSourceModel(model);

    proxy = new ChartProxy(false, this);
    proxy->setSourceModel(model);
#endif // SHOW_TABLES
#endif // QWT_FOUND
#ifdef SHOW_TABLES
    m_tableViewTotal->setModel(totalProxy);
    m_tableViewNoTotal->setModel(proxy);
#endif
}

QSize ChartWidget::sizeHint() const
{
    return {400, 50};
}

#ifdef QWT_FOUND
void ChartWidget::modelReset()
{
    updateQwtChart();
}

#ifndef QT_NO_CONTEXTMENU
void ChartWidget::contextMenuEvent(QContextMenuEvent *event)
{
    QMenu menu(this);
    m_showTotalAction->setChecked(m_showTotal);
    menu.addAction(m_showTotalAction);
    menu.addSeparator();
    m_showLegendAction->setChecked(m_showLegend);
    menu.addAction(m_showLegendAction);
    m_showSymbolsAction->setChecked(m_showSymbols);
    menu.addAction(m_showSymbolsAction);
    m_showVLinesAction->setChecked(m_showVLines);
    menu.addAction(m_showVLinesAction);
    menu.exec(event->globalPos());
}
#endif

void ChartWidget::keyPressEvent(QKeyEvent *event)
{
    if (event->modifiers() & Qt::AltModifier)
    {
        switch (event->key())
        {
        case Qt::Key_T:
            toggleShowTotal(!globalShowTotal);
            break;
        case Qt::Key_L:
            toggleShowLegend(!globalShowLegend);
            break;
        case Qt::Key_S:
            toggleShowSymbols(!globalShowSymbols);
            break;
        case Qt::Key_V:
            toggleShowVLines(!globalShowVLines);
            break;
        default:
            event->ignore();
            return;
        }
        event->accept();
    }
    else
    {
        event->ignore();
    }
}

void ChartWidget::toggleShowTotal(bool checked)
{
    globalShowTotal = checked;
    updateIfOptionsChanged();
}

void ChartWidget::toggleShowLegend(bool checked)
{
    globalShowLegend = checked;
    updateIfOptionsChanged();
}

void ChartWidget::toggleShowSymbols(bool checked)
{
    globalShowSymbols = checked;
    updateIfOptionsChanged();
}

void ChartWidget::toggleShowVLines(bool checked)
{
    globalShowVLines = checked;
    updateIfOptionsChanged();
}

void ChartWidget::updateIfOptionsChanged()
{
    bool update = false;
    if (m_showTotal != globalShowTotal)
    {
        m_showTotal = globalShowTotal;
        update = true;
    }
    if (m_showLegend != globalShowLegend)
    {
        m_showLegend = globalShowLegend;
        update = true;
    }
    if (m_showSymbols != globalShowSymbols)
    {
        m_showSymbols = globalShowSymbols;
        update = true;
    }
    if (m_showVLines != globalShowVLines)
    {
        m_showVLines = globalShowVLines;
        update = true;
    }
    if (update)
    {
        updateQwtChart();
    }
}

static QString getCurveTitle(QString label)
{
    const int MaxLineLength = 48;

    int labelLength = label.size();
    if (labelLength <= MaxLineLength)
    {
        return label;
    }
    static QRegularExpression delimBefore("[(<]");
    static QRegularExpression delimAfter("[- .,)>\\/]");
    QString result;
    do
    {
        int i = -1;
        int wrapAfter = 0;
        int i1 = label.indexOf(delimBefore, MaxLineLength);
        int i2 = label.indexOf(delimAfter, MaxLineLength - 1);
        if (i1 >= 0)
        {
            if (i2 >= 0)
            {
                if (i2 < i1)
                {
                    i = i2;
                    wrapAfter = 1;
                }
                else
                {
                    i = i1;
                }
            }
            else
            {
                i = i1;
            }
        }
        else
        {
            i = i2;
            wrapAfter = 1;
        }
        if (i < 0)
        {
            break;
        }
        i += wrapAfter;
        result += label.left(i).toHtmlEscaped();
        label.remove(0, i);
        if (label.isEmpty()) // special: avoid <br> at the end
        {
            return result;
        }
        result += "<br>";
        labelLength -= i;
    }
    while (labelLength > MaxLineLength);
    result += label.toHtmlEscaped();
    return result;
}

void ChartWidget::updateQwtChart()
{
    int columns = m_model->columnCount();
    int rows = m_model->rowCount();

    m_plot->detachItems();

    m_plot->insertLegend(m_showLegend ? new QwtLegend() : nullptr);

    m_plot->setAxisTitle(QwtPlot::xBottom, m_model->headerData(0).toString());
    m_plot->setAxisTitle(QwtPlot::yRight, m_model->headerData(1).toString());
    m_plot->setAxisScaleDraw(QwtPlot::xBottom, new TimeScaleDraw());
    if (!(m_model->type() == ChartModel::Allocations || m_model->type() == ChartModel::Instances ||
          m_model->type() == ChartModel::Temporary))
    {
        m_plot->setAxisScaleDraw(QwtPlot::yRight, new SizeScaleDraw());
    }

    auto grid = new QwtPlotGrid();
    grid->attach(m_plot);

    int column = 1;
    if (!m_showTotal)
    {
        column += 2;
    }
    for (; column < columns; column += 2)
    {
        auto curve = new QwtPlotCurve(getCurveTitle(m_model->getColumnLabel(column)));
        curve->setRenderHint(QwtPlotItem::RenderAntialiased, true);
        curve->setYAxis(QwtPlot::yRight);

        curve->setPen(m_model->getColumnDataSetPen(column - 1));
        curve->setBrush(m_model->getColumnDataSetBrush(column - 1));

        auto adapter = new ChartModel2QwtSeriesData(m_model, column);
        curve->setSamples(adapter);

        if (m_showSymbols)
        {
            QwtSymbol *symbol = new QwtSymbol(QwtSymbol::Ellipse,
                QBrush(Qt::white), QPen(Qt::black, 2), QSize(6, 6));
            curve->setSymbol(symbol);
        }

        curve->attach(m_plot);
    }

    if (m_showVLines)
    {
        for (int row = 1; row < rows; ++row)
        {
            auto marker = new QwtPlotMarker();
            marker->setLinePen(m_vLinePen);
            marker->setLineStyle(QwtPlotMarker::VLine);
            marker->setXValue(m_model->getTimestamp(row));
            marker->attach(m_plot);
        }
    }

    m_plot->replot();
}
#endif // QWT_FOUND

#include "chartwidget.moc"
