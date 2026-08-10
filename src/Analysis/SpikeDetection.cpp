#include "SpikeDetection.h"

#include "TimeSeries.h"

#include "IPFX.h"

#include <iostream>

std::vector<int> DetectSpikes(const TimeSeries& timeSeries)
{
    int windowSize = 10;
    float threshold = 3.0f;

    const std::vector<float>& y = timeSeries.ySeries;
    std::vector<int> spikes;

    if (y.size() < windowSize * 2)
        return spikes;

    for (size_t i = windowSize; i < y.size() - windowSize; ++i)
    {
        float mean = 0.0f;
        float var = 0.0f;

        // Compute mean
        for (int j = -windowSize; j <= windowSize; ++j)
            mean += y[i + j];

        mean /= (2 * windowSize + 1);

        // Compute variance
        for (int j = -windowSize; j <= windowSize; ++j)
        {
            float diff = y[i + j] - mean;
            var += diff * diff;
        }

        float stddev = std::sqrt(var / (2 * windowSize + 1));

        if ((y[i] - mean) > threshold * stddev)
            spikes.push_back(i);
    }

    return spikes;
}

/**
 * 
 * filter - Cutoff frequency for 4-pole low-pass Bessel filter in kHz (optional, default 10)
 */
std::vector<int> DetectSpikesIPFX(const TimeSeries& timeSeries, double filter)
{
    const std::vector<float>& v = timeSeries.ySeries;
    const std::vector<float>& t = timeSeries.xSeries;

    if (t.size() != v.size() || t.size() < 2) {
        return std::vector<int>();
    }

    const float filter_khz = 10.0f;
    const float dv_cutoff = 20.0f;
    const float min_height = 2.0f;
    const float min_peak = -30.0f;

    const std::vector<float> dvdt = ipfx::calculate_dvdt(v, t, filter_khz);

    std::vector<ipfx::Index> putative_spikes = ipfx::detect_putative_spikes(v, t, std::nullopt, std::nullopt, filter_khz, dv_cutoff, &dvdt);

    std::vector<ipfx::Index> peaks = ipfx::find_peak_indexes(v, t, putative_spikes);

    auto filtered = ipfx::filter_putative_spikes(v, t, putative_spikes, peaks, min_height, min_peak, filter_khz, &dvdt);

    putative_spikes = std::move(filtered.first);
    peaks = std::move(filtered.second);

    if (putative_spikes.empty()) {
        std::cout << "No spikes detected\n";
        return std::vector<int>();
    }

    for (std::size_t i = 0; i < putative_spikes.size(); ++i)
    {
        std::cout
            << "Spike index: " << putative_spikes[i]
            << ", peak index: " << peaks[i]
            << '\n';
    }

    std::vector<int> spikes;
    for (int i = 0; i < putative_spikes.size(); i++)
        spikes.push_back(putative_spikes[i]);

    return spikes;
}
