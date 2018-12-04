#include "histogramwidgetqwtplot.h"
#include "histogrammodel.h"
#include "util.h"
#ifdef NO_K_LIB
#include "noklib.h"
#endif

#include <math.h>

#include <qwt_column_symbol.h>
#include <qwt_plot_grid.h>
#include <qwt_plot_multi_barchart.h>
#include <qwt_plot_picker.h>
#include <qwt_scale_draw.h>
#include <qwt_text.h>

#include <QHash>
#include <QPair>
#include <QVector>

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

class BarSizes
{
public:
    BarSizes()
    {
        const int BarCount = 9;
        m_barRects.reserve(BarCount);
        m_barLeftRight.reserve(BarCount);
    }

    void clear()
    {
        m_barRects.clear();
        m_barLeftRight.clear();
    }

    void setBarSize(int sampleIndex, int barIndex, const QwtColumnRect &qwtRect)
    {
        if (m_barRects.size() <= sampleIndex)
        {
            m_barRects.resize(sampleIndex + 1);
            m_barLeftRight.resize(sampleIndex + 1);
        }
        QRectF rectF(qwtRect.toRect());
        QRect rect(floor(rectF.left()), floor(rectF.top()), ceil(rectF.width()), ceil(rectF.height()));
        m_barRects[sampleIndex][barIndex] = rect;
        m_barLeftRight[sampleIndex] = QPoint(rect.left(), rect.right());
    }

    bool findBar(const QPoint &pos, int& sampleIndex, int& barIndex) const
    {
        sampleIndex = -1;
        for (int i = 0, count = m_barLeftRight.size(); i < count; ++i)
        {
            if ((pos.x() >= m_barLeftRight[i].x()) && (pos.x() <= m_barLeftRight[i].y()))
            {
                sampleIndex = i;
                break;
            }
        }
        if (sampleIndex < 0)
        {
            barIndex = -1;
            return false;
        }
        // why deltaY = 1: relax the search condition (otherwise 'total' will be reported on inner edges)
        barIndex = findBarIndex(sampleIndex, pos.y(), 1);
        return (barIndex >= 0);
    }

private:
    // find an index of a bar of the sample with 'sampleIndex' index which contains
    // 'posY' y-coordinate; deltaY > 0 allows to increase the effective bar height
    int findBarIndex(int sampleIndex, int posY, int deltaY) const
    {
        int result = -1;
        const QHash<int, QRect>& sampleBars = m_barRects[sampleIndex];
        QHash<int, QRect>::const_iterator it = sampleBars.constBegin();
        while (it != sampleBars.constEnd())
        {
            const QRect& rect = it.value();
            if ((posY >= rect.top()) && (posY <= (rect.bottom() + deltaY)))
            {
                result = it.key();
                break;
            }
            ++it;
        }
        return result;
    }

    // 'm_barRects' vector index is 'sampleIndex'; each value is the map
    // from 'barIndex' to the bar coordinates last passed to 'drawBar'
    QVector<QHash<int, QRect>> m_barRects;

    // 'm_barLeftRight' vector index is 'sampleIndex'; each value stores
    // the left and right x-coordinates of this sample's bars
    QVector<QPoint> m_barLeftRight;
};

class Picker: public QwtPlotPicker
{
public:
    Picker(HistogramWidgetQwtPlot *plot)
        : QwtPlotPicker(QwtPlot::xBottom, QwtPlot::yRight, plot->canvas()), m_plot(plot)
    {
        setTrackerMode(QwtPlotPicker::AlwaysOn);
    }

    virtual void reset() override
    {
        QwtPlotPicker::reset();
        m_totalBarSizes.clear();
        m_barSizes.clear();
    }

    BarSizes m_totalBarSizes;
    BarSizes m_barSizes;

protected:
    virtual QwtText trackerText(const QPoint &pos) const
    {
//        qDebug() << "Picker: (" << pos.x() << "; " << pos.y() << ")";
        HistogramModel *model = m_plot->model();
        if (!model)
        {
            return {};
        }
        int sampleIndex, barIndex;
        QString s;
        if (m_barSizes.findBar(pos, sampleIndex, barIndex))
        {
            s = m_plot->getBarText(false, sampleIndex, barIndex);
//            s += QString("<br> Sample: %1. Bar: %2").arg(sampleIndex).arg(barIndex);
        }
        else if (m_plot->hasOption(ChartOptions::ShowTotal) &&
                 m_totalBarSizes.findBar(pos, sampleIndex, barIndex))
        {
            s = m_plot->getBarText(true, sampleIndex, barIndex);
//            s += QString("<br> (Total) Sample: %1. Bar: %2").arg(sampleIndex).arg(barIndex);
        }
        else
        {
            return {};
        }
        QwtText text("<p style='margin-left:4px'>" + s + "</p> ");
        text.setRenderFlags((text.renderFlags() & ~Qt::AlignHorizontal_Mask) | Qt::AlignLeft);
        text.setColor(Qt::white);
//        QColor c(Qt::darkGreen);
        QColor c(0, 0x60, 0);
        text.setBorderPen(QPen(c));
        text.setBorderRadius(6);
        c.setAlpha(170);
        text.setBackgroundBrush(c);
        return text;
    }

private:
    HistogramWidgetQwtPlot *m_plot;
};

