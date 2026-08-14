#include "IPFX.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <complex>
#include <limits>
#include <stdexcept>

namespace ipfx
{

    namespace
    {

        using FilterCoeffs = std::array<double, 5>;

        bool allclose(
            double a,
            double b,
            double rtol = 1e-5,
            double atol = 1e-8)
        {
            return std::abs(a - b) <= atol + rtol * std::abs(b);
        }

        // Equivalent to:
        //     scipy.signal.bessel(4, Wn, "low")
        //
        // Wn is normalized to Nyquist:
        //     0 < Wn < 1
        std::pair<FilterCoeffs, FilterCoeffs>
            bessel4_lowpass(double Wn)
        {
            if (!(Wn > 0.0 && Wn < 1.0)) {
                throw std::invalid_argument(
                    "Bessel normalized cutoff must satisfy 0 < Wn < 1");
            }

            using Complex = std::complex<double>;

            constexpr double pi =
                3.141592653589793238462643383279502884;

            const std::array<Complex, 4> prototype = { {
                Complex(-0.65721117167188295458,  0.83016143500487337724),
                Complex(-0.90475879678824494596,  0.27091873300387466367),
                Complex(-0.90475879678824494596, -0.27091873300387466367),
                Complex(-0.65721117167188295458, -0.83016143500487337724)
            } };

            const double warped =
                4.0 * std::tan(pi * Wn / 2.0);

            std::array<Complex, 4> poles{};

            for (std::size_t i = 0; i < poles.size(); ++i) {
                const Complex s = prototype[i] * warped;
                poles[i] = (4.0 + s) / (4.0 - s);
            }

            std::array<Complex, 5> poly = { {
                Complex(1.0, 0.0),
                Complex(0.0, 0.0),
                Complex(0.0, 0.0),
                Complex(0.0, 0.0),
                Complex(0.0, 0.0)
            } };

            std::size_t degree = 0;

            for (const Complex& p : poles) {
                ++degree;

                for (std::size_t j = degree; j > 0; --j) {
                    poly[j] -= p * poly[j - 1];
                }
            }

            FilterCoeffs a{};

            for (std::size_t i = 0; i < a.size(); ++i) {
                a[i] = poly[i].real();
            }

            const double denominator_at_dc =
                a[0] + a[1] + a[2] + a[3] + a[4];

            const double gain =
                denominator_at_dc / 16.0;

            FilterCoeffs b = { {
                gain,
                4.0 * gain,
                6.0 * gain,
                4.0 * gain,
                gain
            } };

            return { b, a };
        }

        // Steady-state initial conditions equivalent to scipy.signal.lfilter_zi
        // for this fixed 4th-order filter.
        std::array<double, 4>
            lfilter_zi(
                const FilterCoeffs& b,
                const FilterCoeffs& a)
        {
            std::array<double, 4> B{};

            for (std::size_t i = 0; i < B.size(); ++i) {
                B[i] = b[i + 1] - a[i + 1] * b[0];
            }

            std::array<double, 4> zi{};

            const double asum =
                a[0] + a[1] + a[2] + a[3] + a[4];

            const double bsum =
                B[0] + B[1] + B[2] + B[3];

            zi[0] = bsum / asum;

            double cumulative_a = 1.0;
            double cumulative_b = 0.0;

            for (std::size_t k = 1; k < zi.size(); ++k) {
                cumulative_a += a[k];
                cumulative_b += b[k] - a[k] * b[0];

                zi[k] =
                    cumulative_a * zi[0] -
                    cumulative_b;
            }

            return zi;
        }

        // Direct Form II transposed IIR filter.
        std::vector<double>
            lfilter(
                const FilterCoeffs& b,
                const FilterCoeffs& a,
                const std::vector<double>& x,
                std::array<double, 4> state)
        {
            if (a[0] == 0.0) {
                throw std::invalid_argument(
                    "a[0] cannot be zero");
            }

            FilterCoeffs bn = b;
            FilterCoeffs an = a;

            if (an[0] != 1.0) {
                const double a0 = an[0];

                for (double& value : bn) {
                    value /= a0;
                }

                for (double& value : an) {
                    value /= a0;
                }
            }

            std::vector<double> y(x.size());

            for (std::size_t n = 0; n < x.size(); ++n) {
                const double xn = x[n];

                const double yn =
                    bn[0] * xn +
                    state[0];

                state[0] =
                    bn[1] * xn -
                    an[1] * yn +
                    state[1];

                state[1] =
                    bn[2] * xn -
                    an[2] * yn +
                    state[2];

                state[2] =
                    bn[3] * xn -
                    an[3] * yn +
                    state[3];

                state[3] =
                    bn[4] * xn -
                    an[4] * yn;

                y[n] = yn;
            }

            return y;
        }

