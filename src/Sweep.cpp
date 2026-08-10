#include "Sweep.h"

#include "Analysis/SpikeDetection.h"

int Sweep::GetSweepNumber() const
{
    return _sweepNumber;
}

void Sweep::SetSweepNumber(int sweepNumber)
{
    _sweepNumber = sweepNumber;
}

void Sweep::AnalyzeSweep()
{
    stimulus.GetRecording().GetData().ComputeExtents();
    stimulus.CalculateStimulusAmplitude();
    stimulus.DetectStimulusType();
    acquisition.GetRecording().GetData().ComputeExtents();

    _properties.spikeIndices = DetectSpikesIPFX(acquisition.GetRecording().GetData());
}

void Sweep::fromVariantMap(const QVariantMap& variantMap)
{
    mv::util::variantMapMustContain(variantMap, "Stimulus");
    mv::util::variantMapMustContain(variantMap, "Acquisition");
    mv::util::variantMapMustContain(variantMap, "SweepProperties");

    stimulus.fromVariantMap(variantMap["Stimulus"].toMap());
    acquisition.fromVariantMap(variantMap["Acquisition"].toMap());
    _sweepNumber = variantMap.contains("SweepNumber") ? variantMap["SweepNumber"].toInt() : -1;

    _properties.fromVariantMap(variantMap["SweepProperties"].toMap());
}

QVariantMap Sweep::toVariantMap() const
{
    QVariantMap variantMap;

    variantMap["Stimulus"] = stimulus.toVariantMap();
    variantMap["Acquisition"] = acquisition.toVariantMap();
    variantMap["SweepNumber"] = _sweepNumber;
    variantMap["SweepProperties"] = _properties.toVariantMap();

    return variantMap;
}
