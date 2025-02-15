#include "Experiment.h"

void Experiment::addAcquisition(Recording&& recording)
{
    _acquisitions.push_back(std::move(recording));
}

void Experiment::addStimulus(Recording&& recording)
{
    _stimuli.push_back(std::move(recording));
}
