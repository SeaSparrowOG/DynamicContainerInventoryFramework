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
                success &= ProcessRule(sanitized);
            }
            else if (sanitized.isArray()) {
                const auto trimTo = path.size();
                const auto size = sanitized.size();
                for (Json::ArrayIndex i = 0; i < size; ++i) {
                    path += "[" + std::to_string(i) + "]";
                    const auto& element = sanitized[i];
                    if (!element.isObject()) {
                        REX::WARN("      >Skipped {} because it isn't an object."sv, path);
                        success = false;
                        continue;
                    }
                    path.resize(trimTo);
                }
            }
            else {
                REX::WARN(fmt::format("{}"sv, sanitized.type()));
                //REX::WARN("      >Skipped. Expected an Object or Array at the top level, but got {}."sv, sanitized.type());
            }
        }
        return false;
    }

    bool Manager::ProcessRule(const Json::Value& val) noexcept {
        std::vector<std::unique_ptr<Changes::Change>>       resultingChanges{};
        std::vector<std::unique_ptr<Conditions::Condition>> resultingConditions{};

        const auto& changes    = val["changes"];
        const auto& conditions = val["conditions"];

        if (!changes) {

            return false; // missing changes
        }
        if (conditions) {
            
        }
        return false;
    }
}