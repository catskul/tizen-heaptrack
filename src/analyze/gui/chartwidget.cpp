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

#include <QMainWindow>
#include <QTextEdit>
#include <QToolTip>
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
#include <QAction>
#include <QContextMenuEvent>
#include <QFileDialog>
#include <QFileInfo>
#include <QMenu>
#include <QMessageBox>
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
class HelpWidget : public QMainWindow
{
    Q_OBJECT
public:
    explicit HelpWidget(QWidget *parent) : QMainWindow(parent)
    {
        setWindowTitle("Chart Help");
        setWindowFlags(Qt::Tool);
        setAttribute(Qt::WA_ShowWithoutActivating);
        setAttribute(Qt::WA_DeleteOnClose);
        setGeometry(0, 0, 292, 164);
        setMinimumSize(100, 70);
        setMaximumSize(400, 280);
        setPalette(QToolTip::palette());
        setWindowOpacity(0.9);
        setAutoFillBackground(true);
        auto textEdit = new QTextEdit(this);
        textEdit->setReadOnly(true);
        textEdit->setContextMenuPolicy(Qt::NoContextMenu);
        textEdit->viewport()->setAutoFillBackground(false);
        setCentralWidget(textEdit);
        textEdit->setHtml(
            "<p>Use <u>Context Menu</u> (right click inside the chart to open) to control different chart options.</p>" \
            "<p>Use <u>left mouse button</u> to <b>zoom in</b> to selection: press the button, drag " \
            "to make a rectangular selection, release.</p>" \
            "<p>Use <u>left mouse button</u> with modifier keys to:</p>" \
            "<ul>" \
            "<li><b>zoom out</b> (one step back) - <b>&lt;Shift&gt;</b>+click;" \
            "<li><b>reset zoom</b> - <b>&lt;Ctrl&gt;</b>+click;" \
            "<li><b>move (pan)</b> the chart  - <b>&lt;Alt&gt;</b>+drag." \
            "</ul>");
    }

    virtual ~HelpWidget()
    {
        ChartWidget::HelpWindow = nullptr;
    }
protected:
    virtual void closeEvent(QCloseEvent *event) override
    {
        QMainWindow::closeEvent(event);
        ChartWidget::GlobalOptions = ChartOptions::setOption(ChartWidget::GlobalOptions, ChartOptions::ShowHelp, false);
    }
};

ChartOptions::Options ChartWidget::GlobalOptions(
    ChartOptions::ShowHelp |
    ChartOptions::ShowTotal | ChartOptions::ShowUnresolved |
    ChartOptions::ShowLegend | ChartOptions::ShowCurveBorders);

QWidget* ChartWidget::HelpWindow;
QWidget* ChartWidget::MainWindow;
#endif // QWT_FOUND

ChartWidget::ChartWidget(QWidget* parent)
    : QWidget(parent)
#if defined(KChart_FOUND)
    , m_chart(new Chart(this))
#elif defined(QWT_FOUND)
    , m_plot(new ChartWidgetQwtPlot(this, GlobalOptions))
    , m_contextMenuQwt(new ContextMenuQwt(this, false))
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

    createActions();
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

#ifdef QWT_FOUND
void ChartWidget::updateOnSelected(QWidget *mainWindow)
{
    MainWindow = mainWindow;
    m_plot->setOptions(GlobalOptions);
    if (m_plot->hasOption(ChartWidgetQwtPlot::ShowHelp))
    {
        showHelp();
    }
}

