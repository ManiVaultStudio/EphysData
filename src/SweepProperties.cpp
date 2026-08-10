#include "SweepProperties.h"

void SweepProperties::fromVariantMap(const QVariantMap& variantMap)
{
    mv::util::variantMapMustContain(variantMap, "SpikeIndices");

    QVariantList spikeIndicesList = variantMap["SpikeIndices"].toList();
    for (int i = 0; i < spikeIndicesList.size(); i++)
        spikeIndices.push_back(spikeIndicesList[i].toInt());
}

QVariantMap SweepProperties::toVariantMap() const
{
    QVariantMap variantMap;

    QVariantList spikeIndicesList;
    for (const int& idx : spikeIndices)
        spikeIndicesList.append(idx);
    variantMap["SpikeIndices"] = spikeIndicesList;

    return variantMap;
}
