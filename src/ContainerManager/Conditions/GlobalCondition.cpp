#include "GlobalCondition.h"

#include "Settings/JSON/JSONSettings.h"

namespace ContainerManager::Conditions
{
    bool GlobCondition::IsValid(const ConditionCheckParams& a_params) const {
        (void)a_params;
        const bool evalResult = std::ranges::all_of(_data, [](const auto& required) {
            return required.IsValid();
            });
        return _inverted ? !evalResult : evalResult;
    }

    void GlobCondition::PrintCondition(const std::string& a_pref) const {
        logger::info("{}Global Variable Condition:"sv, a_pref);
        for (auto it = _data.begin(); it < _data.end(); ++it) {
            logger::info("{}  - {}{}{}{}"sv, a_pref, _inverted ? "[NOT]<" : "",
                it->GetFormattedDescription(), it < _data.end() - 1 ? " [AND]" : "",
                _inverted ? ">" : "");
        }
    }

    void GlobCondition::AppendCondition(RequiredGlobals& individualData) {
        _data.emplace_back(std::move(individualData));
    }

    std::expected<condition_ptr, error_ptrs> TryCreateGlobalCondition(const Json::Value& from, 
        std::string& path, 
        bool inverted)
    {
        GlobCondition globCondition;
        globCondition.SetInverted(inverted);

        condition_ptr result = std::make_unique<GlobCondition>(globCondition);
        return result;
    }
}