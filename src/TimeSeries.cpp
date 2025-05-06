#include "TimeSeries.h"

#include "util/Serialization.h"

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

    // Release memory usage
    xSeries.shrink_to_fit();
    ySeries.shrink_to_fit();
}

void TimeSeries::fromVariantMap(const QVariantMap& variantMap)
{
    mv::util::variantMapMustContain(variantMap, "NumDataPoints");
    mv::util::variantMapMustContain(variantMap, "xSeries");
    mv::util::variantMapMustContain(variantMap, "ySeries");

    int numDataPoints = variantMap["NumDataPoints"].toInt();

    xSeries.resize(numDataPoints);
    ySeries.resize(numDataPoints);

    mv::util::populateDataBufferFromVariantMap(variantMap["xSeries"].toMap(), (char*) xSeries.data());
    mv::util::populateDataBufferFromVariantMap(variantMap["ySeries"].toMap(), (char*) ySeries.data());
}

QVariantMap TimeSeries::toVariantMap() const
{
    QVariantMap variantMap;
    
    if (xSeries.size() != ySeries.size())
        throw std::runtime_error("TimeSeries xSeries size doesn't match ySeries size");

    variantMap["NumDataPoints"] = xSeries.size();
    variantMap["xSeries"] = mv::util::rawDataToVariantMap((const char*)xSeries.data(), xSeries.size() * sizeof(float));
    variantMap["ySeries"] = mv::util::rawDataToVariantMap((const char*)ySeries.data(), ySeries.size() * sizeof(float));

    return variantMap;
}
