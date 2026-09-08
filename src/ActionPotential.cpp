#include "ActionPotential.h"

#include <util/Serialization.h>

ActionPotential::ActionPotential()
{

}

ActionPotential::ActionPotential(std::vector<float> timeSeries, std::vector<float> voltageSeries, int peakIndex) :
    _timeSeries(std::move(timeSeries)),
    _voltageSeries(std::move(voltageSeries)),
    _peakIndex(peakIndex)
{
}

void ActionPotential::fromVariantMap(const QVariantMap& variantMap)
{
    mv::util::variantMapMustContain(variantMap, "NumDataPoints");
    mv::util::variantMapMustContain(variantMap, "TimeSeries");
    mv::util::variantMapMustContain(variantMap, "VoltageSeries");
    mv::util::variantMapMustContain(variantMap, "PeakIndex");

    int numDataPoints = variantMap["NumDataPoints"].toInt();
    _peakIndex = variantMap["PeakIndex"].toInt();

    _timeSeries.resize(numDataPoints);
    _voltageSeries.resize(numDataPoints);

    mv::util::populateBytesFromBlobMap(variantMap["TimeSeries"].toMap(), (char*)_timeSeries.data(), _timeSeries.size() * sizeof(float));
    mv::util::populateBytesFromBlobMap(variantMap["VoltageSeries"].toMap(), (char*)_voltageSeries.data(), _voltageSeries.size() * sizeof(float));
}

QVariantMap ActionPotential::toVariantMap() const
{
    QVariantMap variantMap;

    variantMap["TimeSeries"] = mv::util::bytesToBlobVariantMap((const char*)_timeSeries.data(), _timeSeries.size() * sizeof(float), mv::util::BlobStorageLocation::InlineInProjectJson);
    variantMap["VoltageSeries"] = mv::util::bytesToBlobVariantMap((const char*)_voltageSeries.data(), _voltageSeries.size() * sizeof(float), mv::util::BlobStorageLocation::InlineInProjectJson);
    variantMap["NumDataPoints"] = QVariant::fromValue(_timeSeries.size());
    variantMap["PeakIndex"] = _peakIndex;

    return variantMap;
}
