#ifndef HISTOGRAMWIDGETQWTPLOT_H
#define HISTOGRAMWIDGETQWTPLOT_H

#include <qwt_plot.h>

class HistogramModel;
class Picker;

class HistogramWidgetQwtPlot : public QwtPlot
{
public:
    explicit HistogramWidgetQwtPlot(QWidget *parent);

    void setModel(HistogramModel *model) { m_model = model; }

    HistogramModel *model() const { return m_model; }

    void rebuild();

private:
    HistogramModel *m_model;

    Picker *m_picker;
};

#endif // HISTOGRAMWIDGETQWTPLOT_H
