#include "Sweep.h"

int Sweep::GetSweepNumber() const
{
    return _sweepNumber;
}

void Sweep::SetSweepNumber(int sweepNumber)
{
    _sweepNumber = sweepNumber;
}

void Sweep::fromVariantMap(const QVariantMap& variantMap)
{
    mv::util::variantMapMustContain(variantMap, "Stimulus");
    mv::util::variantMapMustContain(variantMap, "Acquisition");

    stimulus.fromVariantMap(variantMap["Stimulus"].toMap());
    acquisition.fromVariantMap(variantMap["Acquisition"].toMap());
    _sweepNumber = variantMap.contains("SweepNumber") ? variantMap["SweepNumber"].toInt() : -1;
}

QVariantMap Sweep::toVariantMap() const
{
    QVariantMap variantMap;

    variantMap["Stimulus"] = stimulus.toVariantMap();
    variantMap["Acquisition"] = acquisition.toVariantMap();
    variantMap["SweepNumber"] = _sweepNumber;

    return variantMap;
}
