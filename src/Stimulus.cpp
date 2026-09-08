#include "Stimulus.h"

#include "Analysis/StimulusExtraction.h"

#include <util/Serialization.h>

namespace
{
    void ReadStimulusTiming(StimulusTiming& timing, const QVariantMap& map)
    {
        timing.startTime = map["StartTime"].toDouble();
        timing.duration = map["Duration"].toDouble();
        timing.baseline = map["Baseline"].toFloat();
    }

    void WriteStimulusTiming(const StimulusTiming& timing, QVariantMap& map)
    {
        map["StartTime"] = timing.startTime;
        map["Duration"] = timing.duration;
        map["Baseline"] = timing.baseline;
    }
}

void Stimulus::SetRepresentation(ParameterizedStimulus representation)
{
    std::visit(
        [this](auto&& value)
        {
            _representation = std::forward<decltype(value)>(value);
        },
        std::move(representation));

    _cachedTimeSeries.reset();
}

void Stimulus::SetRepresentation(TimeSeries data, const StimulusTiming& timing)
{
    ArbitraryStimulus arbitrary;
    static_cast<StimulusTiming&>(arbitrary) = timing;
    arbitrary.data = std::move(data);

    _representation = std::move(arbitrary);

    _cachedTimeSeries.reset();
}

void Stimulus::AddAttribute(QString attributeName, QString attribute)
{
    _attributes[attributeName] = attribute;
}

QString Stimulus::GetAttribute(QString attributeName) const
{
    return _attributes[attributeName];
}

const QHash<QString, QString>& Stimulus::GetAttributes() const
{
    return _attributes;
}

bool Stimulus::HasAttribute(QString attributeName) const
{
    return _attributes.contains(attributeName);
}

float Stimulus::GetPeakAmplitude() const
{
    return std::visit(
        [](const auto& representation) -> float
        {
            using T = std::decay_t<decltype(representation)>;

            if constexpr (std::is_same_v<T, SquareStimulus>)
            {
                return representation.amplitude;
            }
            else if constexpr (std::is_same_v<T, RampStimulus>)
            {
                return std::abs(representation.startAmplitude) > std::abs(representation.endAmplitude)
                    ? representation.startAmplitude
                    : representation.endAmplitude;
            }
            else if constexpr (std::is_same_v<T, ChirpStimulus>)
            {
                return representation.amplitude;
            }
            else if constexpr (std::is_same_v<T, ArbitraryStimulus>)
            {
                const auto& y = representation.data.ySeries;

                if (y.empty())
                    return 0.0f;

                const auto [minIt, maxIt] = std::minmax_element(y.begin(), y.end());

                const float minAmplitude = *minIt - representation.baseline;
                const float maxAmplitude = *maxIt - representation.baseline;

                return std::abs(minAmplitude) > std::abs(maxAmplitude) ? minAmplitude : maxAmplitude;
            }

            return 0.0f;
        },
        _representation);
}

void Stimulus::DetectType()
{
    _type = StimulusType::Unknown;
    if (_description.contains("thresh", Qt::CaseInsensitive) || _description.contains("LS") || _description.contains("Rheo", Qt::CaseInsensitive))
    {
        _type = StimulusType::LongSquare; return;
    }
    if (_description.contains("SS"))
    {
        _type = StimulusType::ShortSquare; return;
    }
    if (_description.contains("ramp", Qt::CaseInsensitive) || _description.contains("rmp", Qt::CaseInsensitive))
    {
        _type = StimulusType::Ramp; return;
    }
    if (_description.contains("chirp", Qt::CaseInsensitive))
    {
        _type = StimulusType::Chirp; return;
    }
}

TimeSeries Stimulus::BuildTimeSeries() const
{
    return std::visit(
        [this](const auto& representation) -> TimeSeries
        {
            using T = std::decay_t<decltype(representation)>;

            if constexpr (std::is_same_v<T, ArbitraryStimulus>)
            {
                return representation.data;
            }
            else if constexpr (std::is_same_v<T, SquareStimulus>)
            {
                TimeSeries data;

                const float start = static_cast<float>(representation.startTime);
                const float end = static_cast<float>(representation.startTime + representation.duration);
                const float baseline = representation.baseline;
                const float level = baseline + representation.amplitude;

                data.xSeries = {
                    static_cast<float>(_windowStart),
                    start,
                    start,
                    end,
                    end,
                    static_cast<float>(_windowEnd)
                };

                data.ySeries = {
                    baseline,
                    baseline,
                    level,
                    level,
                    baseline,
                    baseline
                };

                data.ComputeExtents();
                return data;
            }
            else if constexpr (std::is_same_v<T, RampStimulus>)
            {
                TimeSeries data;

                const float start = static_cast<float>(representation.startTime);
                const float end = static_cast<float>(representation.startTime + representation.duration);
                const float baseline = representation.baseline;

                data.xSeries = {
                    static_cast<float>(_windowStart),
                    start,
                    start,
                    end,
                    end,
                    static_cast<float>(_windowEnd)
                };

                data.ySeries = {
                    baseline,
                    baseline,
                    baseline + representation.startAmplitude,
                    baseline + representation.endAmplitude,
                    baseline,
                    baseline
                };

                data.ComputeExtents();
                return data;
            }
            else
            {
                return {};
            }
        },
        _representation);
}