        // Odd extension used by scipy.signal.filtfilt.
        std::vector<double>
            odd_extension(
                const std::vector<double>& x,
                std::size_t edge)
        {
            if (x.size() <= edge) {
                throw std::invalid_argument(
                    "Input vector is too short for filtfilt padding");
            }

            std::vector<double> ext;
            ext.reserve(x.size() + 2 * edge);

            for (std::size_t i = edge; i > 0; --i) {
                ext.push_back(
                    2.0 * x.front() - x[i]);
            }

            ext.insert(
                ext.end(),
                x.begin(),
                x.end());

            for (std::size_t i = 1; i <= edge; ++i) {
                ext.push_back(
                    2.0 * x.back() -
                    x[x.size() - 1 - i]);
            }

            return ext;
        }

        // Equivalent to default scipy.signal.filtfilt(b, a, x)
        // for this 4th-order filter.
        std::vector<double>
            filtfilt(
                const FilterCoeffs& b,
                const FilterCoeffs& a,
                const std::vector<double>& x)
        {
            constexpr std::size_t ntaps = 5;
            constexpr std::size_t edge = 3 * ntaps;

            if (x.size() <= edge) {
                throw std::invalid_argument(
                    "Input must contain more than 15 samples for filtfilt");
            }

            std::vector<double> ext =
                odd_extension(x, edge);

            const std::array<double, 4> zi =
                lfilter_zi(b, a);

            std::array<double, 4> forward_state = zi;

            for (double& value : forward_state) {
                value *= ext.front();
            }

            std::vector<double> y =
                lfilter(
                    b,
                    a,
                    ext,
                    forward_state);

            std::reverse(
                y.begin(),
                y.end());

            std::array<double, 4> backward_state = zi;

            for (double& value : backward_state) {
                value *= y.front();
            }

            y = lfilter(
                b,
                a,
                y,
                backward_state);

            std::reverse(
                y.begin(),
                y.end());

            return std::vector<double>(
                y.begin() + static_cast<std::ptrdiff_t>(edge),
                y.end() - static_cast<std::ptrdiff_t>(edge));
        }

    } // namespace

    bool has_fixed_dt(const std::vector<float>& t)
    {
        if (t.size() < 2)
            return false;

        const double dt0 =
            static_cast<double>(t[1]) -
            static_cast<double>(t[0]);

        for (std::size_t i = 2; i < t.size(); ++i) {
            const double dt =
                static_cast<double>(t[i]) -
                static_cast<double>(t[i - 1]);

            if (!allclose(dt, dt0))
                return false;
        }

        return true;
    }

    std::vector<float>
        calculate_dvdt(
            const std::vector<float>& v,
            const std::vector<float>& t,
            std::optional<float> filter_khz)
    {
        if (v.size() != t.size()) {
            throw std::invalid_argument(
                "v and t must have the same length");
        }

        if (v.size() < 2) {
            return {};
        }

        std::vector<double> voltage(
            v.begin(),
            v.end());

        if (has_fixed_dt(t) &&
            filter_khz.has_value() &&
            *filter_khz != 0.0f)
        {
            const double delta_t =
                static_cast<double>(t[1]) -
                static_cast<double>(t[0]);

            const double sample_freq =
                1.0 / delta_t;

            const double cutoff_hz =
                static_cast<double>(*filter_khz) * 1e3;

            const double filt_coeff =
                cutoff_hz /
                (sample_freq / 2.0);

            if (filt_coeff < 0.0 ||
                filt_coeff >= 1.0)
            {
                throw std::invalid_argument(
                    "Bessel coefficient is outside valid range [0, 1)");
            }

            const auto coeffs =
                bessel4_lowpass(filt_coeff);

            voltage =
                filtfilt(
                    coeffs.first,
                    coeffs.second,
                    voltage);
        }

        std::vector<float> dvdt;
        dvdt.reserve(v.size() - 1);

        const double epsilon =
            std::numeric_limits<double>::epsilon();

        for (std::size_t i = 0; i + 1 < v.size(); ++i) {
            const double dv =
                voltage[i + 1] -
                voltage[i];

            const double dt =
                static_cast<double>(t[i + 1]) -
                static_cast<double>(t[i]);

            if (std::abs(dt) > epsilon) {
                const double derivative =
                    1e-3 * dv / dt;

                dvdt.push_back(
                    static_cast<float>(derivative));
            }
        }

        return dvdt;
    }

