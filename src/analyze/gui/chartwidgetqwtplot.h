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
        ShowHelp = 0x01,
        ShowTotal = 0x02,
        ShowUnresolved = 0x04,
        ShowLegend = 0x10,
        ShowCurveBorders = 0x20,
        ShowSymbols = 0x40,
        ShowVLines = 0x80
    };

    explicit ChartWidgetQwtPlot(QWidget *parent, Options options);

    void setModel(ChartModel* model);

    ChartModel* model() const { return m_model; }

    bool isSizeModel() const { return m_isSizeModel; }

    Options options() const { return m_options; }

    static bool hasOption(Options options, Options option) { return (options & option) != 0; }

    bool hasOption(Options option) const { return hasOption(m_options, option); }

    static Options setOption(Options options, Options option, bool isOn);

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
