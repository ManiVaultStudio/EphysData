#pragma once

#include <cstddef>
#include <optional>
#include <utility>
#include <vector>

namespace ipfx
{

    using Index = std::size_t;

    struct CheckedSpikes
    {
        std::vector<Index> spikeIndexes;
        std::vector<Index> peakIndexes;
        std::vector<Index> upstrokeIndexes;
        std::vector<bool> clipped;
    };

    bool has_fixed_dt(const std::vector<float>& t);

    std::vector<float> calculate_dvdt(
        const std::vector<float>& v,
        const std::vector<float>& t,
        std::optional<float> filter_khz = 10.0f);

    Index find_time_index(
        const std::vector<float>& t,
        float t0);

    std::vector<Index> detect_putative_spikes(
        const std::vector<float>& v,
        const std::vector<float>& t,
        std::optional<float> start = std::nullopt,
        std::optional<float> end = std::nullopt,
        std::optional<float> filter_khz = 10.0f,
        float dv_cutoff = 20.0f,
        const std::vector<float>* dvdt = nullptr);

    std::vector<Index> find_peak_indexes(
        const std::vector<float>& v,
        const std::vector<float>& t,
        const std::vector<Index>& spike_indexes,
        std::optional<float> end = std::nullopt);

    std::pair<std::vector<Index>, std::vector<Index>> filter_putative_spikes(
        const std::vector<float>& v,
        const std::vector<float>& t,
        const std::vector<Index>& spike_indexes,
        const std::vector<Index>& peak_indexes,
        float min_height = 2.0f,
        float min_peak = -30.0f,
        std::optional<float> filter_khz = 10.0f,
        const std::vector<float>* dvdt = nullptr);

    std::vector<Index> find_upstroke_indexes(
        const std::vector<float>& v,
        const std::vector<float>& t,
        const std::vector<Index>& spike_indexes,
        const std::vector<Index>& peak_indexes,
        std::optional<float> filter_khz = 10.0f,
        const std::vector<float>* dvdt = nullptr);

    std::vector<Index> refine_threshold_indexes(
        const std::vector<float>& v,
        const std::vector<float>& t,
        const std::vector<Index>& upstroke_indexes,
        float thresh_frac = 0.05f,
        std::optional<float> filter_khz = 10.0f,
        const std::vector<float>* dvdt = nullptr);

    std::vector<bool> find_clipped_spikes(
        const std::vector<float>& v,
        const std::vector<float>& t,
        const std::vector<Index>& spike_indexes,
        const std::vector<Index>& peak_indexes,
        Index end_index,
        float tol = 1.0f);

    CheckedSpikes check_thresholds_and_peaks(
        const std::vector<float>& v,
        const std::vector<float>& t,
        const std::vector<Index>& spike_indexes,
        const std::vector<Index>& peak_indexes,
        const std::vector<Index>& upstroke_indexes,
        std::optional<float> start = std::nullopt,
        std::optional<float> end = std::nullopt,
        float max_interval = 0.005f,
        float thresh_frac = 0.05f,
        std::optional<float> filter_khz = 10.0f,
        const std::vector<float>* dvdt = nullptr,
        float tol = 1.0f,
        float reject_at_stim_start_interval = 0.0f);

} // namespace ipfx
