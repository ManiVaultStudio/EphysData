#pragma once

#include "EphysData_export.h"
#include "StimulusRepresentation.h"
#include "TimeSeries.h"

#include <cstddef>
#include <optional>

enum class StimulusType;

namespace StimulusExtraction
{
    // Public stimulus-detection and parameterization API.

    /**
     * Index/time bounds and baseline-relative area of a detected active stimulus region.
     */
    struct StimulusRegion
    {
        size_t begin = 0;
        size_t end = 0;

        double startTime = 0.0;
        double endTime = 0.0;
        double area = 0.0;

        float baseline = 0.0f;

        double Duration() const { return endTime - startTime; }
    };

    EPHYSDATA_EXPORT bool NormalizeTrailingNaNs(TimeSeries& data);

    /**
     * Finds the main stimulus region while ignoring small leading test pulses and noise.
     *
     * Returns std::nullopt when no region looks substantial enough to represent a real stimulus.
     */
    EPHYSDATA_EXPORT std::optional<StimulusRegion> FindMainRegion(
        StimulusType type,
        const TimeSeries& data);

    /**
     * Attempts to replace a raw stimulus waveform with a compact parameterized representation.
     *
     * Returns std::nullopt if the waveform cannot be represented reliably.
     */
    EPHYSDATA_EXPORT std::optional<ParameterizedStimulus> TryParameterize(
        StimulusType type,
        const TimeSeries& data);

} // namespace StimulusExtraction
