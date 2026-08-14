#pragma once

#include "EphysData_export.h"

#include "Sweep.h"
#include "ActionPotential.h"

#include <util/Serializable.h>

#include <vector>
#include <string>
#include <memory>

class EPHYSDATA_EXPORT Experiment : public mv::util::Serializable
{
public:
    std::string GetName() { return _name; }
    void SetName(std::string name) { _name = name; }

    const std::vector<Sweep>& GetSweeps() const { return _sweeps; }
    std::vector<Sweep>& GetSweeps() { return _sweeps; }

    const ActionPotential* getActionPotential() const { return _actionPotential; }

    void AddSweep(Sweep&& sweep);

    void setActionPotential(ActionPotential* actionPotential);

    std::vector<uint32_t> getStimsetSweeps(const QString& stimset) const;
    std::vector<uint32_t> GetStimTypeSweeps(StimulusType stimType) const;

public: // Serialization
    void fromVariantMap(const QVariantMap& variantMap) override;
    QVariantMap toVariantMap() const override;

private:
    std::string _name;

    std::vector<Sweep> _sweeps;

    ActionPotential* _actionPotential = nullptr;
};
