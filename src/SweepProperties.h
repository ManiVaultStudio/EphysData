#pragma once

#include "EphysData_export.h"

#include "util/Serializable.h"

#include <vector>

class SweepProperties : public mv::util::Serializable
{
public:
    std::vector<int> spikeIndices;

public: // Serialization
    void fromVariantMap(const QVariantMap& variantMap) override;
    QVariantMap toVariantMap() const override;
};
