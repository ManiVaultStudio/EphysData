#include "StimulusExtraction.h"

#include "Stimulus.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <vector>

// Internal stimulus-detection helpers. These functions estimate baseline/noise, identify active regions,
// reject likely test pulses, and fit compact stimulus representations.

namespace
{
    // Shared values used while detecting active stimulus regions.
    struct DetectionContext
    {
        float baseline = 0.0f;
        float noise = 0.0f;
        float threshold = 0.0f;
    };

    // Verify that the waveform contains finite, matching X/Y data with increasing timestamps.
    bool IsValid(const TimeSeries& data)
    {
        // If either X or Y in the timeseries is empty, it's invalid
        if (data.xSeries.empty() || data.ySeries.empty())
        {
            qWarning() << "Stimulus data is empty, xSeries: " << data.xSeries.size() << "ySeries: " << data.ySeries.size();
            return false;
        }

        // If xSeries and ySeries have differing numbers of values, it's invalid
        if (data.xSeries.size() != data.ySeries.size())
        {
            qWarning() << "Stimulus data differs in size, xSeries: " << data.xSeries.size() << "ySeries: " << data.ySeries.size();
            return false;
        }

        // Make sure xSeries has increasing timestamps
        for (size_t i = 0; i < data.xSeries.size(); ++i)
        {
            //if (!std::isfinite(data.xSeries[i]) || !std::isfinite(data.ySeries[i]))
            //{
            //    qWarning() << "Stimulus data contains non-finite values, X:" << data.xSeries[i] << "Y:" << data.ySeries[i];
            //    return false;
            //}

            if (i > 0 && data.xSeries[i] <= data.xSeries[i - 1])
            {
                qWarning() << "Stimulus timeseries is non-monotonically increasing";
                return false;
            }
        }

        return true;
    }

    // Returns the median of a copy of the supplied values.
    float Median(std::vector<float> values)
    {
        if (values.empty())
            return 0.0f;

        const size_t middle = values.size() / 2;
        std::nth_element(values.begin(), values.begin() + middle, values.end());

        float median = values[middle];

        if (values.size() % 2 == 0)
        {
            const auto lower = std::max_element(values.begin(), values.begin() + middle);
            median = (*lower + median) * 0.5f;
        }

        return median;
    }

    // Returns the arithmetic mean over the half-open range [begin, end).
    float Mean(const std::vector<float>& values, size_t begin, size_t end)
    {
        if (begin >= end || end > values.size())
            return 0.0f;

        double sum = 0.0;

        for (size_t i = begin; i < end; ++i)
            sum += values[i];

        return static_cast<float>(sum / static_cast<double>(end - begin));
    }

    // Estimates baseline from samples at both ends of the sweep using a robust median.
    float EstimateBaseline(const TimeSeries& data)
    {
        if (data.ySeries.empty())
            return 0.0f;

        // Use the first 1% of the sweep. This should contain mostly pre-stimulus baseline.
        const size_t count = std::max<size_t>(1, data.ySeries.size() / 100);

        std::vector<float> samples(data.ySeries.begin(), data.ySeries.begin() + count);
        return Median(std::move(samples));
    }

    // Estimates baseline noise using the median absolute deviation from the baseline.
    float EstimateNoise(const TimeSeries& data, float baseline)
    {
        if (data.ySeries.empty())
            return 0.0f;

        const size_t count = std::max<size_t>(1, data.ySeries.size() / 100);

        std::vector<float> deviations;
        deviations.reserve(count);

        for (size_t i = 0; i < count; ++i)
            deviations.push_back(std::abs(data.ySeries[i] - baseline));

        return Median(std::move(deviations));
    }

    // Computes the baseline, noise estimate and activity threshold used by the detector.
    DetectionContext BuildDetectionContext(const TimeSeries& data)
    {
        DetectionContext context;
        context.baseline = EstimateBaseline(data);
        context.noise = EstimateNoise(data, context.baseline);
        // Keep the threshold above both measured baseline noise and a small absolute floor.
        // Require a deviation well above estimated noise, with a small absolute floor.
        context.threshold = std::max(context.noise * 6.0f, 0.001f);

        const auto [minIt, maxIt] = std::minmax_element(data.ySeries.begin(), data.ySeries.end());

        qDebug() << "Stimulus detection:"
            << "baseline =" << context.baseline
            << "noise =" << context.noise
            << "threshold =" << context.threshold
            << "yMin =" << *minIt
            << "yMax =" << *maxIt;

        return context;
    }

    // Converts the last active sample index into an exclusive end time for duration calculations.
    double GetExclusiveEndTime(const TimeSeries& data, size_t endIndex)
    {
        if (endIndex + 1 < data.xSeries.size())
            return static_cast<double>(data.xSeries[endIndex + 1]);

        if (endIndex == 0)
            return static_cast<double>(data.xSeries[endIndex]);

        const double current = static_cast<double>(data.xSeries[endIndex]);
        const double previous = static_cast<double>(data.xSeries[endIndex - 1]);

        return current + (current - previous);
    }

