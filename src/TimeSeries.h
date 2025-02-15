#pragma once

#include "EphysData_export.h"

#include <vector>

class EPHYSDATA_EXPORT TimeSeries
{
public:
    void downsample();

public:
    std::vector<float> xSeries;
    std::vector<float> ySeries;
};
