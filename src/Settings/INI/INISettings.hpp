#pragma once

namespace Settings
{
    namespace INI
    {
        template <typename T>
        concept is_numeric = (std::is_floating_point_v<T> || std::is_integral_v<T>) &&
            !std::is_same_v<T, bool>;
        
        template <typename T>
        concept is_string = std::is_same_v<T, std::string>;

        template <typename T>
        concept is_setting = std::is_same_v<T, bool> || is_numeric<T> || is_string<T>;

        class Holder final : 
            public REX::TSingleton<Holder>
        {
        public:
            [[nodiscard]] bool Initialize() noexcept;

            template <typename T> requires is_setting<T>
            [[nodiscard]]
            T GetSetting(const std::string& str, const T defaultValue) const noexcept {
                auto it = _settings.find(str);
                if (it == _settings.end()) {
                    return defaultValue;
                }
                if constexpr (std::is_same_v<bool, T>) {
                    const char ascii_lowercase = [](unsigned char c) {
                        constexpr unsigned char diff = 'a' - 'A';
                        return  (c >= 'A' && c <= 'Z') ? 
                            static_cast<char>(c + diff) :
                            static_cast<char>(c);
                    };

                    constexpr std::array<std::string_view, 2> truthy = { "true", "1" };
                    return std::ranges::any_of(truthy, [lhs = it->second](const std::string_view rhs) {
                        return std::ranges::equal(
                            lhs | std::views::transform(ascii_lowercase),
                            rhs
                        );
                    });
                }
                else if constexpr (is_numeric<T>) {
                    T res{};
                    const auto [ptr, ec] = std::from_chars(
                        it->second.data(),
                        it->second.data() + it->second.size(),
                        res
                    );

                    if (ec == std::errc{} && ptr == it->second.data() + it->second.size()) {
                        return res;
                    }
                    return defaultValue;
                }
                return it->second;
            }
        private:
            std::unordered_map<std::string, std::string> _settings{};

            void parse(const char* path) noexcept;
        };

        [[nodiscard]]
        inline static bool Initialize() noexcept {
            REX::INFO("Initializing INI settings..."sv);
            auto* holder = Holder::GetSingleton();
            if (!holder) {
                REX::ERROR("  >Failed to retrieve internal INI Settings holder."sv);
                return false;
            }
            return holder->Initialize();
        }
    }
}