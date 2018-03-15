#include "chartwidgetqwtplot.h"
#include "chartmodel.h"
#include "chartmodel2qwtseriesdata.h"
#include "util.h"

#include <qwt_legend.h>
#include <qwt_plot_curve.h>
#include <qwt_plot_grid.h>
#include <qwt_plot_marker.h>
#include <qwt_plot_panner.h>
#include <qwt_plot_zoomer.h>
#include <qwt_scale_draw.h>
#include <qwt_symbol.h>

#include <QRegularExpression>

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
        return Util::formatByteSize(value);
    }
};

class Zoomer: public QwtPlotZoomer
{
public:
    Zoomer(ChartWidgetQwtPlot *plot)
        : QwtPlotZoomer(QwtPlot::xBottom, QwtPlot::yRight, plot->canvas()),
          m_plot(plot)
    {
        setRubberBandPen(QColor(Qt::darkGreen));
        setTrackerMode(QwtPlotPicker::AlwaysOn);
    }

protected:
    virtual QwtText trackerTextF(const QPointF &pos) const
    {
        QString value;
        if (m_plot->isSizeModel())
        {
            value = Util::formatByteSize(pos.y());
        }
        else
        {
            value = QString::number((qint64)pos.y());
        }
        QwtText text(QString(" %1 : %2 ").arg(Util::formatTime((qint64)pos.x())).arg(value));
        text.setColor(Qt::white);
        QColor c = rubberBandPen().color();
        text.setBorderPen(QPen(c));
        text.setBorderRadius(6);
        c.setAlpha(170);
        text.setBackgroundBrush(c);
        return text;
    }
private:
    ChartWidgetQwtPlot *m_plot;
};

ChartWidgetQwtPlot::ChartWidgetQwtPlot(QWidget *parent, Options options)
    : QwtPlot(parent), m_model(nullptr), m_isSizeModel(false), m_options(options),
      m_zoomer(new Zoomer(this))
{
    setCanvasBackground(Qt::white);
    enableAxis(QwtPlot::yRight);
    enableAxis(QwtPlot::yLeft, false);

    // TODO!! show help about zooming and panning

    // LeftButton for the zooming
    // Shift+LeftButton: zoom out by 1
    // Ctrl+LeftButton: zoom out to full size
    m_zoomer->setMousePattern(QwtEventPattern::MouseSelect2, Qt::LeftButton, Qt::ControlModifier);
    m_zoomer->setMousePattern(QwtEventPattern::MouseSelect3, Qt::LeftButton, Qt::ShiftModifier);

    // Alt+LeftButton for the panning
    auto panner = new QwtPlotPanner(canvas());
    panner->setMouseButton(Qt::LeftButton, Qt::AltModifier);

    m_vLinePen.setStyle(Qt::DashLine);
    m_vLinePen.setColor(Qt::gray);
}

void ChartWidgetQwtPlot::setModel(ChartModel* model)
{
    m_model = model;
    m_isSizeModel = (model != nullptr) &&
        !(model->type() == ChartModel::Allocations ||
          model->type() == ChartModel::Instances ||
          model->type() == ChartModel::Temporary);
}

ChartWidgetQwtPlot::Options ChartWidgetQwtPlot::setOption(Options option, bool isOn)
{
    setOptions(isOn ? (m_options | option) : Options(m_options & ~option));
    return m_options;
}

void ChartWidgetQwtPlot::setOptions(Options options)
{
    if (m_options != options)
    {
        bool oldShowTotal = hasOption(ShowTotal);
        m_options = options;
        rebuild(oldShowTotal != hasOption(ShowTotal));
    }
}

