#pragma once

#include "EphysData_export.h"

#include <util/Serializable.h>

#include <vector>

class EPHYSDATA_EXPORT TimeSeries : public mv::util::Serializable
{
public:
    void downsample();
    void trim();
    void trim(int start, int end);
    void computeExtents();
    std::pair<int, int> findStimulusRange();

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
};
