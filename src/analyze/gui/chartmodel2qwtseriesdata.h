#ifndef CHARTMODEL2QWTSERIESDATA_H
#define CHARTMODEL2QWTSERIESDATA_H

#include "chartmodel.h"
#include "qwt_series_data.h"

class ChartModel2QwtSeriesData : public QwtSeriesData<QPointF>
{
public:
    ChartModel2QwtSeriesData(ChartModel *model, int column);

    size_t size() const override
    {
        return m_model->rowCount();
    }

    QPointF sample(size_t i) const override;

    virtual QRectF boundingRect() const override;

private:
    void updateBoundingRect();

    ChartModel *m_model;
    int m_column;
    QRectF m_boundingRect;
};

#endif // CHARTMODEL2QWTSERIESDATA_H
