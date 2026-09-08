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
