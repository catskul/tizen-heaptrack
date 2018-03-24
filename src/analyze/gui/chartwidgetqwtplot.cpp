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

#include <QToolTip>

#include <limits>

ChartOptions::Options ChartOptions::GlobalOptions(
    ChartOptions::ShowHelp |
    ChartOptions::ShowTotal | ChartOptions::ShowUnresolved |
    ChartOptions::ShowLegend | ChartOptions::ShowCurveBorders);

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
//        setRubberBandPen(QColor(Qt::darkGreen));
        setRubberBandPen(QColor(0, 0x60, 0));
        setTrackerMode(QwtPlotPicker::AlwaysOn);
    }

protected:
    virtual QwtText trackerTextF(const QPointF &pos) const
    {
//        qDebug() << "Zoomer: (" << pos.x() << "; " << pos.y() << ")";
        if ((pos.x() < 0) || (pos.y() < 0))
        {
            return {};
        }
        QwtText text;
        text.setRenderFlags(text.renderFlags() & ~Qt::AlignHorizontal_Mask | Qt::AlignLeft);
        text.setBorderRadius(6);
        QString tooltip;
        if (m_plot->getCurveTooltip(pos, tooltip))
        {
            text.setText("<p style='margin-left:4px'>" + tooltip + "</p> ");
            text.setColor(Qt::white);
            QPen bandPen = rubberBandPen();
            text.setBorderPen(bandPen);
            QColor c = bandPen.color();
            c.setAlpha(170);
            text.setBackgroundBrush(c);
            m_plot->clearTooltip();
        }
        else // show default text
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
            text.setText(QString("<p style='margin-left:4px'><b>%1 : %2</b></p> ")
                         .arg(Util::formatTime((qint64)pos.x())).arg(value));
            text.setColor(Qt::yellow);
            static QPen bluePen(QColor(0, 0, 0xA0));
            text.setBorderPen(bluePen);
            QColor c = bluePen.color();
            c.setAlpha(190);
            text.setBackgroundBrush(c);
            m_plot->restoreTooltip();
        }
        return text;
    }
private:
    ChartWidgetQwtPlot *m_plot;
};

ChartOptions::Options ChartOptions::setOption(Options options, Options option, bool isOn)
{
    return (isOn ? (options | option) : Options(options & ~option));
}

ChartOptions::Options ChartOptions::setOption(Options option, bool isOn)
{
    setOptions(ChartOptions::setOption(m_options, option, isOn));
    return m_options;
}

ChartOptions::Options ChartOptions::toggleOption(Options option)
{
    return setOption(option, !hasOption(option));
}