void Stimulus::fromVariantMap(const QVariantMap& variantMap)
{
    mv::util::variantMapMustContain(variantMap, "Type");
    mv::util::variantMapMustContain(variantMap, "Description");
    mv::util::variantMapMustContain(variantMap, "WindowStart");
    mv::util::variantMapMustContain(variantMap, "WindowEnd");
    mv::util::variantMapMustContain(variantMap, "Attributes");
    mv::util::variantMapMustContain(variantMap, "Representation");

    _type = static_cast<StimulusType>(variantMap["Type"].toInt());
    _description = variantMap["Description"].toString();
    _windowStart = variantMap["WindowStart"].toDouble();
    _windowEnd = variantMap["WindowEnd"].toDouble();

    _attributes.clear();

    const QVariantMap attributesMap = variantMap["Attributes"].toMap();
    for (auto it = attributesMap.begin(); it != attributesMap.end(); ++it)
        _attributes[it.key()] = it.value().toString();

    const QVariantMap representationMap = variantMap["Representation"].toMap();

    mv::util::variantMapMustContain(representationMap, "Kind");
    mv::util::variantMapMustContain(representationMap, "StartTime");
    mv::util::variantMapMustContain(representationMap, "Duration");
    mv::util::variantMapMustContain(representationMap, "Baseline");

    const auto kind = static_cast<StimulusRepresentationKind>(representationMap["Kind"].toInt());

    switch (kind)
    {
    case StimulusRepresentationKind::Square:
    {
        mv::util::variantMapMustContain(representationMap, "Amplitude");

        SquareStimulus square;
        ReadStimulusTiming(square, representationMap);
        square.amplitude = representationMap["Amplitude"].toFloat();

        _representation = std::move(square);
        break;
    }

    case StimulusRepresentationKind::Ramp:
    {
        mv::util::variantMapMustContain(representationMap, "StartAmplitude");
        mv::util::variantMapMustContain(representationMap, "EndAmplitude");

        RampStimulus ramp;
        ReadStimulusTiming(ramp, representationMap);
        ramp.startAmplitude = representationMap["StartAmplitude"].toFloat();
        ramp.endAmplitude = representationMap["EndAmplitude"].toFloat();

        _representation = std::move(ramp);
        break;
    }

    case StimulusRepresentationKind::Chirp:
    {
        mv::util::variantMapMustContain(representationMap, "Amplitude");
        mv::util::variantMapMustContain(representationMap, "StartFrequency");
        mv::util::variantMapMustContain(representationMap, "EndFrequency");
        mv::util::variantMapMustContain(representationMap, "FrequencyScale");

        ChirpStimulus chirp;
        ReadStimulusTiming(chirp, representationMap);
        chirp.amplitude = representationMap["Amplitude"].toFloat();
        chirp.startFrequency = representationMap["StartFrequency"].toFloat();
        chirp.endFrequency = representationMap["EndFrequency"].toFloat();
        chirp.frequencyScale = static_cast<ChirpFrequencyScale>(representationMap["FrequencyScale"].toInt());

        _representation = std::move(chirp);
        break;
    }

    case StimulusRepresentationKind::Arbitrary:
    {
        mv::util::variantMapMustContain(representationMap, "Data");

        ArbitraryStimulus arbitrary;
        ReadStimulusTiming(arbitrary, representationMap);
        arbitrary.data.fromVariantMap(representationMap["Data"].toMap());

        _representation = std::move(arbitrary);
        break;
    }

    default:
        throw std::runtime_error("Unknown StimulusRepresentationKind");
    }

    _cachedTimeSeries.reset();
    GetTimeSeries(); // Reconstruct cached waveform during loading
}

QVariantMap Stimulus::toVariantMap() const
{
    QVariantMap map;

    map["Type"] = static_cast<int>(_type);
    map["Description"] = _description;
    map["WindowStart"] = _windowStart;
    map["WindowEnd"] = _windowEnd;

    QVariantMap attributesMap;
    for (auto it = _attributes.constBegin(); it != _attributes.constEnd(); ++it)
        attributesMap[it.key()] = it.value();

    map["Attributes"] = std::move(attributesMap);

    QVariantMap representationMap;

    std::visit(
        [&representationMap](const auto& representation)
        {
            using T = std::decay_t<decltype(representation)>;

            WriteStimulusTiming(representation, representationMap);

            if constexpr (std::is_same_v<T, SquareStimulus>)
            {
                representationMap["Kind"] = static_cast<int>(StimulusRepresentationKind::Square);
                representationMap["Amplitude"] = representation.amplitude;
            }
            else if constexpr (std::is_same_v<T, RampStimulus>)
            {
                representationMap["Kind"] = static_cast<int>(StimulusRepresentationKind::Ramp);
                representationMap["StartAmplitude"] = representation.startAmplitude;
                representationMap["EndAmplitude"] = representation.endAmplitude;
            }
            else if constexpr (std::is_same_v<T, ChirpStimulus>)
            {
                representationMap["Kind"] = static_cast<int>(StimulusRepresentationKind::Chirp);
                representationMap["Amplitude"] = representation.amplitude;
                representationMap["StartFrequency"] = representation.startFrequency;
                representationMap["EndFrequency"] = representation.endFrequency;
                representationMap["FrequencyScale"] = static_cast<int>(representation.frequencyScale);
            }
            else if constexpr (std::is_same_v<T, ArbitraryStimulus>)
            {
                representationMap["Kind"] = static_cast<int>(StimulusRepresentationKind::Arbitrary);
                representationMap["Data"] = representation.data.toVariantMap();
            }
        },
        _representation);

    map["Representation"] = std::move(representationMap);

    return map;
}