    Index find_time_index(
        const std::vector<float>& t,
        float t0)
    {
        for (Index i = 0; i < t.size(); ++i) {
            if (t[i] >= t0) {
                return i;
            }
        }

        throw std::runtime_error(
            "Could not find given time in time vector");
    }

    std::vector<Index>
        detect_putative_spikes(
            const std::vector<float>& v,
            const std::vector<float>& t,
            std::optional<float> start,
            std::optional<float> end,
            std::optional<float> filter_khz,
            float dv_cutoff,
            const std::vector<float>* dvdt)
    {
        if (v.size() != t.size()) {
            throw std::invalid_argument(
                "Voltage and time series do not have the same dimensions");
        }

        if (v.empty()) {
            return {};
        }

        const float start_time =
            start.has_value() ? *start : t.front();

        const float end_time =
            end.has_value() ? *end : t.back();

        const Index start_index =
            find_time_index(t, start_time);

        const Index end_index =
            find_time_index(t, end_time);

        if (end_index < start_index) {
            throw std::invalid_argument(
                "End time occurs before start time");
        }

        std::vector<float> local_dvdt;

        if (dvdt == nullptr) {
            std::vector<float> v_window(
                v.begin() + static_cast<std::ptrdiff_t>(start_index),
                v.begin() + static_cast<std::ptrdiff_t>(end_index + 1));

            std::vector<float> t_window(
                t.begin() + static_cast<std::ptrdiff_t>(start_index),
                t.begin() + static_cast<std::ptrdiff_t>(end_index + 1));

            local_dvdt =
                calculate_dvdt(
                    v_window,
                    t_window,
                    filter_khz);
        }
        else {
            if (dvdt->size() < end_index) {
                throw std::invalid_argument(
                    "Precomputed dvdt does not match voltage/time arrays");
            }

            local_dvdt.assign(
                dvdt->begin() +
                static_cast<std::ptrdiff_t>(start_index),
                dvdt->begin() +
                static_cast<std::ptrdiff_t>(end_index));
        }

        std::vector<Index> putative_spikes;

        if (local_dvdt.size() >= 2) {
            for (Index i = 0; i + 1 < local_dvdt.size(); ++i) {
                const bool below_before =
                    local_dvdt[i] < dv_cutoff;

                const bool above_after =
                    local_dvdt[i + 1] >= dv_cutoff;

                if (below_before && above_after) {
                    putative_spikes.push_back(i);
                }
            }
        }

        if (putative_spikes.size() <= 1) {
            for (Index& spike : putative_spikes) {
                spike += start_index;
            }

            return putative_spikes;
        }

        std::vector<Index> filtered;
        filtered.reserve(putative_spikes.size());

        filtered.push_back(putative_spikes.front());

        for (Index i = 1; i < putative_spikes.size(); ++i) {
            const Index previous =
                putative_spikes[i - 1];

            const Index current =
                putative_spikes[i];

            bool went_negative = false;

            for (Index j = previous; j < current; ++j) {
                if (local_dvdt[j] < 0.0f) {
                    went_negative = true;
                    break;
                }
            }

            if (went_negative) {
                filtered.push_back(current);
            }
        }

        for (Index& spike : filtered) {
            spike += start_index;
        }

        return filtered;
    }