    // Computes baseline-relative signal area using trapezoidal integration over time.
    double ComputeRegionArea(const TimeSeries& data, const StimulusExtraction::StimulusRegion& region, float baseline)
    {
        if (region.begin >= region.end)
            return 0.0;

        double area = 0.0;

        // Trapezoidal integration makes area meaningful even if the time axis is not uniformly sampled.
        for (size_t i = region.begin; i < region.end; ++i)
        {
            const double y0 = std::abs(static_cast<double>(data.ySeries[i]) - baseline);
            const double y1 = std::abs(static_cast<double>(data.ySeries[i + 1]) - baseline);
            const double dt = static_cast<double>(data.xSeries[i + 1]) - data.xSeries[i];

            area += 0.5 * (y0 + y1) * dt;
        }

        return area;
    }

    // Finds all contiguous regions whose signal exceeds the baseline-relative activity threshold.
    std::vector<StimulusExtraction::StimulusRegion> FindActiveRegions(const TimeSeries& data, const DetectionContext& context)
    {
        size_t activeCount = 0;
        size_t firstActive = data.ySeries.size();

        for (size_t i = 0; i < data.ySeries.size(); ++i)
        {
            if (std::abs(data.ySeries[i] - context.baseline) > context.threshold)
            {
                ++activeCount;

                if (firstActive == data.ySeries.size())
                    firstActive = i;
            }
        }

        qDebug() << "Active samples:" << activeCount;

        if (activeCount > 0)
        {
            qDebug() << "First active:"
                << firstActive
                << "x =" << data.xSeries[firstActive]
                << "y =" << data.ySeries[firstActive];
        }

        std::vector<StimulusExtraction::StimulusRegion> regions;

        bool inRegion = false;
        size_t begin = 0;

        for (size_t i = 0; i < data.ySeries.size(); ++i)
        {
            // Activity is measured relative to the estimated baseline, not relative to zero.
            const bool active = std::abs(data.ySeries[i] - context.baseline) > context.threshold;

            if (active && !inRegion)
            {
                begin = i;
                inRegion = true;
            }

            if (!inRegion)
                continue;

            const bool finalSample = i == data.ySeries.size() - 1;

            if (active && !finalSample)
                continue;

            const size_t end = active ? i : i - 1;

            StimulusExtraction::StimulusRegion region;
            region.begin = begin;
            region.end = end;
            region.startTime = static_cast<double>(data.xSeries[begin]);
            region.endTime = GetExclusiveEndTime(data, end);
            region.baseline = context.baseline;
            region.area = ComputeRegionArea(data, region, context.baseline);

            if (region.endTime > region.startTime)
                regions.push_back(region);

            inRegion = false;
        }

        return regions;
    }

    // Finds the dominant active region inside an already-trimmed stimulus waveform.
    std::optional<StimulusExtraction::StimulusRegion> FindActiveRegion(const TimeSeries& data, const DetectionContext& context)
    {
        auto regions = FindActiveRegions(data, context);

        if (regions.empty())
            return std::nullopt;

        // The loader has already isolated the main stimulus, so simply choose the region with the largest area.
        const auto largest = std::max_element(
            regions.begin(),
            regions.end(),
            [](const auto& a, const auto& b)
            {
                return a.area < b.area;
            });

        return *largest;
    }

    // Removes a small leading region when its area is much smaller than a later stimulus region.
    void RemoveLikelyTestPulse(std::vector<StimulusExtraction::StimulusRegion>& regions)
    {
        if (regions.size() < 2)
            return;

        // Compare the leading region against the largest later region.
        // A much smaller first region is treated as a test pulse.
        const auto largestLater = std::max_element(
            regions.begin() + 1,
            regions.end(),
            [](const auto& a, const auto& b)
            {
                return a.area < b.area;
            });

        constexpr double TEST_PULSE_AREA_RATIO = 0.25;

        // Only remove the first region when a clearly larger later stimulus exists.
        if (regions.front().area < largestLater->area * TEST_PULSE_AREA_RATIO)
            regions.erase(regions.begin());
    }

    // Returns the minimum duration required for a region to count as a real stimulus for each protocol type.
    double GetMinimumMainStimulusDuration(StimulusType type)
    {
        // These are deliberately conservative defaults. Tune them to the shortest real protocols in your data.
        switch (type)
        {
        case StimulusType::LongSquare:
            return 0.050;

        case StimulusType::ShortSquare:
            return 0.001;

        case StimulusType::Ramp:
            return 0.050;

        case StimulusType::Chirp:
            return 0.050;

        case StimulusType::Unknown:
            return 0.050;
        }

        return 0.050;
    }

