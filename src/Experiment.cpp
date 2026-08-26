#include "Experiment.h"

#include "MemoryPool.h"

#include <util/Serialization.h>

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
    mv::util::variantMapMustContain(variantMap, "Name");
    mv::util::variantMapMustContain(variantMap, "sweeps");
    mv::util::variantMapMustContain(variantMap, "TimeSeriesMemoryPool");
    mv::util::variantMapMustContain(variantMap, "TimeSeriesMemoryPoolSize");

    size_t memoryPoolSize = variantMap["TimeSeriesMemoryPoolSize"].toInt();
    MemoryPool::Instance().Resize(memoryPoolSize);
    mv::util::populateBytesFromBlobMap(variantMap["TimeSeriesMemoryPool"].toMap(), (char*)MemoryPool::Instance().GetData().data(), memoryPoolSize * sizeof(float));

    _name = variantMap["Name"].toString().toStdString();
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

    MemoryPool::Instance().Clear();
}

QVariantMap Experiment::toVariantMap() const
{
    QVariantMap variantMap;

    variantMap["Name"] = QString::fromStdString(_name);

    QVariantList sweepList;

    for (auto& sweep : _sweeps)
        sweepList.append(sweep.toVariantMap());

    variantMap["sweeps"] = sweepList;

    if (_actionPotential)
    {
        variantMap["actionPotential"] = _actionPotential->toVariantMap();
    }

    variantMap["TimeSeriesMemoryPool"] = mv::util::bytesToBlobVariantMap((const char*)MemoryPool::Instance().GetData().data(), MemoryPool::Instance().Size() * sizeof(float));
    variantMap["TimeSeriesMemoryPoolSize"] = (int) MemoryPool::Instance().Size();
    MemoryPool::Instance().Clear();

    return variantMap;
}