    std::vector<Index>
        find_peak_indexes(
            const std::vector<float>& v,
            const std::vector<float>& t,
            const std::vector<Index>& spike_indexes,
            std::optional<float> end)
    {
        if (v.size() != t.size()) {
            throw std::invalid_argument(
                "Voltage and time series do not have the same dimensions");
        }

        if (spike_indexes.empty()) {
            return {};
        }

        const float end_time =
            end.has_value() ? *end : t.back();

        const Index end_index =
            find_time_index(t, end_time);

        std::vector<Index> peak_indexes;
        peak_indexes.reserve(spike_indexes.size());

        for (Index i = 0; i < spike_indexes.size(); ++i) {
            const Index begin =
                spike_indexes[i];

            const Index next =
                (i + 1 < spike_indexes.size())
                ? spike_indexes[i + 1]
                : end_index;

            if (begin >= v.size()) {
                throw std::out_of_range(
                    "Spike index is outside voltage array");
            }

            if (next <= begin) {
                peak_indexes.push_back(begin);
                continue;
            }

            Index peak = begin;

            for (Index j = begin + 1;
                j < next && j < v.size();
                ++j)
            {
                if (v[j] > v[peak]) {
                    peak = j;
                }
            }

            peak_indexes.push_back(peak);
        }

        return peak_indexes;
    }

    std::pair<std::vector<Index>, std::vector<Index>>
        filter_putative_spikes(
            const std::vector<float>& v,
            const std::vector<float>& t,
            const std::vector<Index>& spike_indexes,
            const std::vector<Index>& peak_indexes,
            float min_height,
            float min_peak,
            std::optional<float> filter_khz,
            const std::vector<float>* dvdt)
    {
        if (v.size() != t.size()) {
            throw std::invalid_argument(
                "Voltage and time series do not have the same dimensions");
        }

        if (spike_indexes.empty() ||
            peak_indexes.empty())
        {
            return { {}, {} };
        }

        if (spike_indexes.size() != peak_indexes.size()) {
            throw std::invalid_argument(
                "Spike and peak index arrays must have the same length");
        }

        std::vector<float> calculated_dvdt;

        const std::vector<float>* derivative = dvdt;

        if (derivative == nullptr) {
            calculated_dvdt =
                calculate_dvdt(
                    v,
                    t,
                    filter_khz);

            derivative = &calculated_dvdt;
        }

        std::vector<Index> spikes_after_reset;
        std::vector<Index> peaks_after_reset;

        spikes_after_reset.reserve(spike_indexes.size());
        peaks_after_reset.reserve(peak_indexes.size());

        spikes_after_reset.push_back(spike_indexes.front());

        for (Index i = 0; i + 1 < spike_indexes.size(); ++i) {
            const Index peak =
                peak_indexes[i];

            const Index next_spike =
                spike_indexes[i + 1];

            bool went_negative = false;

            const Index stop =
                std::min(next_spike, derivative->size());

            for (Index j = peak; j < stop; ++j) {
                if ((*derivative)[j] < 0.0f) {
                    went_negative = true;
                    break;
                }
            }

            if (went_negative) {
                peaks_after_reset.push_back(
                    peak_indexes[i]);

                spikes_after_reset.push_back(
                    spike_indexes[i + 1]);
            }
        }

        peaks_after_reset.push_back(
            peak_indexes.back());

        if (spikes_after_reset.size() !=
            peaks_after_reset.size())
        {
            throw std::runtime_error(
                "Internal spike/peak alignment error");
        }

        std::vector<Index> spikes_above_peak;
        std::vector<Index> peaks_above_peak;

        for (Index i = 0;
            i < spikes_after_reset.size();
            ++i)
        {
            const Index spike =
                spikes_after_reset[i];

            const Index peak =
                peaks_after_reset[i];

            if (peak >= v.size() ||
                spike >= v.size())
            {
                throw std::out_of_range(
                    "Spike or peak index outside voltage array");
            }

            if (v[peak] >= min_peak) {
                spikes_above_peak.push_back(spike);
                peaks_above_peak.push_back(peak);
            }
        }

        std::vector<Index> final_spikes;
        std::vector<Index> final_peaks;

        final_spikes.reserve(spikes_above_peak.size());
        final_peaks.reserve(peaks_above_peak.size());

        for (Index i = 0;
            i < spikes_above_peak.size();
            ++i)
        {
            const Index spike =
                spikes_above_peak[i];

            const Index peak =
                peaks_above_peak[i];

            if (v[peak] - v[spike] >= min_height) {
                final_spikes.push_back(spike);
                final_peaks.push_back(peak);
            }
        }

        return {
            std::move(final_spikes),
            std::move(final_peaks)
        };
    }


