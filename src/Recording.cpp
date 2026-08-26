#include "Recording.h"

#include <util/Serialization.h>

TimeSeries& Recording::GetData()
{
    return _data;
}

const TimeSeries& Recording::GetData() const
{
    return _data;
}

void Recording::AddAttribute(QString attributeName, QString attribute)
{
    _attributes[attributeName] = attribute;
}

QString Recording::GetAttribute(QString attributeName) const
{
    return _attributes[attributeName];
}

const QHash<QString, QString>& Recording::GetAttributes() const
{
    return _attributes;
}

bool Recording::HasAttribute(QString attributeName) const
{
    return _attributes.contains(attributeName);
}

QString Stimulus::GetStimulusDescription() const
{
    return _stimulusDescription;
}

void Stimulus::SetStimulusDescription(QString description)
{
    _stimulusDescription = description;
}

void Stimulus::CalculateStimulusAmplitude()
{
    float yMin = std::numeric_limits<float>::max();
    float yMax = -std::numeric_limits<float>::max();
    const std::vector<float>& ySeries = _recording.GetData().ySeries;
    for (int i = 0; i < ySeries.size(); i++)
    {
        if (ySeries[i] < yMin) yMin = ySeries[i];
        if (ySeries[i] > yMax) yMax = ySeries[i];
    }
    _stimulusAmplitude = abs(yMin) > abs(yMax) ? yMin : yMax;
}

void Stimulus::DetectStimulusType()
{
    _stimulusType = StimulusType::Unknown;
    if (_stimulusDescription.contains("thresh", Qt::CaseInsensitive) || _stimulusDescription.contains("LS") || _stimulusDescription.contains("Rheo", Qt::CaseInsensitive))
    {
        _stimulusType = StimulusType::LongSquare; return;
    }
    if (_stimulusDescription.contains("SS"))
    {
        _stimulusType = StimulusType::ShortSquare; return;
    }
    if (_stimulusDescription.contains("ramp", Qt::CaseInsensitive) || _stimulusDescription.contains("rmp", Qt::CaseInsensitive))
    {
        _stimulusType = StimulusType::Ramp; return;
    }
    if (_stimulusDescription.contains("chirp", Qt::CaseInsensitive))
    {
        _stimulusType = StimulusType::Chirp; return;
    }
}

void Recording::fromVariantMap(const QVariantMap& variantMap)
{
    mv::util::variantMapMustContain(variantMap, "Data");

    _data.fromVariantMap(variantMap["Data"].toMap());

    // Load attributes
    if (variantMap.contains("Attributes"))
    {
        QVariantMap attributeMap = variantMap["Attributes"].toMap();
        for (auto it = attributeMap.constBegin(); it != attributeMap.constEnd(); ++it) {
            _attributes.insert(it.key(), it.value().toString());
        }
    }
}

QVariantMap Recording::toVariantMap() const
{
    QVariantMap variantMap;

    variantMap["Data"] = _data.toVariantMap();

    // Store attributes
    QVariantMap attributeMap;
    for (auto it = _attributes.constBegin(); it != _attributes.constEnd(); ++it) {
        attributeMap.insert(it.key(), it.value());
    }

    variantMap["Attributes"] = attributeMap;

    return variantMap;
}

void Stimulus::fromVariantMap(const QVariantMap& variantMap)
{
    mv::util::variantMapMustContain(variantMap, "Recording");
    mv::util::variantMapMustContain(variantMap, "StimulusDescription");
    mv::util::variantMapMustContain(variantMap, "StimulusAmplitude");

    _recording.fromVariantMap(variantMap["Recording"].toMap());

    _stimulusType = static_cast<StimulusType>(variantMap["StimulusType"].toInt());
    _stimulusDescription = variantMap["StimulusDescription"].toString();
    _stimulusAmplitude = variantMap["StimulusAmplitude"].toFloat();
}

QVariantMap Stimulus::toVariantMap() const
{
    QVariantMap variantMap;

    variantMap["Recording"] = _recording.toVariantMap();

    variantMap["StimulusType"] = static_cast<int>(_stimulusType);
    variantMap["StimulusDescription"] = _stimulusDescription;
    variantMap["StimulusAmplitude"] = _stimulusAmplitude;

    return variantMap;
}

void Acquisition::fromVariantMap(const QVariantMap& variantMap)
{
    mv::util::variantMapMustContain(variantMap, "Recording");

    _recording.fromVariantMap(variantMap["Recording"].toMap());
}

QVariantMap Acquisition::toVariantMap() const
{
    QVariantMap variantMap;

    variantMap["Recording"] = _recording.toVariantMap();

    return variantMap;
}
