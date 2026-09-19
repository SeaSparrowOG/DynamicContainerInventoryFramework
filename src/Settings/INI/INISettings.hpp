#pragma once

namespace Settings
{
    namespace INI
    {
#define INT_INDEX static_cast<std::size_t>(2u)
#define BOOL_INDEX static_cast<std::size_t>(3u)
#define FLOAT_INDEX static_cast<std::size_t>(1u)
#define STRING_INDEX static_cast<std::size_t>(0u)

        using Type = std::variant<std::string, float, int, bool>;
        using Setting = std::optional<Type>;

        [[nodiscard]] bool Load();

        class SettingsHolder final : 
            public REX::TSingleton<SettingsHolder>
        {
        public:
            void                  Print() const;
            void                  Clear() { _settings.clear(); }
            bool                  Reload();
            void                  OverrideSettings();
            [[nodiscard]] bool    LoadSettings();
            [[nodiscard]] Setting GetSetting(const std::string& str) const;

        private:
            std::unordered_map<std::string, Setting> _settings{};
        };

        [[nodiscard]] inline static Setting GetSetting(const std::string& str) 
        {
            static const auto* settings = SettingsHolder::GetSingleton();
            if (!settings) {
                REX::ERROR("Requested setting {}, but internal settings holder is null."sv, str);
                return std::nullopt;
            }
            return settings->GetSetting(str);
        }
    }
}