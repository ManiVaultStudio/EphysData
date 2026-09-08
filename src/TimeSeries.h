#pragma once

#include "EphysData_export.h"

#include <util/Serializable.h>

#include <vector>

class EPHYSDATA_EXPORT TimeSeries : public mv::util::Serializable
{
public:
    void Downsample();
    void Trim();
    void Trim(int start, int end);
    void Trim(int start, int end, float paddingSeconds);
    void ComputeExtents();
    //std::pair<int, int> findStimulusRange();
    std::pair<int, int> FindStimulusRange();

public: // Serialization
    void fromVariantMap(const QVariantMap& variantMap) override;
    QVariantMap toVariantMap() const override;

public:
    std::vector<float> xSeries;
    std::vector<float> ySeries;

    float xMin = 0;
    float xMax = 0;
    float yMin = 0;
    float yMax = 0;

    int samplingRate = -1;
};
