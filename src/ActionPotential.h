#pragma once

#include "EphysData_export.h"

#include <util/Serializable.h>

#include <vector>

class EPHYSDATA_EXPORT ActionPotential : public mv::util::Serializable
{
public:
    ActionPotential();
    ActionPotential(std::vector<float> timeSeries, std::vector<float> voltageSeries, int peakIndex);

    const std::vector<float>& getTimeSeries() const { return _timeSeries; }
    const std::vector<float>& getVoltageSeries() const { return _voltageSeries; }

    int getPeakIndex() const { return _peakIndex; }

public: // Serialization
    void fromVariantMap(const QVariantMap& variantMap) override;
    QVariantMap toVariantMap() const override;

private:
    std::vector<float> _timeSeries;
    std::vector<float> _voltageSeries;

    int _peakIndex = -1;
};
