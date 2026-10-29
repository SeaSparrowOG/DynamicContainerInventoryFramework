#pragma once

#include "Changes/Change.hpp"
#include "Settings/JSON/JSONSettings.hpp" // this pulls in FormUtils and json/json. This is intentional.

namespace Container
{
    class Manager final : public REX::TSingleton<Manager>
    {
    public:
        [[nodiscard]] bool Initialize() noexcept;

    private:
        [[nodiscard]] bool ProcessRule(const Json::Value& val) noexcept;
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