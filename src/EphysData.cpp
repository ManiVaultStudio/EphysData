#include "EphysData.h"
#include "Application.h"

#include <util/Icon.h>

#include <workflow/WorkflowContextVariantMap.h>
#include <util/Serialization.h>

#include <QtCore>
#include <QPainter>

#include <set>

Q_PLUGIN_METADATA(IID "studio.manivault.EphysData")

using namespace mv;
using namespace mv::util;
using namespace mv::workflow;

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

//UniqueWorkflowPlan EphysData::fromVariantMapWorkflow(QVariantMap variantMap)
//{
//    auto plan = std::make_unique<WorkflowPlan>(__FUNCTION__);
//
//    const auto experimentList = variantMap["Experiments"].toList();
//
//    WorkflowPlan::Jobs experimentJobs;
//
//    experimentJobs.reserve(experimentList.size());
//
//    for (const auto& experiment : experimentList) {
//        const auto experimentIndex = experimentList.indexOf(experiment);
//        experimentJobs.emplace_back(WorkflowPlan::Job(QString("Load experiment %1").arg(QString::number(experimentIndex)), [this, experiment, experimentIndex](const WorkflowPlan::Job&, const SharedWorkflowExecutionContext&) {
//            _experiments[experimentIndex].fromVariantMap(experiment.toMap());
//        }, WorkflowPlan::JobThreadAffinity::CurrentWorkerThread, WorkflowPlan::JobProgressMode::Atomic));
//    }
//
//    plan->addSequentialStage("Load experiments", experimentJobs);
//
//    return plan;
//}

//UniqueWorkflowPlan EphysData::toVariantMapWorkflow() const
//{
//    auto plan = std::make_unique<WorkflowPlan>(__FUNCTION__);
//
//    const auto baseSaveStage = plan->addNestedWorkflowStage("Save raw data base", [this](const WorkflowPlan::Job&, const SharedWorkflowExecutionContext&) -> UniqueWorkflowPlan {
//        return this->Plugin::toVariantMapWorkflow();
//        });
//
//    plan->addSequentialStage("Preflight", [this](const WorkflowPlan::Job&, const SharedWorkflowExecutionContext& executionContext) {
//        if (getClusters().size() > 500000) {
//            executionContext->warning(QString("%1 clusters dataset contains approximately %2 clusters; very large numbers of clusters can take considerable time to save and load. Consider reducing the number of clusters if project serialization performance becomes a concern.").arg(getGuiName()).arg(getIntegerCountHumanReadable(getClusters().size())));
//        }
//        });
//
//    const auto serializeClustersStage = plan->addNestedWorkflowStage("Serialize clusters", [this](const WorkflowPlan::Job&, const SharedWorkflowExecutionContext&) -> UniqueWorkflowPlan {
//        return ClustersSerializer::toVariantMapWorkflow(_clusters);
//        });
//
//    plan->addSequentialStage("Save data", [this, baseSaveStage, serializeClustersStage](const WorkflowPlan::Job&, const SharedWorkflowExecutionContext& executionContext) {
//        auto outputMap = executionContext->takeOutput(baseSaveStage).toMap();
//
//        outputMap.insert(executionContext->takeOutput(serializeClustersStage).toMap());
//
//        executionContext->setOutput(outputMap);
//        });
//
//    return plan;
//}

void EphysData::fromVariantMap(const QVariantMap& variantMap)
{
    WidgetAction::fromVariantMap(variantMap);

    variantMapMustContain(variantMap, "Experiments");

    QVariantList experimentList = variantMap["Experiments"].toList();

    _experiments.resize(experimentList.size());
    for (int i = 0; i < _experiments.size(); i++)
    {
        _experiments[i].fromVariantMap(experimentList[i].toMap());
    }
}

QVariantMap EphysData::toVariantMap() const
{
    auto variantMap = WidgetAction::toVariantMap();

    QVariantList experimentList;

    for (int i = 0; i < _experiments.size(); i++)
    {
        const Experiment& experiment = _experiments[i];

        experimentList.append(experiment.toVariantMap());
    }

    variantMap["Experiments"] = experimentList;

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

    getRawData<EphysData>()->fromVariantMap(variantMap["Data"].toMap());

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
