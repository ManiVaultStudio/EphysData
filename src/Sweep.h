#pragma once

#include "EphysData_export.h"

#include "Recording.h"

class EPHYSDATA_EXPORT Sweep : public mv::util::Serializable
{
public:
    int GetSweepNumber() const;
    void SetSweepNumber(int sweepNumber);

public: // Serialization
    void fromVariantMap(const QVariantMap& variantMap) override;
    QVariantMap toVariantMap() const override;

public:
    Stimulus stimulus;
    Recording acquisition;

    int _sweepNumber;
};
