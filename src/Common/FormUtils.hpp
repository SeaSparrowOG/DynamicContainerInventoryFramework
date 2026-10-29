#pragma once

namespace FormUtils
{
    enum class FailureReason
    {
        None,
        MalformedString,
        NoFormID,
        PluginNotLoaded,
        FormNotInPlugin,
        WrongFormType,

        PO3TweaksMissing,

        GenericFailure
    };

    constexpr std::string_view failure_to_string(const FailureReason err) noexcept {
        switch (err) {
            case FailureReason::None: return "None";
            case FailureReason::MalformedString: return "MalformedString";
            case FailureReason::NoFormID: return "NoFormID";
            case FailureReason::PluginNotLoaded: return "PluginNotLoaded";
            case FailureReason::FormNotInPlugin: return "FormNotInPlugin";
            case FailureReason::WrongFormType: return "WrongFormType";
            case FailureReason::PO3TweaksMissing: return "PO3TweaksMissing";
            case FailureReason::GenericFailure: return "GenericFailure";
            default: std::unreachable();
        }
    }

    template <typename T>
    struct form_type
    {
        static constexpr RE::FormType value = RE::FormType::None;
    };

    template <>
    struct form_type<RE::BGSKeyword> { static constexpr RE::FormType value = RE::FormType::Keyword; };
    template <>
    struct form_type<RE::BGSLocationRefType> { static constexpr RE::FormType value = RE::FormType::LocationRefType; };
    template<>
    struct form_type<RE::TESGlobal> { static constexpr RE::FormType value = RE::FormType::Global; };
    template<>
    struct form_type<RE::TESRace> { static constexpr RE::FormType value = RE::FormType::Race; };
    template<>
    struct form_type<RE::TESSound> { static constexpr RE::FormType value = RE::FormType::Sound; };
    template<>
    struct form_type<RE::TESObjectCELL> { static constexpr RE::FormType value = RE::FormType::Cell; };
    template<>
    struct form_type<RE::TESWorldSpace> { static constexpr RE::FormType value = RE::FormType::WorldSpace; };
    template<>
    struct form_type<RE::TESQuest> { static constexpr RE::FormType value = RE::FormType::Quest; };
    template<>
    struct form_type<RE::TESIdleForm> { static constexpr RE::FormType value = RE::FormType::Idle; };
    template<>
    struct form_type<RE::TESObjectANIO> { static constexpr RE::FormType value = RE::FormType::AnimatedObject; };
    template<>
    struct form_type<RE::TESImageSpaceModifier> { static constexpr RE::FormType value = RE::FormType::ImageAdapter; };
    template<>
    struct form_type<RE::BGSVoiceType> { static constexpr RE::FormType value = RE::FormType::VoiceType; };
    template<>
    struct form_type<RE::BGSMusicType> { static constexpr RE::FormType value = RE::FormType::MusicType; };
    template<>
    struct form_type<RE::BGSSoundDescriptorForm> { static constexpr RE::FormType value = RE::FormType::SoundRecord; };

    constexpr bool supports_edids_without_tweaks(RE::FormType type)
    {
        switch (type) {
        case RE::FormType::Keyword:
        case RE::FormType::LocationRefType:
        case RE::FormType::Global:
        case RE::FormType::Race:
        case RE::FormType::Sound:
        case RE::FormType::Cell:
        case RE::FormType::WorldSpace:
        case RE::FormType::Quest:
        case RE::FormType::Idle:
        case RE::FormType::AnimatedObject:
        case RE::FormType::ImageAdapter:
        case RE::FormType::VoiceType:
        case RE::FormType::MusicType:
        case RE::FormType::SoundRecord:
            return true;
        default:
            return false;
        }
    }

    template <typename T>
    [[nodiscard]]
    std::expected<T*, FailureReason> get_form_from_edid(const std::string_view str) noexcept {
        constexpr bool supportsEDID = supports_edids_without_tweaks(form_type<T>::value);
        if constexpr (!supportsEDID) {
            static auto* tweaks = REX::W32::GetModuleHandleW(L"po3_Tweaks.dll");
            if (!tweaks) {
                return std::unexpected(FailureReason::PO3TweaksMissing);
            }
        }

        auto* form = RE::TESForm::LookupByEditorID(str);
        if (!form) {
            return nullptr;
        }

        T* cast = skyrim_cast<T*>(form);
        if (!cast) {
            return std::unexpected(FailureReason::WrongFormType);
        }
        return cast;
    }

    enum class ToNumFailure
    {
        EmptyString,
        InvalidString,
        NotANumber,
        OutOfRange
    };

    constexpr std::string_view failure_to_string(const ToNumFailure err) noexcept {
        switch (err) {
            case ToNumFailure::EmptyString: return "EmptyString";
            case ToNumFailure::InvalidString: return "InvalidString";
            case ToNumFailure::NotANumber: return "NotANumber";
            case ToNumFailure::OutOfRange: return "OutOfRange";
            default: std::unreachable();
        }
    }

