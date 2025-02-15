#pragma once

#include "EphysData_export.h"

#include "TimeSeries.h"

#include <vector>
#include <string>
#include <memory>

class EPHYSDATA_EXPORT Recording
{
public:
    std::vector<std::string>    comments;
    TimeSeries                  data;
};
