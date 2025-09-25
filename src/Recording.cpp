#include "Recording.h"

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

int Recording::GetSweepNumber() const
{
    return _sweepNumber;
}

void Recording::SetSweepNumber(int sweepNumber)
{
    _sweepNumber = sweepNumber;
}

QString Recording::GetStimulusDescription() const
{
    return _stimulusDescription;
}

void Recording::SetStimulusDescription(QString description)
{
    _stimulusDescription = description;
}

void Recording::fromVariantMap(const QVariantMap& variantMap)
{
    mv::util::variantMapMustContain(variantMap, "Data");
    mv::util::variantMapMustContain(variantMap, "StimulusDescription");

    _data.fromVariantMap(variantMap["Data"].toMap());
    _sweepNumber = variantMap.contains("SweepNumber") ? variantMap["SweepNumber"].toInt() : 0;
    _stimulusDescription = variantMap["StimulusDescription"].toString();

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
    variantMap["SweepNumber"] = _sweepNumber;
    variantMap["StimulusDescription"] = _stimulusDescription;

    // Store attributes
    QVariantMap attributeMap;
    for (auto it = _attributes.constBegin(); it != _attributes.constEnd(); ++it) {
        attributeMap.insert(it.key(), it.value());
    }

    variantMap["Attributes"] = attributeMap;

    return variantMap;
}
