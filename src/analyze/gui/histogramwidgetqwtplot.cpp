#include "histogramwidgetqwtplot.h"
#include "histogrammodel.h"
#include "noklib.h"

#include <qwt_column_symbol.h>
#include <qwt_plot_multi_barchart.h>

void populate(QwtPlotMultiBarChart *);

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

// TODO!! remove test code
static void populate(QwtPlotMultiBarChart *m_barChart)
{
//    static const char *colors[] = { "DarkOrchid", "SteelBlue", "Gold" };
    static const char *colors[] = { "red", "green", "blue" };

    const int numSamples = 5;
    const int numBars = sizeof( colors ) / sizeof( colors[0] );

    QList<QwtText> titles;
    for ( int i = 0; i < numBars; i++ )
    {
        QString title("Bar %1");
        titles += title.arg( i );
    }
    m_barChart->setBarTitles( titles );
//!!    m_barChart->setLegendIconSize( QSize( 10, 14 ) );

    for ( int i = 0; i < numBars; i++ )
    {
        QwtColumnSymbol *symbol = new QwtColumnSymbol( QwtColumnSymbol::Box );
        symbol->setLineWidth( 2 );
//        symbol->setFrameStyle( QwtColumnSymbol::Raised );
        QColor c(colors[i]);
        c.setAlpha(160);
        symbol->setPalette(QPalette(c));

        m_barChart->setSymbol(i, symbol);
    }

    QVector< QVector<double> > series;
    for ( int i = 0; i < numSamples; i++ )
    {
        QVector<double> values;
        values.append(200);
        values.append(300);
        values.append(500);
/*        for ( int j = 0; j < numBars; j++ )
            values += ( 2 + qrand() % 8 );*/

        series += values;
    }

    m_barChart->setSamples(series);
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
    m_barChart->attach(this);

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
}
