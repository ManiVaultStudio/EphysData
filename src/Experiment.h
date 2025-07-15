#pragma once

#include "EphysData_export.h"

#include "Recording.h"

#include <util/Serializable.h>

#include <vector>
#include <string>
#include <memory>

class EPHYSDATA_EXPORT Experiment : public mv::util::Serializable
{
public:
    const std::vector<Recording>& getAcquisitions() const { return _acquisitions; }
    const std::vector<Recording>& getStimuli() const { return _stimuli; }

    void addAcquisition(Recording&& recording);
    void addStimulus(Recording&& recording);

public: // Serialization
    void fromVariantMap(const QVariantMap& variantMap) override;
    QVariantMap toVariantMap() const override;

private:
    std::vector<Recording> _acquisitions;
    std::vector<Recording> _stimuli;
};
