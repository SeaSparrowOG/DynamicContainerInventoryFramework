#include "Change.hpp"

namespace Container::Changes
{
    std::expected<Changes, ChangeFail> create_change(const Json::Value &val) noexcept {
        (void)val;
        return Changes{};
    }

    void ChangeFail::Log(const std::string& prefix) const noexcept {
        (void)prefix;
    }
}