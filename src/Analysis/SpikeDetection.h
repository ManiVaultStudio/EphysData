#pragma once

#include <vector>

class TimeSeries;

std::vector<int> DetectSpikes(const TimeSeries& timeSeries);

std::vector<int> DetectSpikesIPFX(const TimeSeries& timeSeries, double filter = 10.0);
