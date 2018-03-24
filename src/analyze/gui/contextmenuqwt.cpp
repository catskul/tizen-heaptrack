#include "contextmenuqwt.h"
#include "noklib.h"

#include <QKeyEvent>
#include <QMenu>

ContextMenuQwt::ContextMenuQwt(QObject *parent, bool isHistogram)
{
    m_showTotalAction = new QAction(i18n("Show &Total"), parent);
    m_showTotalAction->setStatusTip(i18n("Show the total amount curve"));
    m_showTotalAction->setCheckable(true);

    m_showUnresolvedAction = new QAction(i18n("Show &Unresolved"), parent);
    m_showUnresolvedAction->setStatusTip(i18n("Show unresolved functions' curves"));
    m_showUnresolvedAction->setCheckable(true);

    m_exportChartAction = new QAction(i18n("&Export Chart..."), parent);
    m_exportChartAction->setStatusTip(i18n("Export the current chart to a file."));

    // shortcuts don't work under Windows (Qt 5.10.0) so need to use a workaround (manual
    // processing in keyPressEvent)
    m_showTotalAction->setShortcut(QKeySequence(Qt::ALT | Qt::Key_T));
    m_showUnresolvedAction->setShortcut(QKeySequence(Qt::ALT | Qt::Key_U));
    m_exportChartAction->setShortcut(QKeySequence(Qt::ALT | Qt::Key_E));

#if QT_VERSION >= 0x050A00
    m_showTotalAction->setShortcutVisibleInContextMenu(true);
    m_showUnresolvedAction->setShortcutVisibleInContextMenu(true);
    m_exportChartAction->setShortcutVisibleInContextMenu(true);
#endif

    if (!isHistogram)
    {
        m_resetZoomAction = new QAction(i18n("Reset Zoom and Pan"), parent);
        m_resetZoomAction->setStatusTip(i18n("Reset the chart zoom and pan"));

        m_showLegendAction = new QAction(i18n("Show &Legend"), parent);
        m_showLegendAction->setStatusTip(i18n("Show the chart legend"));
        m_showLegendAction->setCheckable(true);

        m_showCurveBordersAction = new QAction(i18n("Show Curve &Borders"), parent);
        m_showCurveBordersAction->setStatusTip(i18n("Show curve borders (as black lines)"));
        m_showCurveBordersAction->setCheckable(true);

        m_showSymbolsAction = new QAction(i18n("Show &Symbols"), parent);
        m_showSymbolsAction->setStatusTip(i18n("Show symbols (the chart data points)"));
        m_showSymbolsAction->setCheckable(true);

        m_showVLinesAction = new QAction(i18n("Show &Vertical Lines"), parent);
        m_showVLinesAction->setStatusTip(i18n("Show vertical lines corresponding to timestamps"));
        m_showVLinesAction->setCheckable(true);

        m_showHelpAction = new QAction(i18n("Show Chart &Help"), parent);
        m_showHelpAction->setStatusTip(i18n("Show a window with breif help information inside the chart."));
        m_showHelpAction->setCheckable(true);

        m_resetZoomAction->setShortcut(QKeySequence(Qt::ALT | Qt::Key_R));
        m_showLegendAction->setShortcut(QKeySequence(Qt::ALT | Qt::Key_L));
        m_showCurveBordersAction->setShortcut(QKeySequence(Qt::ALT | Qt::Key_B));
        m_showSymbolsAction->setShortcut(QKeySequence(Qt::ALT | Qt::Key_S));
        m_showVLinesAction->setShortcut(QKeySequence(Qt::ALT | Qt::Key_V));

#if QT_VERSION >= 0x050A00
        m_resetZoomAction->setShortcutVisibleInContextMenu(true);
        m_showLegendAction->setShortcutVisibleInContextMenu(true);
        m_showCurveBordersAction->setShortcutVisibleInContextMenu(true);
        m_showSymbolsAction->setShortcutVisibleInContextMenu(true);
        m_showVLinesAction->setShortcutVisibleInContextMenu(true);
#endif
    }
    else
    {
        m_resetZoomAction = nullptr;
        m_showLegendAction = nullptr;
        m_showCurveBordersAction = nullptr;
        m_showSymbolsAction = nullptr;
        m_showVLinesAction = nullptr;
        m_showHelpAction = nullptr;
    }
}

void ContextMenuQwt::initializeMenu(QMenu& menu, ChartOptions::Options options) const
{
    if (m_resetZoomAction)
    {
        menu.addAction(m_resetZoomAction);
        menu.addSeparator();
    }
    if (m_showTotalAction)
    {
        m_showTotalAction->setChecked(ChartOptions::hasOption(options, ChartOptions::ShowTotal));
        menu.addAction(m_showTotalAction);
    }
    if (m_showUnresolvedAction)
    {
        m_showUnresolvedAction->setChecked(ChartOptions::hasOption(options, ChartOptions::ShowUnresolved));
        menu.addAction(m_showUnresolvedAction);
    }
    if ((m_showTotalAction != nullptr) || (m_showUnresolvedAction != nullptr))
    {
        menu.addSeparator();
    }
    if (m_showLegendAction)
    {
        m_showLegendAction->setChecked(ChartOptions::hasOption(options, ChartOptions::ShowLegend));
        menu.addAction(m_showLegendAction);
    }
    if (m_showCurveBordersAction)
    {
        m_showCurveBordersAction->setChecked(ChartOptions::hasOption(options, ChartOptions::ShowCurveBorders));
        menu.addAction(m_showCurveBordersAction);
    }
    if (m_showSymbolsAction)
    {
        m_showSymbolsAction->setChecked(ChartOptions::hasOption(options, ChartOptions::ShowSymbols));
        menu.addAction(m_showSymbolsAction);
    }
    if (m_showVLinesAction)
    {
        m_showVLinesAction->setChecked(ChartOptions::hasOption(options, ChartOptions::ShowVLines));
        menu.addAction(m_showVLinesAction);
    }
    if (m_exportChartAction)
    {
        menu.addSeparator();
        menu.addAction(m_exportChartAction);
    }
    if (m_showHelpAction)
    {
        menu.addSeparator();
        m_showHelpAction->setChecked(ChartOptions::hasOption(options, ChartOptions::ShowHelp));
        menu.addAction(m_showHelpAction);
    }
}

void ContextMenuQwt::handleKeyPress(QKeyEvent *event)
{
    if (event->modifiers() & Qt::AltModifier)
    {
        switch (event->key())
        {
        case Qt::Key_R:
            if (m_resetZoomAction)
            {
                m_resetZoomAction->activate(QAction::Trigger);
            }
            break;
        case Qt::Key_T:
            if (m_showTotalAction)
            {
                m_showTotalAction->activate(QAction::Trigger);
            }
            break;
        case Qt::Key_U:
            if (m_showUnresolvedAction)
            {
                m_showUnresolvedAction->activate(QAction::Trigger);
            }
            break;
        case Qt::Key_L:
            if (m_showLegendAction)
            {
                m_showLegendAction->activate(QAction::Trigger);
            }
            break;
        case Qt::Key_S:
            if (m_showSymbolsAction)
            {
                m_showSymbolsAction->activate(QAction::Trigger);
            }
            break;
        case Qt::Key_V:
            if (m_showVLinesAction)
            {
                m_showVLinesAction->activate(QAction::Trigger);
            }
        case Qt::Key_B:
            if (m_showCurveBordersAction)
            {
                m_showCurveBordersAction->activate(QAction::Trigger);
            }
            break;
        case Qt::Key_E:
            if (m_exportChartAction)
            {
                m_exportChartAction->activate(QAction::Trigger);
            }
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
