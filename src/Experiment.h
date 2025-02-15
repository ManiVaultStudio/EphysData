#pragma once

#include "EphysData_export.h"

#include "Recording.h"

#include <vector>
#include <string>
#include <memory>

class EPHYSDATA_EXPORT Experiment
{
public:
    const std::vector<Recording>& getAcquisitions() { return _acquisitions; }

    void addAcquisition(Recording&& recording);
    void addStimulus(Recording&& recording);

private:
    std::vector<Recording> _acquisitions;
    std::vector<Recording> _stimuli;
};
