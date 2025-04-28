#include "TimeSeries.h"

void TimeSeries::downsample()
{
    int sampleInterval = 100;
    //// Find vertical range of timeseries
    //auto minmax = std::minmax_element(data.begin(), data.end());
    //float minVal = *minmax.first;
    //float maxVal = *minmax.second;
    //float range = maxVal - minVal;
    //float rangeThreshold = range * 0.05f;

    int newSize = xSeries.size() / sampleInterval;

    for (int i = 0; i < newSize; i++)
    {
        xSeries[i] = xSeries[i * sampleInterval];
        ySeries[i] = ySeries[i * sampleInterval];
    }
    xSeries[newSize - 1] = xSeries[xSeries.size() - 1];
    ySeries[newSize - 1] = ySeries[ySeries.size() - 1];

    xSeries.resize(newSize);
    ySeries.resize(newSize);
}
