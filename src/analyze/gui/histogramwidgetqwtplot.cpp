#include "histogramwidgetqwtplot.h"
#include "histogrammodel.h"
#include "noklib.h"

#include <qwt_column_symbol.h>
#include <qwt_plot_grid.h>
#include <qwt_plot_multi_barchart.h>
#include <qwt_plot_picker.h>
#include <qwt_scale_draw.h>

class HistogramScaleDraw: public QwtScaleDraw
{
public:
    HistogramScaleDraw(QVector<QString>& rowNames) : m_rowNames(rowNames) { }

    virtual QwtText label(double value) const
    {
        qint64 index = (qint64)value;
        if ((index >= 0) && (index < m_rowNames.size()))
        {
            return m_rowNames[index];
        }
        return QwtScaleDraw::label(value);
    }

private:
    QVector<QString> m_rowNames;
};

class Picker: public QwtPlotPicker
{
public:
    Picker(HistogramWidgetQwtPlot *plot)
        : QwtPlotPicker(QwtPlot::xBottom, QwtPlot::yRight, plot->canvas())
    {
        setTrackerMode(QwtPlotPicker::AlwaysOn);
    }

protected:
    virtual QwtText trackerText(const QPoint &pos) const
    {
//        qDebug() << "Picker: (" << pos.x() << "; " << pos.y() << ")";
        return QwtText(QString(" (%1, %2) ").arg(pos.x()).arg(pos.y()));
    }
/*    virtual QwtText trackerTextF(const QPointF &pos) const
    {
//        qDebug() << "Picker: (" << pos.x() << "; " << pos.y() << ")";
        return QwtText(QString(" (%1, %2) ").arg(pos.x()).arg(pos.y()));
    }*/
};

class MultiBarChart: public QwtPlotMultiBarChart
{
public:

protected:
    virtual void drawBar( QPainter *painter, int sampleIndex,
        int barIndex, const QwtColumnRect &rect) const
    {
        qDebug() << "drawBar: (sampleIndex=" << sampleIndex << "; barIndex=" << barIndex
                 << "; " << rect.toRect() << ")";
        QwtPlotMultiBarChart::drawBar(painter, sampleIndex, barIndex, rect);
    }
};

HistogramWidgetQwtPlot::HistogramWidgetQwtPlot(QWidget *parent)
    : QwtPlot(parent), m_model(nullptr), m_picker(new Picker(this))
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

    if (!m_model)
    {
        return;
    }

    auto grid = new QwtPlotGrid();
    grid->setPen(QPen(Qt::lightGray));
    grid->attach(this);

    setAxisAutoScale(QwtPlot::yRight);

    auto totalBarChart = new MultiBarChart();
    totalBarChart->setStyle(QwtPlotMultiBarChart::Stacked);
    totalBarChart->setLayoutHint(0.33);
    totalBarChart->setLayoutPolicy(QwtPlotMultiBarChart::ScaleSamplesToAxes);

    auto barChart = new MultiBarChart();
    barChart->setStyle(QwtPlotMultiBarChart::Stacked);
    barChart->setLayoutHint(totalBarChart->layoutHint());
    barChart->setLayoutPolicy(QwtPlotMultiBarChart::ScaleSamplesToAxes);

    QVector<QString> rowNames;
    QVector<QVector<double>> totalSeries;
    QVector<QVector<double>> series;
    int rows = m_model->rowCount();
    int columns = m_model->columnCount();
    int maxColumn = 0;
    for (int row = 0; row < rows; ++row)
    {
        rowNames.append(m_model->headerData(row, Qt::Vertical).toString());

        QVector<double> totalValues;
        totalValues.append(m_model->data(m_model->index(row, 0)).toDouble());
        totalSeries.append(totalValues);

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

    for (int column = 0; column <= maxColumn; ++column)
    {
        auto symbol = new QwtColumnSymbol(QwtColumnSymbol::Box);
        symbol->setLineWidth(2);
        symbol->setPalette(QPalette(m_model->getColumnColor(column)));
        if (column > 0)
        {
            barChart->setSymbol(column - 1, symbol);
        }
        else
        {
            totalBarChart->setSymbol(0, symbol);
        }
    }

    totalBarChart->setSamples(totalSeries);
    barChart->setSamples(series);

    auto bottomScale = new HistogramScaleDraw(rowNames);
    bottomScale->enableComponent(QwtScaleDraw::Backbone, false);
    bottomScale->enableComponent(QwtScaleDraw::Ticks, false);
    setAxisScaleDraw(QwtPlot::xBottom, bottomScale);

    totalBarChart->setAxes(QwtPlot::xBottom, QwtPlot::yRight);
    barChart->setAxes(QwtPlot::xBottom, QwtPlot::yRight);

    totalBarChart->attach(this);
    barChart->attach(this);
}
