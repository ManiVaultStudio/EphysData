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

public:
    bool HasAttribute(QString attributeName) const;

public: // Serialization
    void fromVariantMap(const QVariantMap& variantMap) override;
    QVariantMap toVariantMap() const override;

private:
    TimeSeries                  _data;

    QHash<QString, QString>     _attributes;

class EPHYSDATA_EXPORT Stimulus : public mv::util::Serializable
{
public:
    Recording& GetRecording() { return _recording; }
    const Recording& GetRecording() const { return _recording; }

    QString GetStimulusDescription() const;
    void SetStimulusDescription(QString description);

    float GetStimulusAmplitude() const { return _stimulusAmplitude; }
    void CalculateStimulusAmplitude();

    StimulusType GetStimulusType() const { return _stimulusType; }
    void DetectStimulusType();

public: // Serialization
    void fromVariantMap(const QVariantMap& variantMap) override;
    QVariantMap toVariantMap() const override;

private:
    Recording       _recording;

    QString         _stimulusDescription;
    float           _stimulusAmplitude;

    StimulusType    _stimulusType;
};
