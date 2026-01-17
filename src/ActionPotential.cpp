#include "ActionPotential.h"

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
    mv::util::variantMapMustContain(variantMap, "timeSeries");
    mv::util::variantMapMustContain(variantMap, "voltageSeries");
    mv::util::variantMapMustContain(variantMap, "peakIndex");

    QVariantList timeSeriesList = variantMap["timeSeries"].toList();
    QVariantList voltageSeriesList = variantMap["voltageSeries"].toList();
    _peakIndex = variantMap["peakIndex"].toInt();
    qDebug() << "Loading action potential..";
    _timeSeries.resize(timeSeriesList.size());
    _voltageSeries.resize(voltageSeriesList.size());

    for (int i = 0; i < _timeSeries.size(); i++)
        _timeSeries[i] = timeSeriesList[i].toFloat();

    for (int i = 0; i < _voltageSeries.size(); i++)
        _voltageSeries[i] = voltageSeriesList[i].toFloat();
}

QVariantMap ActionPotential::toVariantMap() const
{
    QVariantMap variantMap;

    QVariantList timeSeriesList;
    QVariantList voltageSeriesList;

    for (auto& x : _timeSeries)
        timeSeriesList.append(x);

    for (auto& y : _voltageSeries)
        voltageSeriesList.append(y);

    variantMap["timeSeries"] = timeSeriesList;
    variantMap["voltageSeries"] = voltageSeriesList;
    variantMap["peakIndex"] = _peakIndex;

    return variantMap;
}