    template <typename T>
    concept is_integral = !std::same_as<std::remove_cv_t<T>, bool> && std::is_integral_v<T>;

    template <typename T>
    concept is_floating_point = std::is_floating_point_v<T>;

    template <typename T>
    concept is_numeric = is_integral<T> || is_floating_point<T>;

    enum class base
    {
        dec,
        hex,

        generic
    };

    template <typename T, base BASE>
    concept valid_conversion = is_numeric<T> && (BASE != base::hex || is_integral<T>);

    template <typename T, base BASE = base::generic> requires valid_conversion<T, BASE>
    [[nodiscard]]
    std::expected<T, ToNumFailure> str_to_num(std::string_view str) noexcept {
        T res{};

        if (str.empty()) {
            return std::unexpected(ToNumFailure::EmptyString);
        }

        if constexpr (is_integral<T>) {
            int base = BASE == base::hex ? 16 : 10;
            if constexpr (BASE == base::generic || BASE == base::hex) {
                const bool hasPrefix = str.starts_with("0x") || str.starts_with("0X");
                if (hasPrefix) {
                    if (str.size() == 2u) {
                        return std::unexpected(ToNumFailure::InvalidString);
                    }
                    str.remove_prefix(2u);
                }

                if constexpr (BASE == base::generic) {
                    const bool isHex = hasPrefix || std::ranges::any_of(str, [](const unsigned char rhs) {
                        return (rhs >= 'a' && rhs <= 'f') || (rhs >= 'A' && rhs <= 'F'); 
                    });

                    if (isHex) {
                        base = 16;
                    }
                }
            }
            const auto [ptr, ec] = std::from_chars(
                str.data(), 
                str.data() + str.size(), 
                res, 
                base
            );

            if (ec != std::errc{} || ptr != str.data() + str.size()) {
                if (ec == std::errc::result_out_of_range) {
                    return std::unexpected(ToNumFailure::OutOfRange);
                }
                return std::unexpected(ToNumFailure::NotANumber);
            }
        }
        else {
            const auto [ptr, ec] = std::from_chars(
                str.data(), 
                str.data() + str.size(),
                res, 
                std::chars_format::general
            );

            if (ec != std::errc{} || ptr != str.data() + str.size()) {
                if (ec == std::errc::result_out_of_range) {
                    return std::unexpected(ToNumFailure::OutOfRange);
                }
                return std::unexpected(ToNumFailure::NotANumber);
            }
        }
        
        return res;
    }

    template <typename T>
    [[nodiscard]]
    std::expected<T*, FailureReason> get_form_from_string(std::string_view str) noexcept {
        if (str.empty()) {
            return std::unexpected(FailureReason::MalformedString);
        }

        const auto delimiter = std::ranges::find(str, '|');
        if (delimiter == str.end()) {
            return get_form_from_edid<T>(str);
        }

        const auto split = static_cast<size_t>(delimiter - str.begin());
        const auto first = str.substr(0u, split);
        const auto second = str.substr(split + 1);
        if (first.empty() || second.empty()) {
            return std::unexpected(FailureReason::MalformedString);
        }

        auto parsedNum = str_to_num<RE::FormID, base::hex>(second);
        const bool nameFirst = parsedNum.has_value();
        if (!parsedNum) {
            parsedNum = str_to_num<RE::FormID, base::hex>(first);
        }
        if (!parsedNum) {
            return std::unexpected(FailureReason::NoFormID);
        }

        auto* dh = RE::TESDataHandler::GetSingleton();
        if (!dh) {
            return std::unexpected(FailureReason::GenericFailure); 
        }

        const auto modName = nameFirst ? first : second;
        auto* mod = dh->LookupModByName(modName);
        if (!mod) {
            return std::unexpected(FailureReason::PluginNotLoaded);
        }

        auto* form = dh->LookupFormID(parsedNum.value(), modName);
        if (!form) {
            return std::unexpected(FailureReason::FormNotInPlugin);
        }

        auto cast = skyrim_cast<T*>(form);
        if (!cast) {
            return std::unexpected(FailureReason::WrongFormType);
        }
        return cast;
    }
}

template <>
struct fmt::formatter<FormUtils::FailureReason> {
    constexpr auto parse(format_parse_context& ctx) {
        return ctx.begin();
    }

    auto format(const FormUtils::FailureReason& val, format_context& ctx) const {
        return fmt::format_to(ctx.out(), "{}", FormUtils::failure_to_string(val));
    }
};

template <>
struct fmt::formatter<FormUtils::ToNumFailure> {
    constexpr auto parse(format_parse_context& ctx) {
        return ctx.begin();
    }

    auto format(const FormUtils::ToNumFailure& val, format_context& ctx) const {
        return fmt::format_to(ctx.out(), "{}", FormUtils::failure_to_string(val));
    }
};