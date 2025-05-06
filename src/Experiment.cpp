#include "Experiment.h"

void Experiment::addAcquisition(Recording&& recording)
{
    _acquisitions.push_back(std::move(recording));
}

void Experiment::addStimulus(Recording&& recording)
{
    _stimuli.push_back(std::move(recording));
}

void Experiment::fromVariantMap(const QVariantMap& variantMap)
{
    mv::util::variantMapMustContain(variantMap, "acquisitions");
    mv::util::variantMapMustContain(variantMap, "stimuli");

    QVariantList acquisitionList = variantMap["acquisitions"].toList();
    QVariantList stimulusList = variantMap["stimuli"].toList();
    qDebug() << "Loading experiment..";
    _acquisitions.resize(acquisitionList.size());
    _stimuli.resize(stimulusList.size());

    for (int i = 0; i < _acquisitions.size(); i++)
        _acquisitions[i].fromVariantMap(acquisitionList[i].toMap());

    for (int i = 0; i < _stimuli.size(); i++)
        _stimuli[i].fromVariantMap(stimulusList[i].toMap());
}

QVariantMap Experiment::toVariantMap() const
{
    QVariantMap variantMap;

    QVariantList acquisitionList;
    QVariantList stimulusList;

    for (auto& recording : _acquisitions)
        acquisitionList.append(recording.toVariantMap());

    for (auto& recording : _stimuli)
        stimulusList.append(recording.toVariantMap());

    variantMap["acquisitions"] = acquisitionList;
    variantMap["stimuli"] = stimulusList;

    return variantMap;
}