ChartWidgetQwtPlot::ChartWidgetQwtPlot(QWidget *parent, Options options)
    : QwtPlot(parent), m_model(nullptr), m_isSizeModel(false), ChartOptions(options),
      m_zoomer(new Zoomer(this))
{
    setCanvasBackground(Qt::white);
    enableAxis(QwtPlot::yRight);
    enableAxis(QwtPlot::yLeft, false);

    // LeftButton for zooming
    // Shift+LeftButton: zoom out by 1
    // Ctrl+LeftButton: zoom out to full size
    m_zoomer->setMousePattern(QwtEventPattern::MouseSelect2, Qt::LeftButton, Qt::ControlModifier);
    m_zoomer->setMousePattern(QwtEventPattern::MouseSelect3, Qt::LeftButton, Qt::ShiftModifier);

    // Alt+LeftButton for panning
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

void ChartWidgetQwtPlot::setOptions(Options options)
{
    if (m_options != options)
    {
        bool oldShowTotal = hasOption(ShowTotal);
        bool oldShowUnresolved = hasOption(ShowUnresolved);
        m_options = options;
        rebuild(oldShowTotal != hasOption(ShowTotal) || (oldShowUnresolved != hasOption(ShowUnresolved)));
    }
}

void ChartWidgetQwtPlot::rebuild(bool resetZoomAndPan)
{
    const int MaxTitleLineLength = 48;

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
        QString columnLabel = m_model->getColumnLabel(column);
        if (!hasOption(ShowUnresolved) &&
            Util::isUnresolvedFunction(columnLabel)) // column label starts with a function name
        {
            continue;
        }

        auto adapter = new ChartModel2QwtSeriesData(m_model, column);

        QRectF bounds = adapter->boundingRect();
        qint64 maxCost = bounds.bottom();

        QString titleEnd = QString(" (max=<b>%1</b>)").arg(m_isSizeModel
            ? Util::formatByteSize(maxCost) : QString::number(maxCost));
        int lastLineExtra = titleEnd.length() - 2 * 3; // minus the two "<b>" tags length
        auto curve = new QwtPlotCurve(
            Util::wrapLabel(columnLabel, MaxTitleLineLength, lastLineExtra) +
            titleEnd);
        curve->setRenderHint(QwtPlotItem::RenderAntialiased, true);
        curve->setYAxis(QwtPlot::yRight);

        static QPen blackPen;

        curve->setPen(hasOption(ShowCurveBorders)
                      ? blackPen : m_model->getColumnDataSetPen(column - 1));

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

bool ChartWidgetQwtPlot::getCurveTooltip(const QPointF &position, QString &tooltip) const
{
    qreal timestamp = position.x();
    qreal cost = position.y();
    if ((timestamp < 0) || (cost < 0))
    {
        return false;
    }
    int row = m_model->getRowForTimestamp(timestamp);
    if (row < 0)
    {
        return false;
    }
    qint64 rowTimestamp = m_model->getTimestamp(row); // rowTimestamp <= timestamp
    // find a column which value (cost) for the row found is greater than or equal to
    // 'cost' and at the same time this value is the smallest from all such values
    qreal minCostFound = std::numeric_limits<qreal>::max();
    int columnFound = -1;
    int column = 1;
    if (!hasOption(ShowTotal))
    {
        column += 2;
    }
    int columns = m_model->columnCount();
    int rowCount = m_model->rowCount();
    for (; column < columns; column += 2)
    {
        if (!hasOption(ShowUnresolved) &&
            Util::isUnresolvedFunction(m_model->getColumnLabel(column))) // column label starts with a function name
        {
            continue;
        }
        qint64 columnCost = m_model->getCost(row, column);
        qreal intermediateCost = columnCost;
        if ((rowTimestamp < timestamp) && (row + 1 < rowCount))
        {
            // solve linear equation to find line-approximated 'intermediateCost'
            qreal x1, y1, x2, y2;
            x1 = rowTimestamp;
            y1 = columnCost;
            x2 = m_model->getTimestamp(row + 1);
            y2 = m_model->getCost(row + 1, column);
            qreal dx = x2 - x1;
            if (dx > 1e-9) // avoid division by zero or near-zero
            {
                qreal a = (y2 - y1) / dx;
                qreal b = (x2 * y1 - x1 * y2) / dx;
                intermediateCost = a * timestamp + b;
            }
        }
        if ((cost <= intermediateCost) && (intermediateCost <= minCostFound))
        {
            minCostFound = intermediateCost;
            columnFound = column;
        }
    }
    if (columnFound >= 0)
    {
//qDebug() << "timestamp: " << timestamp << " ms; row timestamp: " << m_model->getTimestamp(row) << " ms; cost="
//         << cost << "; row=" << row << "; minCostFound=" << minCostFound;
        tooltip = m_model->data(m_model->index(row, columnFound), Qt::ToolTipRole).toString();
        return true;
    }
    return false;
}

void ChartWidgetQwtPlot::clearTooltip()
{
    if (m_plotTooltip.isEmpty())
    {
        m_plotTooltip = parentWidget()->toolTip();
    }
    parentWidget()->setToolTip(QString());
    QToolTip::hideText();
}

void ChartWidgetQwtPlot::restoreTooltip()
{
    if (!m_plotTooltip.isEmpty())
    {
        parentWidget()->setToolTip(m_plotTooltip);
    }
}
