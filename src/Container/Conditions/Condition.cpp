#include "Condition.hpp"

namespace Container::Conditions
{
    std::expected<Conditions, ConditionFail> create_condition(const Json::Value &val) noexcept {
        (void)val;
        return Conditions{};
    }

    void ConditionFail::Log(const std::string& prefix) const noexcept {
        (void)prefix;
    }
}