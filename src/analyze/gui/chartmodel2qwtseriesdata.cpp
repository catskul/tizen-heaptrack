#include "chartmodel2qwtseriesdata.h"

ChartModel2QwtSeriesData::ChartModel2QwtSeriesData(ChartModel *model, int column)
    : m_model(model), m_column(column)
{
    updateBoundingRect();
}

void ChartModel2QwtSeriesData::updateBoundingRect()
{
    int rows = m_model->rowCount();
    if (rows > 0)
    {
        m_boundingRect.setLeft(m_model->data(m_model->index(0, 0)).toDouble());
        m_boundingRect.setTop(m_model->data(m_model->index(0, m_column)).toDouble());
        m_boundingRect.setRight(m_model->data(m_model->index(rows - 1, 0)).toDouble());
        qreal maxCost = -1E9;
        for (int row = 0; row < rows; ++row)
        {
            qreal cost = m_model->data(m_model->index(row, m_column)).toDouble();
            if (cost > maxCost)
            {
                maxCost = cost;
            }
        }
        m_boundingRect.setBottom(maxCost);
    }
}

QPointF ChartModel2QwtSeriesData::sample(size_t i) const
{
    int row = (int)i;
    qreal timeStamp = m_model->data(m_model->index(row, 0)).toDouble();
    qreal cost = m_model->data(m_model->index(row, m_column)).toDouble();
    return QPointF(timeStamp, cost);
}

QRectF ChartModel2QwtSeriesData::boundingRect() const
{
    return m_boundingRect;
}
