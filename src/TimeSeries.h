#pragma once

#include "EphysData_export.h"

#include <util/Serializable.h>

#include <vector>

class EPHYSDATA_EXPORT TimeSeries : public mv::util::Serializable
{
public:
    void downsample();

public: // Serialization
    void fromVariantMap(const QVariantMap& variantMap) override;
    QVariantMap toVariantMap() const override;

public:
    std::vector<float> xSeries;
    std::vector<float> ySeries;
};
