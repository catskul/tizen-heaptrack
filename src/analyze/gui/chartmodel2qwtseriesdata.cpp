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
        m_boundingRect.setLeft(m_model->getTimestamp(0));
        m_boundingRect.setTop(m_model->getCost(0, m_column));
        m_boundingRect.setRight(m_model->getTimestamp(rows - 1));
        qint64 maxCost = -1;
        for (int row = 0; row < rows; ++row)
        {
            qint64 cost = m_model->getCost(row, m_column);
            if (cost > maxCost)
            {
                maxCost = cost;
            }
        }
        m_boundingRect.setBottom(maxCost);
    }
    else
    {
        m_boundingRect = {};
    }
}

QPointF ChartModel2QwtSeriesData::sample(size_t i) const
{
    int row = (int)i;
    return QPointF(m_model->getTimestamp(row), m_model->getCost(row, m_column));
}

QRectF ChartModel2QwtSeriesData::boundingRect() const
{
    return m_boundingRect;
}
