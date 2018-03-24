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

#include "histogramwidget.h"
#include "chartwidget.h"

#include <QSortFilterProxyModel>
#include <QVBoxLayout>

#if defined(KChart_FOUND)
#include <KChartBarDiagram>
#include <KChartChart>

#include <KChartBackgroundAttributes>
#include <KChartCartesianCoordinatePlane>
#include <KChartDataValueAttributes>
#include <KChartFrameAttributes.h>
#include <KChartGridAttributes>
#include <KChartHeaderFooter>
#include <KChartLegend>
#elif defined(QWT_FOUND)
#include <QMenu>
#endif

#ifdef NO_K_LIB
#include "noklib.h"
#else
#include <KColorScheme>
#include <KFormat>
#include <KLocalizedString>
#endif

#include "util.h"

#include "histogrammodel.h"

#ifdef KChart_FOUND
using namespace KChart;
#endif

namespace {
#if defined(KChart_FOUND)
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
#endif

class HistogramProxy : public QSortFilterProxyModel
{
    Q_OBJECT
public:
    explicit HistogramProxy(bool showTotal, QObject* parent = nullptr)
        : QSortFilterProxyModel(parent)
        , m_showTotal(showTotal)
    {
    }
    virtual ~HistogramProxy() = default;

protected:
    bool filterAcceptsColumn(int sourceColumn, const QModelIndex& /*sourceParent*/) const override
    {
        if (m_showTotal) {
            return sourceColumn == 0;
        } else {
            return sourceColumn != 0;
        }
    }

private:
    bool m_showTotal;
};
}

HistogramWidget::HistogramWidget(QWidget* parent)
    : QWidget(parent)
#if defined(KChart_FOUND)
    , m_chart(new KChart::Chart(this))
    , m_total(new BarDiagram(this))
    , m_detailed(new BarDiagram(this))
#elif defined(QWT_FOUND)
    , m_plot(new HistogramWidgetQwtPlot(this, ChartOptions::GlobalOptions))
    , m_contextMenuQwt(new ContextMenuQwt(this, true))
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

    connectContextMenu();
#endif
#ifdef SHOW_TABLES
    auto hLayout = new QHBoxLayout();
    hLayout->addWidget(m_tableViewTotal);
    hLayout->addWidget(m_tableViewNoTotal);
    hLayout->setStretch(0, 25);
    hLayout->setStretch(1, 75);
    layout->addLayout(hLayout);
    layout->setStretch(0, 100);
    layout->setStretch(1, 100);
#endif
    setLayout(layout);

#ifdef KChart_FOUND
    auto* coordinatePlane = dynamic_cast<CartesianCoordinatePlane*>(m_chart->coordinatePlane());
    Q_ASSERT(coordinatePlane);

    {
        m_total->setAntiAliasing(true);

#ifdef NO_K_LIB
        QPalette pal;
        const QPen foreground(pal.color(QPalette::Active, QPalette::Foreground));
#else
        KColorScheme scheme(QPalette::Active, KColorScheme::Window);
        QPen foreground(scheme.foreground().color());
#endif
        auto bottomAxis = new CartesianAxis(m_total);
        auto axisTextAttributes = bottomAxis->textAttributes();
        axisTextAttributes.setPen(foreground);
        bottomAxis->setTextAttributes(axisTextAttributes);
        auto axisTitleTextAttributes = bottomAxis->titleTextAttributes();
        axisTitleTextAttributes.setPen(foreground);
        bottomAxis->setTitleTextAttributes(axisTitleTextAttributes);
        bottomAxis->setPosition(KChart::CartesianAxis::Bottom);
        bottomAxis->setTitleText(i18n("Requested Allocation Size"));
        m_total->addAxis(bottomAxis);

        auto* rightAxis = new CartesianAxis(m_total);
        rightAxis->setTextAttributes(axisTextAttributes);
        rightAxis->setTitleTextAttributes(axisTitleTextAttributes);
        rightAxis->setTitleText(i18n("Number of Allocations"));
        rightAxis->setPosition(CartesianAxis::Right);
        m_total->addAxis(rightAxis);

        coordinatePlane->addDiagram(m_total);

        m_total->setType(BarDiagram::Normal);
    }

    {
        m_detailed->setAntiAliasing(true);

        coordinatePlane->addDiagram(m_detailed);

        m_detailed->setType(BarDiagram::Stacked);
    }
#endif
}

HistogramWidget::~HistogramWidget() = default;

void HistogramWidget::setModel(HistogramModel *model)
{
#if defined(KChart_FOUND) || defined(SHOW_TABLES)
    HistogramProxy *totalProxy, *proxy;
#endif
#if defined(KChart_FOUND)
    {
        totalProxy = new HistogramProxy(true, this);
        totalProxy->setSourceModel(model);
        m_total->setModel(totalProxy);
    }
    {
        proxy = new HistogramProxy(false, this);
        proxy->setSourceModel(model);
        m_detailed->setModel(proxy);
    }
#elif defined(QWT_FOUND)
    connect(model, SIGNAL(modelReset()), this, SLOT(modelReset()));
    m_plot->setModel(model);
#ifdef SHOW_TABLES
    totalProxy = new HistogramProxy(true, this);
    totalProxy->setSourceModel(model);

    proxy = new HistogramProxy(false, this);
    proxy->setSourceModel(model);
#endif // SHOW_TABLES
#endif // QWT_FOUND, KChart_FOUND
#ifdef SHOW_TABLES
    m_tableViewTotal->setModel(totalProxy);
    m_tableViewNoTotal->setModel(proxy);
#endif
}

void HistogramWidget::updateOnSelected()
{
    m_plot->setOptions(ChartOptions::GlobalOptions);
}

#ifdef QWT_FOUND
void HistogramWidget::modelReset()
{
    m_plot->rebuild();
}

void HistogramWidget::connectContextMenu()
{
    connect(m_contextMenuQwt->showTotalAction(), &QAction::triggered, this, &HistogramWidget::toggleShowTotal);
    connect(m_contextMenuQwt->showUnresolvedAction(), &QAction::triggered, this, &HistogramWidget::toggleShowUnresolved);
    connect(m_contextMenuQwt->exportChartAction(), &QAction::triggered, this, [=]() {
        Util::exportChart(this, *m_plot, "Allocation Histogram");
    });

    setFocusPolicy(Qt::StrongFocus);
}

#ifndef QT_NO_CONTEXTMENU
void HistogramWidget::contextMenuEvent(QContextMenuEvent *event)
{
    QMenu menu(this);
    m_contextMenuQwt->initializeMenu(menu, m_plot->options());
    menu.exec(event->globalPos());
}
#endif

void HistogramWidget::keyPressEvent(QKeyEvent *event)
{
    m_contextMenuQwt->handleKeyPress(event);
}

void HistogramWidget::toggleShowTotal()
{
    ChartOptions::GlobalOptions = m_plot->toggleOption(ChartOptions::ShowTotal);
}

void HistogramWidget::toggleShowUnresolved()
{
    ChartOptions::GlobalOptions = m_plot->toggleOption(ChartOptions::ShowUnresolved);
}
#endif // QWT_FOUND

#include "histogramwidget.moc"
