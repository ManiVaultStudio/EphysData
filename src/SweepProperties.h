#pragma once

#include "EphysData_export.h"

#include "util/Serializable.h"

#include <vector>

class SweepProperties : public mv::util::Serializable
{
public:
    int GetSpikeCount() const { return spikeIndices.size(); }

    std::vector<int> spikeIndices;

private:
    bool _isLowestSpikingSweep = false;

public: // Serialization
    void fromVariantMap(const QVariantMap& variantMap) override;
    QVariantMap toVariantMap() const override;

    friend class Sweep;
};
