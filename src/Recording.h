#pragma once

#include "EphysData_export.h"

#include "TimeSeries.h"

#include <util/Serializable.h>

#include <vector>
#include <string>
#include <memory>

#include <QHash>

class EPHYSDATA_EXPORT Recording : public mv::util::Serializable
{
public:
    TimeSeries& GetData();
    const TimeSeries& GetData() const;

    void AddAttribute(QString attributeName, QString attribute);
    QString GetAttribute(QString attributeName) const;
    const QHash<QString, QString>& GetAttributes() const;

    QString GetStimulusDescription() const;
    void SetStimulusDescription(QString description);

public:
    bool HasAttribute(QString attributeName) const;

public: // Serialization
    void fromVariantMap(const QVariantMap& variantMap) override;
    QVariantMap toVariantMap() const override;

private:
    TimeSeries                  _data;

    QHash<QString, QString>     _attributes;

    QString                     _stimulusDescription;
};
