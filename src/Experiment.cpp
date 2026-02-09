#include "Experiment.h"

void Experiment::AddSweep(Sweep&& sweep)
{
    _sweeps.push_back(std::move(sweep));
}

void Experiment::setActionPotential(ActionPotential* actionPotential)
{
    _actionPotential = actionPotential;
}

std::vector<uint32_t> Experiment::getStimsetSweeps(const QString& stimset) const
{
    std::vector<uint32_t> stimSetIndices;
    for (int i = 0; i < _sweeps.size(); i++)
    {
        if (_sweeps[i].stimulus.GetStimulusDescription() == stimset)
        {
            stimSetIndices.push_back(i);
        }
    }
    return stimSetIndices;
}

std::vector<uint32_t> Experiment::GetStimTypeSweeps(StimulusType stimType) const
{
    std::vector<uint32_t> stimSetIndices;
    for (int i = 0; i < _sweeps.size(); i++)
    {
        if (_sweeps[i].stimulus.GetStimulusType() == stimType)
        {
            stimSetIndices.push_back(i);
        }
    }
    return stimSetIndices;
}

void Experiment::fromVariantMap(const QVariantMap& variantMap)
{
    mv::util::variantMapMustContain(variantMap, "sweeps");

    QVariantList sweepList = variantMap["sweeps"].toList();

    qDebug() << "Loading experiment..";
    _sweeps.resize(sweepList.size());

    for (int i = 0; i < _sweeps.size(); i++)
        _sweeps[i].fromVariantMap(sweepList[i].toMap());

    if (variantMap.contains("actionPotential"))
    {
        _actionPotential = new ActionPotential();
        _actionPotential->fromVariantMap(variantMap["actionPotential"].toMap());
    }
}

QVariantMap Experiment::toVariantMap() const
{
    QVariantMap variantMap;

    QVariantList sweepList;

    for (auto& sweep : _sweeps)
        sweepList.append(sweep.toVariantMap());

    variantMap["sweeps"] = sweepList;

    if (_actionPotential)
    {
        variantMap["actionPotential"] = _actionPotential->toVariantMap();
    }

    return variantMap;
}
