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
 * filter - Cutoff frequency for 4-pole low-pass Bessel filter in kHz
 *          (optional, default 10)
 */
std::vector<int> DetectSpikesIPFX(const TimeSeries& timeSeries, double filter)
{
    const std::vector<float>& v = timeSeries.ySeries;
    const std::vector<float>& t = timeSeries.xSeries;

    if (t.size() != v.size() || t.size() < 2)
        return {};

    const float filter_khz = static_cast<float>(filter);
    const float dv_cutoff = 20.0f;
    const float min_height = 2.0f;
    const float min_peak = -30.0f;

    const std::vector<float> dvdt =
        ipfx::calculate_dvdt(
            v,
            t,
            filter_khz);

    std::vector<ipfx::Index> putative_spikes =
        ipfx::detect_putative_spikes(
            v,
            t,
            std::nullopt,
            std::nullopt,
            filter_khz,
            dv_cutoff,
            &dvdt);

    if (putative_spikes.empty())
        return {};

    std::vector<ipfx::Index> peaks =
        ipfx::find_peak_indexes(
            v,
            t,
            putative_spikes);

    auto filtered =
        ipfx::filter_putative_spikes(
            v,
            t,
            putative_spikes,
            peaks,
            min_height,
            min_peak,
            filter_khz,
            &dvdt);

    putative_spikes = std::move(filtered.first);
    peaks = std::move(filtered.second);

    if (putative_spikes.empty())
        return {};

    // Find maximum dV/dt between each putative threshold and peak.
    std::vector<ipfx::Index> upstrokes =
        ipfx::find_upstroke_indexes(
            v,
            t,
            putative_spikes,
            peaks,
            filter_khz,
            &dvdt);

    if (upstrokes.empty())
        return {};

    // Refine threshold based on a fraction of the average upstroke dV/dt.
    constexpr float thresh_frac = 0.05f;

    std::vector<ipfx::Index> thresholds =
        ipfx::refine_threshold_indexes(
            v,
            t,
            upstrokes,
            thresh_frac,
            filter_khz,
            &dvdt);

    if (thresholds.empty())
        return {};

    // Validate threshold/peak relationships and remove or repair
    // implausible detections.
    constexpr float max_interval = 0.005f; // 5 ms

    ipfx::CheckedSpikes checked =
        ipfx::check_thresholds_and_peaks(
            v,
            t,
            thresholds,
            peaks,
            upstrokes,
            std::nullopt,
            std::nullopt,
            max_interval,
            thresh_frac,
            filter_khz,
            &dvdt);

    thresholds = std::move(checked.spikeIndexes);
    peaks = std::move(checked.peakIndexes);
    upstrokes = std::move(checked.upstrokeIndexes);

    if (thresholds.empty())
        return {};

    std::vector<int> spikes;
    spikes.reserve(thresholds.size());

    for (ipfx::Index index : thresholds)
        spikes.push_back(static_cast<int>(index));

    return spikes;
}
