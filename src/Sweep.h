#pragma once

#include "EphysData_export.h"

#include "Recording.h"
#include "Stimulus.h"
#include "SweepProperties.h"

class EPHYSDATA_EXPORT Sweep : public mv::util::Serializable
{
public:
    int GetSweepNumber() const;
    void SetSweepNumber(int sweepNumber);

    bool IsLowestSpikingSweep() const { return _properties._isLowestSpikingSweep; }
    void MarkLowestSpikingSweep() { _properties._isLowestSpikingSweep = true; }

    const SweepProperties GetSweepProperties() const { return _properties; }

    void DetectSpikes();

public: // Serialization
    void fromVariantMap(const QVariantMap& variantMap) override;
    QVariantMap toVariantMap() const override;

public:
    Stimulus stimulus;
    Acquisition acquisition;

private:
    int _sweepNumber;
    SweepProperties _properties;
};
