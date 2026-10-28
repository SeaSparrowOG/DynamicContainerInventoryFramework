#pragma once

#include <json/json.h>

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
            void PreProcess(const std::filesystem::path& path) noexcept;
            void CanonicalizeObject(Json::Value& obj, std::string& path) noexcept;
            static void CanonicalizeKeys(Json::Value& json,
                const std::unordered_map<std::string, std::string>& mappings) noexcept;

            struct PreProcessStatus {
                [[nodiscard]]
                bool HasError() const noexcept;
                void LogError() const noexcept;

                bool opened = true;
                std::string exception{};
                std::string jsonParseErrors{};
                std::vector<std::string> multipleKeyDefinitions{};
            };
            PreProcessStatus status{};
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

