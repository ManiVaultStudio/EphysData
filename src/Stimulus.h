#pragma once

#include "EphysData_export.h"

#include "StimulusRepresentation.h"

#include <util/Serializable.h>

#include <variant>

enum class StimulusType
{
    LongSquare,
    ShortSquare,
    Ramp,
    Chirp,
    Unknown
};

class EPHYSDATA_EXPORT Stimulus : public mv::util::Serializable
{
public:
    void SetRepresentation(ParameterizedStimulus representation);
    void SetRepresentation(TimeSeries data, const StimulusTiming& timing);

    const StimulusRepresentation& GetRepresentation() const { return _representation; }
    StimulusRepresentation& GetRepresentation() { return _representation; }
    bool IsParameterized() const { return !std::holds_alternative<ArbitraryStimulus>(_representation); }

    TimeSeries* GetArbitraryData()
    {
        auto* arbitrary = std::get_if<ArbitraryStimulus>(&_representation);
        return arbitrary ? &arbitrary->data : nullptr;
    }

    float GetYMin() const
    {
        return std::visit(
            [](const auto& representation) -> float
            {
                using T = std::decay_t<decltype(representation)>;

                if constexpr (std::is_same_v<T, SquareStimulus>)
                {
                    return std::min(representation.baseline, representation.baseline + representation.amplitude);
                }
                else if constexpr (std::is_same_v<T, RampStimulus>)
                {
                    return std::min({
                        representation.baseline,
                        representation.baseline + representation.startAmplitude,
                        representation.baseline + representation.endAmplitude
                        });
                }
                else if constexpr (std::is_same_v<T, ChirpStimulus>)
                {
                    return representation.baseline - std::abs(representation.amplitude);
                }
                else
                {
                    return representation.data.yMin;
                }
            },
            _representation);
    }
    float GetYMax() const
    {
        return std::visit(
            [](const auto& representation) -> float
            {
                using T = std::decay_t<decltype(representation)>;

                if constexpr (std::is_same_v<T, SquareStimulus>)
                {
                    return std::max(representation.baseline, representation.baseline + representation.amplitude);
                }
                else if constexpr (std::is_same_v<T, RampStimulus>)
                {
                    return std::max({
                        representation.baseline,
                        representation.baseline + representation.startAmplitude,
                        representation.baseline + representation.endAmplitude
                        });
                }
                else if constexpr (std::is_same_v<T, ChirpStimulus>)
                {
                    return representation.baseline + std::abs(representation.amplitude);
                }
                else
                {
                    return representation.data.yMax;
                }
            },
            _representation);
    }

    float GetWindowStart() const { return _windowStart; }
    float GetWindowEnd() const { return _windowEnd; }
    void SetWindow(double start, double end) {
        _windowStart = start;
        _windowEnd = end;
        _cachedTimeSeries.reset();
    }

    const TimeSeries& GetTimeSeries() const
    {
        if (!_cachedTimeSeries)
            _cachedTimeSeries = BuildTimeSeries();

        return *_cachedTimeSeries;
    }

    StimulusType GetType() const { return _type; }
    void SetType(StimulusType type) { _type = type; }

    QString GetDescription() const { return _description; }
    void SetDescription(QString description) { _description = description; }

    void AddAttribute(QString attributeName, QString attribute);
    QString GetAttribute(QString attributeName) const;
    const QHash<QString, QString>& GetAttributes() const;

    float GetPeakAmplitude() const;

    void DetectType();

private:
    TimeSeries BuildTimeSeries() const;

public:
    bool HasAttribute(QString attributeName) const;

public: // Serialization
    void fromVariantMap(const QVariantMap& variantMap) override;
    QVariantMap toVariantMap() const override;

private:
    StimulusRepresentation  _representation = ArbitraryStimulus{};

    StimulusType            _type = StimulusType::Unknown;
    QString                 _description;

    double _windowStart = 0.0;
    double _windowEnd = 0.0;

    mutable std::optional<TimeSeries> _cachedTimeSeries;

    QHash<QString, QString> _attributes;
};

inline StimulusType StimulusTypeFromString(const QString& s)
{
    if (s == "Long Square")   return StimulusType::LongSquare;
    if (s == "Short Square") return StimulusType::ShortSquare;
    if (s == "Ramp")  return StimulusType::Ramp;
    if (s == "Chirp")  return StimulusType::Chirp;
    return StimulusType::Unknown;
}

inline QString ToString(StimulusType stimulusType)
{
    switch (stimulusType)
    {
    case StimulusType::LongSquare: return QString("Long Square");
    case StimulusType::ShortSquare: return QString("Short Square");
    case StimulusType::Ramp: return QString("Ramp");
    case StimulusType::Chirp: return QString("Chirp");
    default: return QString("Unknown");
    }
}