    std::vector<Index>
        find_upstroke_indexes(
            const std::vector<float>& v,
            const std::vector<float>& t,
            const std::vector<Index>& spike_indexes,
            const std::vector<Index>& peak_indexes,
            std::optional<float> filter_khz,
            const std::vector<float>* dvdt)
    {
        if (v.size() != t.size()) {
            throw std::invalid_argument(
                "Voltage and time series do not have the same dimensions");
        }

        if (spike_indexes.size() != peak_indexes.size()) {
            throw std::invalid_argument(
                "Spike and peak index arrays must have the same length");
        }

        if (spike_indexes.empty()) {
            return {};
        }

        std::vector<float> calculated_dvdt;
        const std::vector<float>* derivative = dvdt;

        if (derivative == nullptr) {
            calculated_dvdt = calculate_dvdt(v, t, filter_khz);
            derivative = &calculated_dvdt;
        }

        std::vector<Index> upstroke_indexes;
        upstroke_indexes.reserve(spike_indexes.size());

        for (Index i = 0; i < spike_indexes.size(); ++i) {
            const Index spike = spike_indexes[i];
            const Index peak = peak_indexes[i];

            if (spike >= v.size() || peak >= v.size()) {
                throw std::out_of_range(
                    "Spike or peak index outside voltage array");
            }

            // IPFX uses argmax(dvdt[spike:peak]).  A valid spike/peak
            // pair should therefore have at least one derivative sample.
            if (peak <= spike || spike >= derivative->size()) {
                throw std::invalid_argument(
                    "Peak index must occur after spike index");
            }

            const Index stop = std::min(peak, derivative->size());
            if (stop <= spike) {
                throw std::invalid_argument(
                    "No dV/dt samples available between spike and peak");
            }

            Index best = spike;
            for (Index j = spike + 1; j < stop; ++j) {
                if ((*derivative)[j] > (*derivative)[best]) {
                    best = j;
                }
            }

            upstroke_indexes.push_back(best);
        }

        return upstroke_indexes;
    }

    std::vector<Index>
        refine_threshold_indexes(
            const std::vector<float>& v,
            const std::vector<float>& t,
            const std::vector<Index>& upstroke_indexes,
            float thresh_frac,
            std::optional<float> filter_khz,
            const std::vector<float>* dvdt)
    {
        if (v.size() != t.size()) {
            throw std::invalid_argument(
                "Voltage and time series do not have the same dimensions");
        }

        if (upstroke_indexes.empty()) {
            return {};
        }

        std::vector<float> calculated_dvdt;
        const std::vector<float>* derivative = dvdt;

        if (derivative == nullptr) {
            calculated_dvdt = calculate_dvdt(v, t, filter_khz);
            derivative = &calculated_dvdt;
        }

        double sum = 0.0;
        for (const Index upstroke : upstroke_indexes) {
            if (upstroke >= derivative->size()) {
                throw std::out_of_range(
                    "Upstroke index outside dV/dt array");
            }
            sum += (*derivative)[upstroke];
        }

        const float avg_upstroke =
            static_cast<float>(sum / static_cast<double>(upstroke_indexes.size()));
        const float target = avg_upstroke * thresh_frac;

        std::vector<Index> threshold_indexes;
        threshold_indexes.reserve(upstroke_indexes.size());

        Index previous_upstroke = 0;
        for (const Index upstroke : upstroke_indexes) {
            bool found = false;
            Index threshold = previous_upstroke;

            // Equivalent to numpy dvdt[upstroke:previous_upstroke:-1].
            // Note that previous_upstroke itself is excluded.
            for (Index j = upstroke; j > previous_upstroke; --j) {
                if (j < derivative->size() && (*derivative)[j] <= target) {
                    threshold = j;
                    found = true;
                    break;
                }
            }

            if (!found) {
                threshold = previous_upstroke;
            }

            threshold_indexes.push_back(threshold);
            previous_upstroke = upstroke;
        }

        return threshold_indexes;
    }

