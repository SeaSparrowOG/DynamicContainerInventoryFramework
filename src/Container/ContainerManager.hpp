#pragma once

#include "Changes/Change.hpp"

namespace Container
{
    class Manager final : public REX::TSingleton<Manager>
    {
    public:
        [[nodiscard]] bool Initialize() noexcept;

    private:
        using Condition = std::unique_ptr<Conditions::Condition>;
        using Change = std::unique_ptr<Changes::Change>;
        [[nodiscard]] size_t RegisterCondition(Condition condition) noexcept;
        [[nodiscard]] bool ProcessRule(const Json::Value& val, std::string& path) noexcept;

        std::vector<Condition> _conditions{};
        std::vector<Change>    _changes{};
    };

    [[nodiscard]]
    inline bool Initialize() noexcept {
        REX::INFO("Initializing Container Manager..."sv);
        auto* manager = Manager::GetSingleton();
        if (!manager) {
            REX::CRITICAL("  - Failed to retrieve internal Conainer Manager singleton."sv);
            return false;
        }
        return manager->Initialize();
    }
}