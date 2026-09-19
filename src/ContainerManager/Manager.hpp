#pragma once

#include "Rules/Rules.hpp"

namespace ContainerManager
{
    class Manager final : 
        public REX::TSingleton<Manager>
    {
    public:
        [[nodiscard]] bool Initialize();
    };

    [[nodiscard]] inline static bool Initialize() {
        REX::INFO("Initializing Container Manager..."sv);
        auto* manager = Manager::GetSingleton();
        if (!manager) {
            REX::CRITICAL("  - Failed to fetch internal Manager."sv);
            return false;
        }
        return manager->Initialize();
    }
}