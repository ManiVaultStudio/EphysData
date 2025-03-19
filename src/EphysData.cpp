#include "EphysData.h"
#include "Application.h"

#include <util/Icon.h>

#include <util/Serialization.h>

#include <QtCore>
#include <QPainter>

#include <set>

Q_PLUGIN_METADATA(IID "studio.manivault.EphysData")

using namespace mv;
using namespace mv::util;

EphysData::~EphysData(void)
{
    
}

void EphysData::init()
{
}

/**
 * Create a new dataset linking back to the original raw data
 */
Dataset<DatasetImpl> EphysData::createDataSet(const QString& guid /*= ""*/) const
{
    return new EphysExperiments(getName(), guid);
}

std::vector<Experiment>& EphysData::getData()
{
    return _experiments;
}

void EphysData::setCellIdentifiers(const QStringList& cellIds)
{
    _cellIds = cellIds;
}

//void EphysData::setData(const std::vector<Experiment>& experiments)
//{
//    _experiments = experiments;
//}

void EphysData::addExperiment(Experiment&& experiment)
{
    _experiments.push_back(std::move(experiment));
}

void EphysData::fromVariantMap(const QVariantMap& variantMap)
{
    WidgetAction::fromVariantMap(variantMap);
}

QVariantMap EphysData::toVariantMap() const
{
    auto variantMap = WidgetAction::toVariantMap();

    return variantMap;
}

EphysExperiments::EphysExperiments(QString dataName, const QString& guid /*= ""*/) :
    DatasetImpl(dataName, true, guid)
{
    setIcon(mv::util::StyledIcon(QIcon(":/ephys_data/Icon_64.png")));
}

EphysExperiments::~EphysExperiments()
{
}

Dataset<DatasetImpl> EphysExperiments::createSubsetFromSelection(const QString& guiName, const Dataset<DatasetImpl>& parentDataSet /*= Dataset<DatasetImpl>()*/, const bool& visible /*= true*/) const
{
    return mv::data().createSubsetFromSelection(getSelection<EphysExperiments>(), const_cast<EphysExperiments*>(this), guiName, parentDataSet, visible);
}

Dataset<DatasetImpl> EphysExperiments::copy() const
{
    auto copySet = new EphysExperiments(getRawDataName());

    copySet->_indices = _indices;

    return copySet;
}

void EphysExperiments::setCellIdentifiers(const QStringList& cellIds)
{
    getRawData<EphysData>()->setCellIdentifiers(cellIds);
}

//void EphysExperiments::setData(const std::vector<Experiment>& cellMorphologies)
//{
//    getRawData<EphysData>()->setData(cellMorphologies);
//}

void EphysExperiments::addExperiment(Experiment&& experiment)
{
    getRawData<EphysData>()->addExperiment(std::move(experiment));
}

std::vector<std::uint32_t>& EphysExperiments::getSelectionIndices()
{
    return getSelection<EphysExperiments>()->_indices;
}

void EphysExperiments::setSelectionIndices(const std::vector<std::uint32_t>& indices)
{
    getSelection<EphysExperiments>()->_indices = indices;
}

bool EphysExperiments::canSelect() const
{
    return getRawData<EphysData>()->getData().size() >= 1;
}

bool EphysExperiments::canSelectAll() const
{
    return canSelect() && (getSelectionSize() < getRawData<EphysData>()->getData().size());
}

bool EphysExperiments::canSelectNone() const
{
    return canSelect() && (getSelectionSize() >= 1);
}

bool EphysExperiments::canSelectInvert() const
{
    return canSelect();
}

void EphysExperiments::selectAll()
{
    // Get reference to selection indices
    auto& selectionIndices = getSelectionIndices();

    // Clear and resize
    selectionIndices.clear();
    selectionIndices.resize(getRawData<EphysData>()->getData().size());

    // Generate cluster selection indices
    std::iota(selectionIndices.begin(), selectionIndices.end(), 0);

    // Notify others that the selection changed
    events().notifyDatasetDataSelectionChanged(this);
}

void EphysExperiments::selectNone()
{
    // Clear selection indices
    getSelectionIndices().clear();

    // Notify others that the selection changed
    events().notifyDatasetDataSelectionChanged(this);
}

void EphysExperiments::selectInvert()
{
    // Get reference to selection indices
    auto& selectionIndices = getSelectionIndices();

    // Create set of selected indices
    std::set<std::uint32_t> selectionSet(selectionIndices.begin(), selectionIndices.end());

    // Get number of items
    const auto numberOfItems = getRawData<EphysData>()->getData().size();

    // Clear and resize
    selectionIndices.clear();
    selectionIndices.reserve(numberOfItems - selectionSet.size());

    // Do the inversion
    for (std::uint32_t i = 0; i < numberOfItems; i++) {
        if (selectionSet.find(i) == selectionSet.end())
            selectionIndices.push_back(i);
    }

    // Notify others that the selection changed
    events().notifyDatasetDataSelectionChanged(this);
}

void EphysExperiments::fromVariantMap(const QVariantMap& variantMap)
{
    DatasetImpl::fromVariantMap(variantMap);

    getRawData<EphysData>()->fromVariantMap(variantMap);

    events().notifyDatasetDataChanged(this);
}

QVariantMap EphysExperiments::toVariantMap() const
{
    auto variantMap = DatasetImpl::toVariantMap();

    variantMap["Data"] = getRawData<EphysData>()->toVariantMap();

    return variantMap;
}

plugin::RawData* EphysDataFactory::produce()
{
    return new EphysData(this);
}
