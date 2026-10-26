#pragma once

namespace Settings
{
    namespace JSON
    {
        class Holder final : 
            public REX::TSingleton<Holder>
        {
        public:
            [[nodiscard]] bool Initialize() noexcept;
        private:
        };

        [[nodiscard]]
        inline static bool Initialize() noexcept {
            REX::INFO("Initializing JSON settings..."sv);
            auto* holder = Holder::GetSingleton();
            if (!holder) {
                REX::CRITICAL("  >Failed to retrieve internal JSON Settings holder."sv);
                return false;
            }
            return holder->Initialize();
        }
    }
}

