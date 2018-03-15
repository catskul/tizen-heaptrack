#ifndef CHARTWIDGETQWTPLOT_H
#define CHARTWIDGETQWTPLOT_H

#include <qwt_plot.h>
#include <qwt_scale_div.h>

#include <QPen>

class ChartModel;
class Zoomer;

class ChartWidgetQwtPlot : public QwtPlot
{
public:
    enum Options
    {
        None = 0,
        ShowTotal = 0x01,
        ShowLegend = 0x10,
        ShowSymbols = 0x20,
        ShowVLines = 0x40
    };

    explicit ChartWidgetQwtPlot(QWidget *parent, Options options);

    void setModel(ChartModel* model);

    bool isSizeModel() const { return m_isSizeModel; }

    Options options() const { return m_options; }

    bool hasOption(Options option) const { return (m_options & option) != 0; }

    Options setOption(Options option, bool isOn);

    void setOptions(Options options);

    void rebuild(bool resetZoomAndPan);

    void resetZoom();

    bool getCurveTooltip(const QPointF &position, QString &tooltip) const;

private:
    friend class Zoomer;

    void clearTooltip();

    void restoreTooltip();

    ChartModel *m_model;

    bool m_isSizeModel;

    Options m_options;

    QPen m_vLinePen;

    Zoomer *m_zoomer;

    QwtScaleDiv m_xScaleDiv, m_yScaleDiv;

    QString m_plotTooltip;
};

inline ChartWidgetQwtPlot::Options operator | (ChartWidgetQwtPlot::Options i, ChartWidgetQwtPlot::Options f)
{ return ChartWidgetQwtPlot::Options(int(i) | int(f)); }

inline ChartWidgetQwtPlot::Options operator & (ChartWidgetQwtPlot::Options i, ChartWidgetQwtPlot::Options f)
{ return ChartWidgetQwtPlot::Options(int(i) & int(f)); }

inline ChartWidgetQwtPlot::Options& operator |= (ChartWidgetQwtPlot::Options &i, ChartWidgetQwtPlot::Options f)
{ return (i = (ChartWidgetQwtPlot::Options(int(i) | int(f)))); }

inline ChartWidgetQwtPlot::Options& operator &= (ChartWidgetQwtPlot::Options &i, ChartWidgetQwtPlot::Options f)
{ return (i = (ChartWidgetQwtPlot::Options(int(i) & int(f)))); }

#endif // CHARTWIDGETQWTPLOT_H
