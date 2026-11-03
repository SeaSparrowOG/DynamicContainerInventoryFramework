#include "ContainerManager.hpp"

namespace
{
    const Json::Value& remove_legacy_wrapper(const Json::Value& val) noexcept {
        if (!val.isObject()) {
            return val;
        }
        const auto& rules = val["rules"];
        if (rules && rules.isArray()) {
            return rules;
        }
        return val;
    }
}
namespace Container
{
    bool Manager::Initialize() noexcept {
        auto* settings = Settings::JSON::Holder::GetSingleton();
        if (!settings) {
            REX::CRITICAL("  - Failed to get internal JSON settings holder."sv);
            return false;
        }

        const auto& configs = settings->GetConfigs();
        if (configs.empty()) {
            REX::INFO("  - No settings."sv);
            return true;
        }

        REX::INFO("  - Parsing {} configs..."sv, configs.size());
        bool success = true;
        for (const auto& [name, config] : configs) {
            REX::INFO("    >{}"sv, name);
            // prior to V4, the format was { "rules" : [ ... ]},
            // which was stupid then and is not needed now. Yay.
            const auto& sanitized = remove_legacy_wrapper(config);

            std::string path = "Root";
            if (sanitized.isObject()) {
                success &= ProcessRule(sanitized, path);
            }
            else if (sanitized.isArray()) {
                const auto trimTo = path.size();
                const auto size = sanitized.size();
                for (Json::ArrayIndex i = 0; i < size; ++i) {
                    path.resize(trimTo);
                    path += "[" + std::to_string(i) + "]";
                    const auto& element = sanitized[i];
                    if (!element.isObject()) {
                        REX::WARN("      >Skipped {} because it isn't an object."sv, path);
                        success = false;
                        continue;
                    }
                    success &= ProcessRule(element, path);
                }
            }
            else {
                REX::WARN(fmt::format("{}"sv, sanitized.type()));
                //REX::WARN("      >Skipped. Expected an Object or Array at the top level, but got {}."sv, sanitized.type());
            }
        }
        return success;
    }

    size_t Manager::RegisterCondition(Condition condition) noexcept {
        size_t idx = _conditions.size();
        _conditions.emplace_back(std::move(condition));
        return idx;
    }

    bool Manager::ProcessRule(const Json::Value &val, std::string &path) noexcept {
        const auto& rawChanges    = val["changes"];
        if (!rawChanges) {
            REX::WARN("      (!) Couldn't find \"Changes\" field in {}. Config is invalid.", path);
            return false; // TODO: Potentially aggregate errors in conditions here?
        }

        auto parsedChanges = Changes::create_change(rawChanges);
        if (!parsedChanges) {
            REX::WARN("      (!) Failed to parse \"Changes\" field in {}. Errors:"sv);
            parsedChanges.error().Log("        ");
        }

        const auto& rawConditions = val["conditions"];
        std::vector<size_t> conditionIDs{};
        if (rawConditions) {
            if (!rawConditions.isObject()) {
                REX::WARN("      (!) \"Conditions\" is not an object, and is instead {}. Config is invalid."sv, 
                    json_type_to_string(rawConditions.type())
                );
                return false;
            }
            auto parsedConditions = Conditions::create_condition(rawConditions);
            if (!parsedConditions) {
                REX::WARN("      (!) Failed to parse \"Changes\" field in {}. Errors:"sv);
                parsedConditions.error().Log("        ");
                return false;
            }

            auto& condVec = parsedConditions.value();
            conditionIDs.reserve(condVec.size());
            for (auto& condition : condVec) {
                // This is kind of silly. I should just pass in a reference instead of casting to an rvalue.
                // It is also kind of silly that I register the condition if the change parse fails. But no harm,
                // since we terminate on failure.
                conditionIDs.emplace_back(
                    RegisterCondition(std::move(condition))
                ); 
            }
        }

        if (!parsedChanges) {
            return false;
        }

        auto& changes = parsedChanges.value();
        for (auto& change : changes) {
            const auto type = change->GetType();
            change->RegisterConditionIDs(conditionIDs);
            auto where = std::ranges::find_if(_changes, 
                [type](const auto& rhs) { return type > rhs->GetType(); }
            );
            _changes.emplace(where, std::move(change));
        }
        return true;
    }
}