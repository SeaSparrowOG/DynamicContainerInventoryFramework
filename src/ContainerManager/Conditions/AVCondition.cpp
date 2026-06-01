#include "AVCondition.h"

#include "Settings/JSON/JSONSettings.h"

namespace ContainerManager::Conditions
{
    static std::optional<RE::ActorValue> GetAVFromString(const std::string& str) {

        auto* avl = RE::ActorValueList::GetSingleton();
        auto av = avl ? avl->LookupActorValueByName(str.c_str()) : RE::ActorValue::kNone;
        if (av == RE::ActorValue::kNone) {
            return std::nullopt;
        }
        return av;
    }

    bool AVCondition::IsValid(const ConditionCheckParams& a_params) const {
        auto* owner = a_params.playerOwner;
        const bool evalResult = std::ranges::all_of(_data, [&](const auto& required) {
            return required.IsValid(owner);
            });
        return _inverted ? !evalResult : evalResult; //xor
    }

    void AVCondition::PrintCondition(const std::string& a_pref) const {
        logger::info("{}AV Condition:"sv, a_pref);
        for (auto it = _data.begin(); it < _data.end(); ++it) {
            logger::info("{}  - {}{}{}{}"sv, a_pref, _inverted ? "[NOT]<" : "", 
                it->GetFormattedDescription(), it < _data.end() - 1 ? " [AND]" : "",
                _inverted ? ">" : "");
        }
    }

    void AVCondition::AppendCondition(Required& individualData) {
        _data.emplace_back(std::move(individualData));
    }

    std::expected<condition_ptr, error_ptrs> TryCreateAVCondition(const Json::Value& from, 
        std::string& path,
        bool inverted)
    {
        AVCondition avCondition;
        avCondition.SetInverted(inverted);

        condition_ptr result = std::make_unique<AVCondition>(avCondition);
        return result;
    }
}