    // Rejects test-pulse/short regions and selects the most substantial remaining stimulus region by area.
    std::optional<StimulusExtraction::StimulusRegion> SelectMainRegion(StimulusType type, std::vector<StimulusExtraction::StimulusRegion> regions)
    {
        if (regions.empty())
            return std::nullopt;

        // Test pulses usually occur first and are much smaller than the protocol stimulus.
        RemoveLikelyTestPulse(regions);

        const double minimumDuration = GetMinimumMainStimulusDuration(type);

        std::optional<StimulusExtraction::StimulusRegion> best;

        for (const auto& region : regions)
        {
            // Prevent a lone short test pulse from being accepted as the main stimulus.
            if (region.Duration() < minimumDuration)
                continue;

            // Area is a useful measure of how substantial a stimulus region is.
            if (!best || region.area > best->area)
                best = region;
        }

        return best;
    }

    // Measures how closely the detected region matches an ideal constant square plateau.
    double CalculateSquareRmsError(const TimeSeries& data, const StimulusExtraction::StimulusRegion& region, const SquareStimulus& square)
    {
        const double expected = static_cast<double>(square.baseline + square.amplitude);

        double squaredError = 0.0;
        size_t count = 0;

        for (size_t i = region.begin; i <= region.end; ++i)
        {
            const double error = static_cast<double>(data.ySeries[i]) - expected;
            squaredError += error * error;
            ++count;
        }

        if (count == 0)
            return std::numeric_limits<double>::infinity();

        return std::sqrt(squaredError / static_cast<double>(count));
    }

    // Fits and validates a compact square-stimulus representation for the detected main region.
    std::optional<SquareStimulus> ExtractSquare(const TimeSeries& data, const StimulusExtraction::StimulusRegion& region, const DetectionContext& context)
    {
        const size_t length = region.end - region.begin + 1;

        if (length < 3)
            return std::nullopt;

        // Ignore a small amount at both edges to reduce transition/ringing artifacts.
        const size_t trim = static_cast<size_t>(length * 0.05f);

        size_t plateauBegin = region.begin + trim;
        size_t plateauEnd = region.end + 1 - trim;

        if (plateauBegin >= plateauEnd)
        {
            plateauBegin = region.begin;
            plateauEnd = region.end + 1;
        }

        SquareStimulus result;
        result.baseline = context.baseline;
        result.amplitude = Mean(data.ySeries, plateauBegin, plateauEnd) - context.baseline;
        result.startTime = region.startTime;
        result.duration = region.Duration();

        const double rmsError = CalculateSquareRmsError(data, region, result);
        const double noiseTolerance = static_cast<double>(context.noise) * 5.0;
        const double relativeTolerance = std::abs(static_cast<double>(result.amplitude)) * 0.01;
        const double tolerance = std::max(noiseTolerance, relativeTolerance);

        // Parameterization discards the original waveform, so reject questionable fits.
        if (rmsError > tolerance)
            return std::nullopt;

        return result;
    }

    // Least-squares line fit used to model ramp stimuli.
    struct LinearFit
    {
        double slope = 0.0;
        double intercept = 0.0;
        double rmsError = std::numeric_limits<double>::infinity();
    };

    // Fits a baseline-relative straight line to the detected region and calculates its RMS residual.
    std::optional<LinearFit> FitLine(const TimeSeries& data, const StimulusExtraction::StimulusRegion& region, float baseline)
    {
        const size_t count = region.end - region.begin + 1;

        if (count < 2)
            return std::nullopt;

        double sumT = 0.0;
        double sumY = 0.0;
        double sumTT = 0.0;
        double sumTY = 0.0;

        for (size_t i = region.begin; i <= region.end; ++i)
        {
            const double t = static_cast<double>(data.xSeries[i]) - region.startTime;
            const double y = static_cast<double>(data.ySeries[i]) - baseline;

            sumT += t;
            sumY += y;
            sumTT += t * t;
            sumTY += t * y;
        }

        const double denominator = static_cast<double>(count) * sumTT - sumT * sumT;

        if (std::abs(denominator) < 1e-12)
            return std::nullopt;

        LinearFit fit;
        fit.slope = (static_cast<double>(count) * sumTY - sumT * sumY) / denominator;
        fit.intercept = (sumY - fit.slope * sumT) / static_cast<double>(count);

        double squaredError = 0.0;

        for (size_t i = region.begin; i <= region.end; ++i)
        {
            const double t = static_cast<double>(data.xSeries[i]) - region.startTime;
            const double observed = static_cast<double>(data.ySeries[i]) - baseline;
            const double expected = fit.intercept + fit.slope * t;
            const double error = observed - expected;

            squaredError += error * error;
        }

        fit.rmsError = std::sqrt(squaredError / static_cast<double>(count));

        return fit;
    }

