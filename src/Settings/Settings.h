#pragma once

#include "INI/INISettings.hpp"
#include "JSON/JSONSettings.hpp"

namespace Settings
{
    [[nodiscard]]
    inline static bool Initialize() noexcept {
        bool success = true;
        success &= INI::Initialize();
        success &= JSON::Initialize();
        return success;
    }
}