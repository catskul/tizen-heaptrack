#ifndef HISTOGRAMWIDGETQWTPLOT_H
#define HISTOGRAMWIDGETQWTPLOT_H

#include "chartwidgetqwtplot.h"

#include <qwt_plot.h>

#include <QString>

class HistogramModel;
class Picker;

class HistogramWidgetQwtPlot : public QwtPlot, public ChartOptions
{
public:
    explicit HistogramWidgetQwtPlot(QWidget *parent, Options options);

    void setModel(HistogramModel *model) { m_model = model; }

    HistogramModel *model() const { return m_model; }

    virtual void setOptions(Options options) override;

    void rebuild();

private:
    friend class Picker;

    QString getBarText(bool isTotal, int sampleIndex, int barIndex) const;

    HistogramModel *m_model;

    Picker *m_picker;
};

#endif // HISTOGRAMWIDGETQWTPLOT_H