    // Fits and validates a compact linear-ramp representation for the detected main region.
    std::optional<RampStimulus> ExtractRamp(const TimeSeries& data, const StimulusExtraction::StimulusRegion& region, const DetectionContext& context)
    {
        if (region.end - region.begin + 1 < 4)
            return std::nullopt;

        // Use all samples in the active region rather than estimating the ramp from its endpoints alone.
        const auto fit = FitLine(data, region, context.baseline);

        if (!fit)
            return std::nullopt;

        RampStimulus result;
        result.baseline = context.baseline;
        result.startTime = region.startTime;
        result.duration = region.Duration();
        result.startAmplitude = static_cast<float>(fit->intercept);
        result.endAmplitude = static_cast<float>(fit->intercept + fit->slope * result.duration);

        const double amplitudeRange = std::abs(
            static_cast<double>(result.endAmplitude) - static_cast<double>(result.startAmplitude));

        const double noiseTolerance = static_cast<double>(context.noise) * 5.0;
        const double relativeTolerance = amplitudeRange * 0.01;
        const double tolerance = std::max(noiseTolerance, relativeTolerance);

        if (fit->rmsError > tolerance)
            return std::nullopt;

        return result;
    }

    // Placeholder for chirp extraction; returns null until the exact chirp model can be validated.
    std::optional<ChirpStimulus> ExtractChirp(const TimeSeries&, const StimulusExtraction::StimulusRegion&, const DetectionContext&)
    {
        // Keep chirps arbitrary until the exact chirp model can be reconstructed and validated safely.
        return std::nullopt;
    }

} // namespace

namespace StimulusExtraction
{
    bool NormalizeTrailingNaNs(TimeSeries& data)
    {
        if (data.ySeries.empty())
            return false;

        const float baseline = EstimateBaseline(data);

        size_t firstNaN = data.ySeries.size();

        for (size_t i = 0; i < data.ySeries.size(); ++i)
        {
            if (!std::isfinite(data.ySeries[i]))
            {
                firstNaN = i;
                break;
            }
        }

        if (firstNaN == data.ySeries.size())
            return true;

        if (firstNaN == 0)
            return false;

        // NaNs must form one trailing block.
        for (size_t i = firstNaN; i < data.ySeries.size(); ++i)
        {
            if (std::isfinite(data.ySeries[i]))
                return false;
        }

        constexpr size_t CHECK_SAMPLES = 100;
        const size_t begin = firstNaN > CHECK_SAMPLES ? firstNaN - CHECK_SAMPLES : 0;

        float maxDeviation = 0.0f;

        for (size_t i = begin; i < firstNaN; ++i)
            maxDeviation = std::max(maxDeviation, std::abs(data.ySeries[i] - baseline));

        constexpr float ACTIVE_THRESHOLD = 0.001f;

        // NaNs started while the waveform was still active.
        if (maxDeviation > ACTIVE_THRESHOLD)
            return false;

        for (size_t i = firstNaN; i < data.ySeries.size(); ++i)
            data.ySeries[i] = baseline;

        return true;
    }

    // Detects and returns the main stimulus region without attempting to parameterize its shape.
    std::optional<StimulusRegion> FindMainRegion(StimulusType type, const TimeSeries& data)
    {
        if (!IsValid(data))
        {
            qWarning("[FindMainRegion] Stimulus data is invalid");
            return std::nullopt;
        }

        const DetectionContext context = BuildDetectionContext(data);
        auto regions = FindActiveRegions(data, context);

        return SelectMainRegion(type, std::move(regions));
    }

    // Detects the main region and dispatches to the extractor appropriate for the stimulus type.
    std::optional<ParameterizedStimulus> TryParameterize(StimulusType type, const TimeSeries& data)
    {
        if (!IsValid(data))
            return std::nullopt;

        const DetectionContext context = BuildDetectionContext(data);

        // The loader already isolated the main stimulus, so we only need its active bounds here.
        const auto region = FindActiveRegion(data, context);

        if (!region)
            return std::nullopt;

        switch (type)
        {
        case StimulusType::LongSquare:
        case StimulusType::ShortSquare:
            if (auto result = ExtractSquare(data, *region, context))
                return ParameterizedStimulus{ std::move(*result) };
            break;

        case StimulusType::Ramp:
            if (auto result = ExtractRamp(data, *region, context))
                return ParameterizedStimulus{ std::move(*result) };
            break;

        case StimulusType::Chirp:
            if (auto result = ExtractChirp(data, *region, context))
                return ParameterizedStimulus{ std::move(*result) };
            break;

        case StimulusType::Unknown:
            break;
        }

        return std::nullopt;
    }

} // namespace StimulusExtraction
