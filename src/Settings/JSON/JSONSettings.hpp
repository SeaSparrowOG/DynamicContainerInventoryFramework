#pragma once

#include <glaze/glaze.hpp>

namespace Settings
{
    namespace JSON
    {
        template <typename T>
        using opt = std::optional<T>;

        template <typename T>
        using arr = std::vector<T>;

        template <typename T>
        using one_or_many = std::variant<arr<T>, T>;

        using str_or_num = std::variant<std::string, RE::FormID>;

        struct worldspace_field {
            opt<one_or_many<str_or_num>> worldspaces;
        };

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

template <>
struct glz::meta<Settings::JSON::worldspace_field>
{
    // this is the devil long may he rot in the abyss wtf is a range
    static constexpr std::string rename_key(std::string_view str) {
        constexpr unsigned char diff = 'a' - 'A';
        constexpr auto ascii_lower = [](unsigned char c) {
            return  (c >= 'A' && c <= 'Z') ?
                static_cast<char>(c + diff) :
                static_cast<char>(c);
        };
        return str |
            std::views::transform(ascii_lower) |
                std::ranges::to<std::string>();
    }
};