static QString getCurveTitle(QString label)
{
    const int MaxLineLength = 48;
    const int LastLineExtra = 12; // take into account the label continuation " (max=...)"

    int labelLength = label.size();
    if (labelLength + LastLineExtra <= MaxLineLength)
    {
        return label.toHtmlEscaped();
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
    while (labelLength + LastLineExtra > MaxLineLength);
    result += label.toHtmlEscaped();
    return result;
}

void ChartWidgetQwtPlot::rebuild(bool resetZoomAndPan)
{
    detachItems();

    if (resetZoomAndPan)
    {
        if (!m_xScaleDiv.isEmpty())
        {
            setAxisScaleDiv(QwtPlot::xBottom, m_xScaleDiv);
        }
        if (!m_yScaleDiv.isEmpty())
        {
            setAxisScaleDiv(QwtPlot::yRight, m_yScaleDiv);
        }
        setAxisAutoScale(QwtPlot::xBottom, true);
        setAxisAutoScale(QwtPlot::yRight, true);
    }

    if (!m_model)
    {
        return;
    }

    insertLegend(hasOption(ShowLegend) ? new QwtLegend() : nullptr);

    auto grid = new QwtPlotGrid();
    grid->setPen(QPen(Qt::lightGray));
    grid->attach(this);

    setAxisTitle(QwtPlot::xBottom, m_model->headerData(0).toString());
    setAxisTitle(QwtPlot::yRight, m_model->headerData(1).toString());
    setAxisScaleDraw(QwtPlot::xBottom, new TimeScaleDraw());
    if (m_isSizeModel)
    {
        setAxisScaleDraw(QwtPlot::yRight, new SizeScaleDraw());
    }

    int column = 1;
    if (!hasOption(ShowTotal))
    {
        column += 2;
    }
    int columns = m_model->columnCount();
    for (; column < columns; column += 2)
    {
        auto adapter = new ChartModel2QwtSeriesData(m_model, column);

        QRectF bounds = adapter->boundingRect();
        qint64 maxCost = bounds.bottom();

        QString title = getCurveTitle(m_model->getColumnLabel(column));
        title += QString(" (max=<b>%1</b>)").arg(
            m_isSizeModel ? Util::formatByteSize(maxCost) : QString::number(maxCost));
        auto curve = new QwtPlotCurve(title);
        curve->setRenderHint(QwtPlotItem::RenderAntialiased, true);
        curve->setYAxis(QwtPlot::yRight);

        curve->setPen(m_model->getColumnDataSetPen(column - 1));
        curve->setBrush(m_model->getColumnDataSetBrush(column - 1));

        curve->setSamples(adapter);

        if (hasOption(ShowSymbols))
        {
            QwtSymbol *symbol = new QwtSymbol(QwtSymbol::Ellipse,
                QBrush(Qt::white), QPen(Qt::black, 2), QSize(6, 6));
            curve->setSymbol(symbol);
        }

        curve->attach(this);
    }

    if (hasOption(ShowVLines))
    {
        int rows = m_model->rowCount();
        for (int row = 1; row < rows; ++row)
        {
            auto marker = new QwtPlotMarker();
            marker->setLinePen(m_vLinePen);
            marker->setLineStyle(QwtPlotMarker::VLine);
            marker->setXValue(m_model->getTimestamp(row));
            marker->attach(this);
        }
    }

    replot();

    if (resetZoomAndPan)
    {
        m_zoomer->setZoomBase(false);
        m_xScaleDiv = axisScaleDiv(QwtPlot::xBottom);
        m_yScaleDiv = axisScaleDiv(QwtPlot::yRight);
    }
}

void ChartWidgetQwtPlot::resetZoom()
{
    bool doReplot = false;
    if (m_zoomer->zoomRectIndex() != 0)
    {
        m_zoomer->zoom(0);
        doReplot = true;
    }
    if (!m_xScaleDiv.isEmpty() && (m_xScaleDiv != axisScaleDiv(QwtPlot::xBottom)))
    {
        setAxisScaleDiv(QwtPlot::xBottom, m_xScaleDiv);
        doReplot = true;
    }
    if (!m_yScaleDiv.isEmpty() && (m_yScaleDiv != axisScaleDiv(QwtPlot::yRight)))
    {
        setAxisScaleDiv(QwtPlot::yRight, m_yScaleDiv);
        doReplot = true;
    }
    if (doReplot)
    {
        replot();
    }
}