void ChartWidget::createActions()
{
    connect(m_contextMenuQwt->resetZoomAction(), &QAction::triggered, this, &ChartWidget::resetZoom);
    connect(m_contextMenuQwt->showTotalAction(), &QAction::triggered, this, &ChartWidget::toggleShowTotal);
    connect(m_contextMenuQwt->showUnresolvedAction(), &QAction::triggered, this, &ChartWidget::toggleShowUnresolved);
    connect(m_contextMenuQwt->showLegendAction(), &QAction::triggered, this, &ChartWidget::toggleShowLegend);
    connect(m_contextMenuQwt->showCurveBordersAction(), &QAction::triggered, this, &ChartWidget::toggleShowCurveBorders);
    connect(m_contextMenuQwt->showSymbolsAction(), &QAction::triggered, this, &ChartWidget::toggleShowSymbols);
    connect(m_contextMenuQwt->showVLinesAction(), &QAction::triggered, this, &ChartWidget::toggleShowVLines);
    connect(m_contextMenuQwt->exportChartAction(), &QAction::triggered, this, &ChartWidget::exportChart);
    connect(m_contextMenuQwt->showHelpAction(), &QAction::triggered, this, &ChartWidget::toggleShowHelp);

    setFocusPolicy(Qt::StrongFocus);
}
#endif

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
        setToolTip(i18n("<qt>Displays the total memory allocated over time. "
                        "This value ignores deallocations and just measures heap "
                        "allocation throughput.</qt>"));
        break;
    case ChartModel::Allocations:
        setToolTip(i18n("<qt>Shows the number of memory allocations over time.</qt>"));
        break;
    case ChartModel::Temporary:
        setToolTip(i18n("<qt>Shows the number of temporary memory allocations over time. "
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
    m_plot->setModel(model);
#ifdef SHOW_TABLES
    totalProxy = new ChartProxy(true, this);
    totalProxy->setSourceModel(model);

    proxy = new ChartProxy(false, this);
    proxy->setSourceModel(model);
#endif // SHOW_TABLES
#endif // QWT_FOUND, KChart_FOUND
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
    m_plot->rebuild(true);
}

#ifndef QT_NO_CONTEXTMENU
void ChartWidget::contextMenuEvent(QContextMenuEvent *event)
{
    QMenu menu(this);
    m_plot->setOption(ChartWidgetQwtPlot::ShowHelp,
                      ChartWidgetQwtPlot::hasOption(GlobalOptions, ChartWidgetQwtPlot::ShowHelp));
    m_contextMenuQwt->initializeMenu(menu, m_plot->options());
    menu.exec(event->globalPos());
}
#endif

void ChartWidget::keyPressEvent(QKeyEvent *event)
{
    m_contextMenuQwt->handleKeyPress(event, m_plot->options());
}

void ChartWidget::resetZoom()
{
    m_plot->resetZoom();
}

void ChartWidget::toggleShowTotal()
{
    GlobalOptions = m_plot->toggleOption(ChartWidgetQwtPlot::ShowTotal);
}

void ChartWidget::toggleShowUnresolved()
{
    GlobalOptions = m_plot->toggleOption(ChartWidgetQwtPlot::ShowUnresolved);
}

void ChartWidget::toggleShowLegend()
{
    GlobalOptions = m_plot->toggleOption(ChartWidgetQwtPlot::ShowLegend);
}

void ChartWidget::toggleShowCurveBorders()
{
    GlobalOptions = m_plot->toggleOption(ChartWidgetQwtPlot::ShowCurveBorders);
}

void ChartWidget::toggleShowSymbols()
{
    GlobalOptions = m_plot->toggleOption(ChartWidgetQwtPlot::ShowSymbols);
}

void ChartWidget::toggleShowVLines()
{
    GlobalOptions = m_plot->toggleOption(ChartWidgetQwtPlot::ShowVLines);
}

void ChartWidget::toggleShowHelp()
{
    bool checked = !ChartOptions::hasOption(GlobalOptions, ChartOptions::ShowHelp);
    if (checked)
    {
        showHelp();
    }
    else
    {
        if (HelpWindow != nullptr)
        {
            delete HelpWindow;
            HelpWindow = nullptr;
        }
    }
    GlobalOptions = ChartOptions::setOption(GlobalOptions, ChartOptions::ShowHelp, checked);
}

void ChartWidget::showHelp()
{
    if (HelpWindow == nullptr)
    {
        HelpWindow = new HelpWidget(MainWindow);
        QPoint p = mapToGlobal(pos());
        HelpWindow->move(p.x() + 32, p.y() + 32);
    }
    HelpWindow->show();
}

void ChartWidget::exportChart()
{
    QString selectedFilter;
    QString saveFilename = QFileDialog::getSaveFileName(this, "Save Chart As",
        m_plot->model()->headerData(1, Qt::Horizontal).toString(),
        "PNG (*.png);; BMP (*.bmp);; JPEG (*.jpg *.jpeg)", &selectedFilter);
    if (!saveFilename.isEmpty())
    {
        QFileInfo fi(saveFilename);
        if (fi.suffix().isEmpty()) // can be on some platforms
        {
            int i = selectedFilter.indexOf("*.");
            if (i >= 0)
            {
                static QRegularExpression delimiters("[ )]");
                i += 2;
                int j = selectedFilter.indexOf(delimiters, i);
                if (j > i)
                {
                    --i;
                    QString suffix = selectedFilter.mid(i, j - i);
                    saveFilename += suffix;
                }
            }
        }
        if (!m_plot->grab().save(saveFilename))
        {
            QMessageBox::warning(this, "Error",
                QString("Cannot save the chart to \"%1\".").arg(saveFilename), QMessageBox::Ok);
        }
    }
}
#endif // QWT_FOUND

#include "chartwidget.moc"
