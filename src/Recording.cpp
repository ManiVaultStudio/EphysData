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
    _stimulusDescription = variantMap["StimulusDescription"].toString();
}

QVariantMap Recording::toVariantMap() const
{
    QVariantMap variantMap;

    variantMap["Data"] = _data.toVariantMap();
    variantMap["StimulusDescription"] = _stimulusDescription;

    return variantMap;
}
