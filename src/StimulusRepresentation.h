#pragma once

#include "Recording.h"

#include <variant>

struct StimulusTiming
{
    double startTime = 0.0;
    double duration = 0.0;

    float baseline = 0.0f;
};

struct SquareStimulus : StimulusTiming
{
    float amplitude = 0.0f;
};

struct RampStimulus : StimulusTiming
{
    float startAmplitude = 0.0f;
    float endAmplitude = 0.0f;
};

enum class ChirpFrequencyScale
{
    Linear,
    Logarithmic
};

struct ChirpStimulus : StimulusTiming
{
    float amplitude = 0.0f;
    float startFrequency = 0.0f;
    float endFrequency = 0.0f;

    ChirpFrequencyScale frequencyScale = ChirpFrequencyScale::Linear;
};

struct ArbitraryStimulus : StimulusTiming
{
    TimeSeries data;
};

using ParameterizedStimulus = std::variant<
    SquareStimulus,
    RampStimulus,
    ChirpStimulus
>;

using StimulusRepresentation = std::variant<
    SquareStimulus,
    RampStimulus,
    ChirpStimulus,
    ArbitraryStimulus
>;

enum class StimulusRepresentationKind
{
    Square,
    Ramp,
    Chirp,
    Arbitrary
};