class MultiBarChart: public QwtPlotMultiBarChart
{
public:
    MultiBarChart(BarSizes *barSizes) : m_barSizes(barSizes)
    {
    }

protected:
    virtual void drawBar( QPainter *painter, int sampleIndex,
        int barIndex, const QwtColumnRect &rect) const
    {
//        qDebug() << "drawBar: (sampleIndex=" << sampleIndex << "; barIndex=" << barIndex << "; " << rect.toRect() << ")";
        QwtPlotMultiBarChart::drawBar(painter, sampleIndex, barIndex, rect);

        m_barSizes->setBarSize(sampleIndex, barIndex, rect);
    }

private:
    BarSizes *m_barSizes;
};

HistogramWidgetQwtPlot::HistogramWidgetQwtPlot(QWidget *parent, Options options)
    : QwtPlot(parent), ChartOptions(options), m_model(nullptr), m_picker(new Picker(this))
{
    setCanvasBackground(Qt::white);
    enableAxis(QwtPlot::yRight);
    enableAxis(QwtPlot::yLeft, false);
    setAxisTitle(QwtPlot::yRight, i18n("Number of Allocations"));
    setAxisTitle(QwtPlot::xBottom, i18n("Requested Allocation Size"));
}

void HistogramWidgetQwtPlot::setOptions(Options options)
{
    if (m_options != options)
    {
        m_options = options;
        rebuild();
    }
}

void HistogramWidgetQwtPlot::rebuild()
{
    const double BarLayoutHint = 0.33;

    detachItems();
    m_picker->reset();

    if (!m_model)
    {
        return;
    }

    auto grid = new QwtPlotGrid();
    grid->setPen(QPen(Qt::lightGray));
    grid->attach(this);

    setAxisAutoScale(QwtPlot::yRight);

    MultiBarChart *totalBarChart = nullptr;
    if (hasOption(ShowTotal))
    {
        totalBarChart = new MultiBarChart(&m_picker->m_totalBarSizes);
        totalBarChart->setStyle(QwtPlotMultiBarChart::Stacked);
        totalBarChart->setLayoutHint(BarLayoutHint);
        totalBarChart->setLayoutPolicy(QwtPlotMultiBarChart::ScaleSamplesToAxes);
    }

    auto barChart = new MultiBarChart(&m_picker->m_barSizes);
    barChart->setStyle(QwtPlotMultiBarChart::Stacked);
    barChart->setLayoutHint(BarLayoutHint);
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

        if (totalBarChart)
        {
            QVector<double> totalValues;
            totalValues.append(m_model->data(m_model->index(row, 0)).toDouble());
            totalSeries.append(totalValues);
        }

        QVector<double> values;
        for (int column = 1; column < columns; ++column)
        {
            if (!hasOption(ShowUnresolved))
            {
                LocationData::Ptr locData = m_model->getLocationData(row, column);
                if (locData && Util::isUnresolvedFunction(locData->function))
                {
                    continue;
                }
            }
            double allocations = m_model->data(m_model->index(row, column)).toDouble();
            if (allocations == 0) // columns are sorted by allocations descending
            {
                break;
            }
            values.append(allocations);
            if (column > maxColumn)
            {
                maxColumn = column;
            }
        }
        if (values.isEmpty())
        {
            values.append(0);
        }
        series.append(values);
    }

    for (int column = (totalBarChart ? 0 : 1); column <= maxColumn; ++column)
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

    if (totalBarChart)
    {
        totalBarChart->setSamples(totalSeries);
        totalBarChart->setAxes(QwtPlot::xBottom, QwtPlot::yRight);
        totalBarChart->attach(this);
    }

    barChart->setSamples(series);

    auto bottomScale = new HistogramScaleDraw(rowNames);
    bottomScale->enableComponent(QwtScaleDraw::Backbone, false);
    bottomScale->enableComponent(QwtScaleDraw::Ticks, false);
    setAxisScaleDraw(QwtPlot::xBottom, bottomScale);

    barChart->setAxes(QwtPlot::xBottom, QwtPlot::yRight);

    barChart->attach(this);

    replot();
}

QString HistogramWidgetQwtPlot::getBarText(bool isTotal, int sampleIndex, int barIndex) const
{
    QString result;
    if (isTotal)
    {
        result = m_model->data(m_model->index(sampleIndex, 0), Qt::ToolTipRole).toString();
    }
    else
    {
        if (hasOption(ShowUnresolved))
        {
            result = m_model->data(m_model->index(sampleIndex, barIndex + 1), Qt::ToolTipRole).toString();
        }
        else // skip unresolved functions
        {
            int indexOfResolved = 0;
            int columns = m_model->columnCount();
            for (int column = 1; column < columns; ++column)
            {
                LocationData::Ptr locData = m_model->getLocationData(sampleIndex, column);
                if (locData && Util::isUnresolvedFunction(locData->function))
                {
                    continue;
                }
                if (indexOfResolved == barIndex)
                {
                    result = m_model->data(m_model->index(sampleIndex, column), Qt::ToolTipRole).toString();
                    break;
                }
                ++indexOfResolved;
            }
        }
    }
    return result;
}