    std::vector<bool>
        find_clipped_spikes(
            const std::vector<float>& v,
            const std::vector<float>& t,
            const std::vector<Index>& spike_indexes,
            const std::vector<Index>& peak_indexes,
            Index end_index,
            float tol)
    {
        if (v.size() != t.size()) {
            throw std::invalid_argument(
                "Voltage and time series do not have the same dimensions");
        }

        if (spike_indexes.size() != peak_indexes.size()) {
            throw std::invalid_argument(
                "Spike and peak index arrays must have the same length");
        }

        std::vector<bool> clipped(spike_indexes.size(), false);
        if (spike_indexes.empty()) {
            return clipped;
        }

        if (end_index >= v.size()) {
            throw std::out_of_range(
                "End index outside voltage array");
        }

        const Index last = spike_indexes.size() - 1;
        const Index spike = spike_indexes[last];
        const Index peak = peak_indexes[last];

        if (spike >= v.size() || peak >= v.size()) {
            throw std::out_of_range(
                "Spike or peak index outside voltage array");
        }

        bool returned = false;
        const float return_level = v[spike] + tol;

        for (Index i = peak; i <= end_index; ++i) {
            if (v[i] <= return_level) {
                returned = true;
                break;
            }
        }

        if (!returned) {
            clipped[last] = true;
        }

        return clipped;
    }

