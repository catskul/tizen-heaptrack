#ifndef HISTOGRAMWIDGETQWTPLOT_H
#define HISTOGRAMWIDGETQWTPLOT_H

#include <qwt_plot.h>

class HistogramModel;

class HistogramWidgetQwtPlot : public QwtPlot
{
public:
    explicit HistogramWidgetQwtPlot(QWidget *parent);

    void setModel(HistogramModel *model) { m_model = model; }

    void rebuild(bool resetZoomAndPan);

private:
    HistogramModel *m_model;
};

#endif // HISTOGRAMWIDGETQWTPLOT_H
