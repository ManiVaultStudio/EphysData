#pragma once

#include "EphysData_export.h"
#include "Experiment.h"

#include "RawData.h"
#include "Set.h"

#include <QImage>
#include <QString>
#include <QColor>

const mv::DataType EphysType = mv::DataType(QString("EphysData"));

// =============================================================================
// Raw Data
// =============================================================================

class EPHYSDATA_EXPORT EphysData : public mv::plugin::RawData
{
public:
    EphysData(const mv::plugin::PluginFactory* factory) : RawData(factory, EphysType) { }
    ~EphysData(void) override;

    void init() override;

    /**
     * Create dataset for raw data
     * @param guid Globally unique dataset identifier (use only for deserialization)
     * @return Smart pointer to dataset
     */
    mv::Dataset<mv::DatasetImpl> createDataSet(const QString& guid = "") const override;

    QStringList& getCellIdentifiers() { return _cellIds; }
    std::vector<Experiment>& getData();

    void setCellIdentifiers(const QStringList& cellIds);
    void setData(const std::vector<Experiment>& cellMorphologies);

public: // Serialization

    /**
     * Load data plugin from variant
     * @param Variant representation of the widget action
     */
    void fromVariantMap(const QVariantMap& variantMap) override;

    /**
     * Save data plugin to variant
     * @return Variant representation of the widget action
     */
    QVariantMap toVariantMap() const override;

private:
    QStringList _cellIds;

    std::vector<Experiment> _experiments;       /** Data store of the plugin, this is the core of your plugin */
};

// =============================================================================
// Data Set
// =============================================================================

class EPHYSDATA_EXPORT EphysExperiments : public mv::DatasetImpl
{
public:
    EphysExperiments(QString dataName, const QString& guid = "");
    ~EphysExperiments() override;

    /**
     * Create subset and specify where the subset will be placed in the data hierarchy
     * @param guiName Name of the subset in the GUI
     * @param parentDataSet Smart pointer to parent dataset in the data hierarchy (default is below the set)
     * @param visible Whether the subset will be visible in the UI
     * @return Smart pointer to the created subset
     */
    mv::Dataset<mv::DatasetImpl> createSubsetFromSelection(const QString& guiName, const mv::Dataset<mv::DatasetImpl>& parentDataSet = mv::Dataset<mv::DatasetImpl>(), const bool& visible = true) const override;

    /** Mandatory override for copying of data sets */
    mv::Dataset<mv::DatasetImpl> copy() const override;

    QStringList& getCellIdentifiers() { return getRawData<EphysData>()->getCellIdentifiers(); }
    std::vector<Experiment>& getData() { return getRawData<EphysData>()->getData(); }

    void setCellIdentifiers(const QStringList& cellIds);

    void setData(const std::vector<Experiment>& cellMorphologies);

    /**
     * Get set icon
     * @param color Icon color for flat (font) icons
     * @return Icon
     */
    QIcon getIcon(const QColor& color = Qt::black) const override;

public: // Selection

    /**
     * Get selection indices
     * @return Selection indices
     */
    std::vector<std::uint32_t>& getSelectionIndices() override;

    /**
     * Select by indices
     * @param indices Selection indices
     */
    void setSelectionIndices(const std::vector<std::uint32_t>& indices) override;

    /** Determines whether items can be selected */
    bool canSelect() const override;

    /** Determines whether all items can be selected */
    bool canSelectAll() const override;

    /** Determines whether there are any items which can be deselected */
    bool canSelectNone() const override;

    /** Determines whether the item selection can be inverted (more than one) */
    bool canSelectInvert() const override;

    /** Select all items */
    void selectAll() override;

    /** Deselect all items */
    void selectNone() override;

    /** Invert item selection */
    void selectInvert() override;

public: // Serialization

    /**
     * Load widget action from variant
     * @param Variant representation of the widget action
     */
    void fromVariantMap(const QVariantMap& variantMap) override;

    /**
        * Save widget action to variant
        * @return Variant representation of the widget action
        */
    QVariantMap toVariantMap() const override;

protected:
    std::vector<unsigned int>   _indices;      /** Indices into the raw data, if this dataset is just a subset */
};

// =============================================================================
// Factory
// =============================================================================

class EphysDataFactory : public mv::plugin::RawDataFactory
{
    Q_INTERFACES(mv::plugin::RawDataFactory mv::plugin::PluginFactory)
        Q_OBJECT
        Q_PLUGIN_METADATA(IID   "studio.manivault.EphysData"
            FILE  "EphysData.json")

public:
    EphysDataFactory() {}
    ~EphysDataFactory() override {}

    /**
     * Get plugin icon
     * @param color Icon color for flat (font) icons
     * @return Icon
     */
    QIcon getIcon(const QColor& color = Qt::black) const override;

    mv::plugin::RawData* produce() override;
};
