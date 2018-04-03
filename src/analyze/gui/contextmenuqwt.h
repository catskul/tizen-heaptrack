#ifndef CONTEXTMENUQWT_H
#define CONTEXTMENUQWT_H

#include "chartwidgetqwtplot.h"

#include <QAction>
#include <QKeyEvent>
#include <QMenu>

class ContextMenuQwt
{
public:
    ContextMenuQwt(QObject *parent, bool isHistogram);

    QAction* resetZoomAction() const { return m_resetZoomAction; }
    QAction* showTotalAction() const { return m_showTotalAction; }
    QAction* showUnresolvedAction() const { return m_showUnresolvedAction; }
    QAction* showLegendAction() const { return m_showLegendAction; }
    QAction* showSymbolsAction() const { return m_showSymbolsAction; }
    QAction* showVLinesAction() const { return m_showVLinesAction; }
    QAction* showCurveBordersAction() const { return m_showCurveBordersAction; }
    QAction* exportChartAction() const { return m_exportChartAction; }
    QAction* showHelpAction() const { return m_showHelpAction; }

    void initializeMenu(QMenu& menu, ChartOptions::Options options, bool isEmpty) const;

    bool handleKeyPress(QKeyEvent *event);

private:
    QAction* m_resetZoomAction;
    QAction* m_showTotalAction;
    QAction* m_showUnresolvedAction;
    QAction* m_showLegendAction;
    QAction* m_showSymbolsAction;
    QAction* m_showVLinesAction;
    QAction* m_showCurveBordersAction;
    QAction* m_exportChartAction;
    QAction* m_showHelpAction;
};

#endif // CONTEXTMENUQWT_H
