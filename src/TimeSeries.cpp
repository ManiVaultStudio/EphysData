#include "TimeSeries.h"

void TimeSeries::downsample()
{
    //// Find vertical range of timeseries
    //auto minmax = std::minmax_element(data.begin(), data.end());
    //float minVal = *minmax.first;
    //float maxVal = *minmax.second;
    //float range = maxVal - minVal;
    //float rangeThreshold = range * 0.05f;

    int newSize = xSeries.size() / 100;

    for (int i = 0; i < newSize; i++)
    {
        xSeries[i] = xSeries[i * 100];
        ySeries[i] = ySeries[i * 100];
    }
    xSeries[newSize] = xSeries[xSeries.size() - 1];
    ySeries[newSize] = ySeries[ySeries.size() - 1];

    xSeries.resize(newSize + 1);
    ySeries.resize(newSize + 1);
}
