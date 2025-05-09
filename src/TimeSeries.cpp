#include "TimeSeries.h"

#include "util/Serialization.h"

void lttb(TimeSeries& input, size_t threshold) {
    size_t dataLength = input.xSeries.size();
    if (threshold >= dataLength || threshold == 0) {
        return;
    }

    std::vector<float> outX, outY;
    outX.push_back(input.xSeries[0]);
    outY.push_back(input.ySeries[0]);

    double bucketSize = static_cast<double>(dataLength - 2) / (threshold - 2);
    size_t a = 0;

    for (size_t i = 0; i < threshold - 2; ++i) {
        size_t start = static_cast<size_t>(std::floor((i + 1) * bucketSize)) + 1;
        size_t end = static_cast<size_t>(std::floor((i + 2) * bucketSize)) + 1;
        if (end >= dataLength) end = dataLength;

        // Calculate average of next bucket
        double avgX = 0.0, avgY = 0.0;
        size_t avgRange = end - start;
        for (size_t j = start; j < end; ++j) {
            avgX += input.xSeries[j];
            avgY += input.ySeries[j];
        }
        avgX /= avgRange;
        avgY /= avgRange;

        // Find point in this bucket with max triangle area
        double maxArea = -1.0;
        size_t maxAreaIndex = start;
        size_t rangeStart = static_cast<size_t>(std::floor(i * bucketSize)) + 1;
        size_t rangeEnd = start;
        if (rangeEnd > dataLength) rangeEnd = dataLength;

        for (size_t j = rangeStart; j < rangeEnd; ++j) {
            double area = std::abs(
                (input.xSeries[a] - avgX) * (input.ySeries[j] - input.ySeries[a]) -
                (input.xSeries[a] - input.xSeries[j]) * (avgY - input.ySeries[a])
            ) * 0.5;
            if (area > maxArea) {
                maxArea = area;
                maxAreaIndex = j;
            }
        }

        outX.push_back(input.xSeries[maxAreaIndex]);
        outY.push_back(input.ySeries[maxAreaIndex]);
        a = maxAreaIndex;
    }

    outX.push_back(input.xSeries.back());
    outY.push_back(input.ySeries.back());

    input.xSeries = outX;
    input.ySeries = outY;
}

void TimeSeries::downsample()
{
    //int sampleInterval = 100;

    //int newSize = xSeries.size() / sampleInterval;

    //for (int i = 0; i < newSize; i++)
    //{
    //    xSeries[i] = xSeries[i * sampleInterval];
    //    ySeries[i] = ySeries[i * sampleInterval];

    //    if (isnan(xSeries[i])) xSeries[i] = 0;
    //    if (isnan(ySeries[i])) ySeries[i] = 0;
    //}
    //xSeries[newSize - 1] = xSeries[xSeries.size() - 1];
    //ySeries[newSize - 1] = ySeries[ySeries.size() - 1];
    //if (isnan(xSeries[newSize - 1])) xSeries[newSize - 1] = 0;
    //if (isnan(ySeries[newSize - 1])) ySeries[newSize - 1] = 0;

    //xSeries.resize(newSize);
    //ySeries.resize(newSize);

    //// Release memory usage
    //xSeries.shrink_to_fit();
    //ySeries.shrink_to_fit();

    lttb(*this, xSeries.size() / 100);

    for (int i = 0; i < xSeries.size(); i++)
    {
        if (isnan(xSeries[i])) xSeries[i] = 0;
        if (isnan(ySeries[i])) ySeries[i] = 0;
    }
}

void TimeSeries::trim()
{
    int size = ySeries.size();
    int newSize = size;

    // Totally last value
    float ref = ySeries[size - 1];
    
    // Find the last value different to the final value, we can trim the rest
    for (int i = size-1; i >= 0; i--)
    {
        if (ySeries[i] != ref)
        {
            newSize = i;
            break;
        }
    }

    // Add a bit of a buffer at the end
    if (newSize + 5 < size)
        newSize = newSize + 5;

    // Resize both series
    xSeries.resize(newSize);
    ySeries.resize(newSize);

    // Release excess memory usage
    xSeries.shrink_to_fit();
    ySeries.shrink_to_fit();
}

void TimeSeries::computeExtents()
{
    xMin = std::numeric_limits<float>::max();
    xMax = -std::numeric_limits<float>::max();
    yMin = std::numeric_limits<float>::max();
    yMax = -std::numeric_limits<float>::max();

    for (int i = 0; i < xSeries.size(); i++)
    {
        if (xSeries[i] < xMin) xMin = xSeries[i];
        if (xSeries[i] > xMax) xMax = xSeries[i];
    }
    for (int i = 0; i < ySeries.size(); i++)
    {
        if (ySeries[i] < yMin) yMin = ySeries[i];
        if (ySeries[i] > yMax) yMax = ySeries[i];
    }
}

void TimeSeries::fromVariantMap(const QVariantMap& variantMap)
{
    mv::util::variantMapMustContain(variantMap, "NumDataPoints");
    mv::util::variantMapMustContain(variantMap, "xSeries");
    mv::util::variantMapMustContain(variantMap, "ySeries");
    mv::util::variantMapMustContain(variantMap, "xMin");
    mv::util::variantMapMustContain(variantMap, "xMax");
    mv::util::variantMapMustContain(variantMap, "yMin");
    mv::util::variantMapMustContain(variantMap, "yMax");

    int numDataPoints = variantMap["NumDataPoints"].toInt();

    xSeries.resize(numDataPoints);
    ySeries.resize(numDataPoints);

    mv::util::populateDataBufferFromVariantMap(variantMap["xSeries"].toMap(), (char*) xSeries.data());
    mv::util::populateDataBufferFromVariantMap(variantMap["ySeries"].toMap(), (char*) ySeries.data());

    xMin = variantMap["xMin"].toFloat();
    xMax = variantMap["xMax"].toFloat();
    yMin = variantMap["yMin"].toFloat();
    yMax = variantMap["yMax"].toFloat();

    qDebug() << "Extents: " << xMin << xMax << yMin << yMax;
}

QVariantMap TimeSeries::toVariantMap() const
{
    QVariantMap variantMap;
    
    if (xSeries.size() != ySeries.size())
        throw std::runtime_error("TimeSeries xSeries size doesn't match ySeries size");

    variantMap["NumDataPoints"] = xSeries.size();
    variantMap["xSeries"] = mv::util::rawDataToVariantMap((const char*)xSeries.data(), xSeries.size() * sizeof(float));
    variantMap["ySeries"] = mv::util::rawDataToVariantMap((const char*)ySeries.data(), ySeries.size() * sizeof(float));
    variantMap["xMin"] = xMin;
    variantMap["xMax"] = xMax;
    variantMap["yMin"] = yMin;
    variantMap["yMax"] = yMax;

    return variantMap;
}