    CheckedSpikes
        check_thresholds_and_peaks(
            const std::vector<float>& v,
            const std::vector<float>& t,
            const std::vector<Index>& input_spikes,
            const std::vector<Index>& input_peaks,
            const std::vector<Index>& input_upstrokes,
            std::optional<float> start,
            std::optional<float> end,
            float max_interval,
            float thresh_frac,
            std::optional<float> filter_khz,
            const std::vector<float>* dvdt,
            float tol,
            float reject_at_stim_start_interval)
    {
        if (v.size() != t.size()) {
            throw std::invalid_argument(
                "Voltage and time series do not have the same dimensions");
        }

        if (input_spikes.size() != input_peaks.size() ||
            input_spikes.size() != input_upstrokes.size())
        {
            throw std::invalid_argument(
                "Spike, peak, and upstroke index arrays must have the same length");
        }

        std::vector<Index> spikes = input_spikes;
        std::vector<Index> peaks = input_peaks;
        std::vector<Index> upstrokes = input_upstrokes;

        if (spikes.empty()) {
            return {};
        }

        // Reject detections immediately following stimulus start, when requested.
        if (start.has_value() && reject_at_stim_start_interval > 0.0f) {
            const float reject_until = *start + reject_at_stim_start_interval;

            std::vector<Index> kept_spikes;
            std::vector<Index> kept_peaks;
            std::vector<Index> kept_upstrokes;
            kept_spikes.reserve(spikes.size());
            kept_peaks.reserve(peaks.size());
            kept_upstrokes.reserve(upstrokes.size());

            for (Index i = 0; i < spikes.size(); ++i) {
                if (spikes[i] >= t.size()) {
                    throw std::out_of_range(
                        "Spike index outside time array");
                }

                if (t[spikes[i]] > reject_until) {
                    kept_spikes.push_back(spikes[i]);
                    kept_peaks.push_back(peaks[i]);
                    kept_upstrokes.push_back(upstrokes[i]);
                }
            }

            spikes = std::move(kept_spikes);
            peaks = std::move(kept_peaks);
            upstrokes = std::move(kept_upstrokes);
        }

        if (spikes.empty()) {
            return {};
        }

        // Merge overlapping threshold/peak pairs exactly as IPFX does:
        // keep the earlier threshold, but the later peak/upstroke.
        std::vector<bool> remove_spike(spikes.size(), false);
        std::vector<bool> remove_peak(peaks.size(), false);
        std::vector<bool> remove_upstroke(upstrokes.size(), false);

        for (Index i = 0; i + 1 < spikes.size(); ++i) {
            if (spikes[i + 1] <= peaks[i] + 1) {
                remove_spike[i + 1] = true;
                remove_peak[i] = true;
                remove_upstroke[i] = true;
            }
        }

        if (std::any_of(remove_spike.begin(), remove_spike.end(), [](bool x) { return x; })) {
            std::vector<Index> new_spikes;
            std::vector<Index> new_peaks;
            std::vector<Index> new_upstrokes;

            for (Index i = 0; i < spikes.size(); ++i) {
                if (!remove_spike[i]) {
                    new_spikes.push_back(spikes[i]);
                }
                if (!remove_peak[i]) {
                    new_peaks.push_back(peaks[i]);
                }
                if (!remove_upstroke[i]) {
                    new_upstrokes.push_back(upstrokes[i]);
                }
            }

            spikes = std::move(new_spikes);
            peaks = std::move(new_peaks);
            upstrokes = std::move(new_upstrokes);
        }

        if (spikes.size() != peaks.size() || spikes.size() != upstrokes.size()) {
            throw std::runtime_error(
                "Internal spike/peak/upstroke alignment error");
        }

        // Find threshold/peak pairs that exceed the allowed interval.
        std::vector<Index> too_long;
        for (Index i = 0; i < spikes.size(); ++i) {
            if (spikes[i] >= t.size() || peaks[i] >= t.size()) {
                throw std::out_of_range(
                    "Spike or peak index outside time array");
            }

            if (t[peaks[i]] - t[spikes[i]] >= max_interval) {
                too_long.push_back(i);
            }
        }

        if (!too_long.empty()) {
            std::vector<float> calculated_dvdt;
            const std::vector<float>* derivative = dvdt;

            if (derivative == nullptr) {
                calculated_dvdt = calculate_dvdt(v, t, filter_khz);
                derivative = &calculated_dvdt;
            }

            double sum = 0.0;
            for (const Index upstroke : upstrokes) {
                if (upstroke >= derivative->size()) {
                    throw std::out_of_range(
                        "Upstroke index outside dV/dt array");
                }
                sum += (*derivative)[upstroke];
            }

            const float avg_upstroke =
                static_cast<float>(sum / static_cast<double>(upstrokes.size()));
            const float target = avg_upstroke * thresh_frac;

            std::vector<bool> drop(spikes.size(), false);

            for (const Index i : too_long) {
                // First assume the peak is correct and re-find the threshold
                // within max_interval before it.
                const Index peak = peaks[i];
                const float earliest_time = t[peak] - max_interval;

                Index t0 = 0;
                if (earliest_time > t.front()) {
                    t0 = find_time_index(t, earliest_time);
                }

                bool found_threshold = false;
                const Index upstroke = upstrokes[i];

                // Equivalent to dvdt[upstroke:t0:-1] <= target.
                for (Index j = upstroke; j > t0; --j) {
                    if (j < derivative->size() && (*derivative)[j] <= target) {
                        spikes[i] = j;
                        found_threshold = true;
                        break;
                    }
                }

                if (found_threshold) {
                    continue;
                }

                // Otherwise assume the threshold is correct and search for a
                // better peak in a window twice the allowed threshold->peak time.
                const Index spike = spikes[i];
                const float search_end_time = t[spike] + 2.0f * max_interval;

                Index search_end = t.size() - 1;
                if (search_end_time < t.back()) {
                    search_end = find_time_index(t, search_end_time);
                }

                if (search_end <= spike) {
                    drop[i] = true;
                    continue;
                }

                Index new_peak = spike;
                for (Index j = spike + 1; j < search_end && j < v.size(); ++j) {
                    if (v[j] > v[new_peak]) {
                        new_peak = j;
                    }
                }

                const bool within_interval =
                    t[new_peak] - t[spike] < max_interval;

                const bool before_next_spike =
                    (i == spikes.size() - 1) ||
                    (t[new_peak] < t[spikes[i + 1]]);

                if (within_interval && before_next_spike) {
                    peaks[i] = new_peak;
                }
                else {
                    drop[i] = true;
                }
            }

            if (std::any_of(drop.begin(), drop.end(), [](bool x) { return x; })) {
                std::vector<Index> kept_spikes;
                std::vector<Index> kept_peaks;
                std::vector<Index> kept_upstrokes;

                for (Index i = 0; i < spikes.size(); ++i) {
                    if (!drop[i]) {
                        kept_spikes.push_back(spikes[i]);
                        kept_peaks.push_back(peaks[i]);
                        kept_upstrokes.push_back(upstrokes[i]);
                    }
                }

                spikes = std::move(kept_spikes);
                peaks = std::move(kept_peaks);
                upstrokes = std::move(kept_upstrokes);
            }
        }

        if (spikes.empty()) {
            return {};
        }

        const float end_time = end.has_value() ? *end : t.back();
        Index end_index = t.size() - 1;
        if (end_time <= t.front()) {
            end_index = 0;
        }
        else if (end_time < t.back()) {
            end_index = find_time_index(t, end_time);
        }

        std::vector<bool> clipped =
            find_clipped_spikes(v, t, spikes, peaks, end_index, tol);

        CheckedSpikes result;
        result.spikeIndexes = std::move(spikes);
        result.peakIndexes = std::move(peaks);
        result.upstrokeIndexes = std::move(upstrokes);
        result.clipped = std::move(clipped);
        return result;
    }

} // namespace ipfx
