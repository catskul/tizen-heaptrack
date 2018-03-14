#include "histogramwidgetqwtplot.h"
#include "histogrammodel.h"
#include "noklib.h"

#include <qwt_column_symbol.h>
#include <qwt_plot_multi_barchart.h>

HistogramWidgetQwtPlot::HistogramWidgetQwtPlot(QWidget *parent)
    : QwtPlot(parent),
      m_barChart(nullptr)
{
    setCanvasBackground(Qt::white);
    enableAxis(QwtPlot::yRight);
    enableAxis(QwtPlot::yLeft, false);
    setAxisTitle(QwtPlot::yRight, i18n("Number of Allocations"));
    setAxisTitle(QwtPlot::xBottom, i18n("Requested Allocation Size"));
}

void HistogramWidgetQwtPlot::rebuild(bool resetZoomAndPan)
{
    detachItems();
    m_barChart = nullptr;

    if (!m_model)
    {
        return;
    }

    m_barChart = new QwtPlotMultiBarChart();
    m_barChart->setSpacing(40); // TODO!! use dynamic spacing
    m_barChart->setStyle(QwtPlotMultiBarChart::Stacked);

    int columns = m_model->columnCount();
    int rows = m_model->rowCount();

    int maxColumn = 0;
    QVector<QVector<double>> series;
    for (int row = 0; row < rows; ++row)
    {
        QString rowName = m_model->headerData(row, Qt::Vertical).toString();
        QVector<double> values;
        for (int column = 1; column < columns; ++column)
        {
            double allocations = m_model->data(m_model->index(row, column)).toDouble();
            if ((allocations == 0) && (column > 1))
            {
                break;
            }
            values.append(allocations);
            if (column > maxColumn)
            {
                maxColumn = column;
            }
        }
        series.append(values);
    }

    for (int column = 1; column <= maxColumn; ++column)
    {
        auto symbol = new QwtColumnSymbol(QwtColumnSymbol::Box);
        symbol->setLineWidth(2);
        symbol->setPalette(QPalette(m_model->getColumnColor(column)));
        m_barChart->setSymbol(column - 1, symbol);
    }

    m_barChart->setSamples(series);

    setAxisAutoScale(QwtPlot::yRight);
    m_barChart->setAxes(QwtPlot::xBottom, QwtPlot::yRight);

    m_barChart->attach(this);
}
