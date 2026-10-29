#pragma once

#include "Common/FormUtils.hpp"

#include <json/json.h>

namespace JSONUtils {
     constexpr std::string_view json_type_to_string(const Json::ValueType type) noexcept {
        switch (type) {
            case Json::ValueType::arrayValue: return "Array";
            case Json::ValueType::intValue: return "Int";
            case Json::ValueType::booleanValue: return "Bool";
            case Json::ValueType::nullValue: return "Null";
            case Json::ValueType::objectValue: return "Object";
            case Json::ValueType::realValue: return "Float";
            case Json::ValueType::stringValue: return "String";
            case Json::ValueType::uintValue: return "Unsigned Int";
            default: std::unreachable();
        }
    }
}
template <>
struct fmt::formatter<Json::ValueType> {
    constexpr auto parse(format_parse_context& ctx) {
        return ctx.begin();
    }

    auto format(const Json::ValueType& val, format_context& ctx) const {
        return fmt::format_to(ctx.out(), "{}", JSONUtils::json_type_to_string(val));
    }
};

namespace Settings
{
    namespace JSON
    {
        class Holder final : 
            public REX::TSingleton<Holder>
        {
        public:
            [[nodiscard]] bool Initialize() noexcept;

            [[nodiscard]]
            std::map<std::string, Json::Value> GetConfigs() const noexcept { return _configs; }

        private:
            void PreProcess(const std::filesystem::path& path, const std::string& name) noexcept;
            void ProcessObject(Json::Value& obj, std::string& path) noexcept;
            static void NormalizeKeys(Json::Value& json,
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

            std::map<std::string, Json::Value> _configs{};
        };

        [[nodiscard]]
        inline bool Initialize() noexcept {
            REX::INFO("Initializing JSON settings..."sv);
            auto* holder = Holder::GetSingleton();
            if (!holder) {
                REX::CRITICAL("  >Failed to retrieve internal JSON Settings holder."sv);
                return false;
            }
            return holder->Initialize();
        }

        struct DeserializationErrors
        {
            std::vector<std::string> parseErrors{};
            std::vector<std::string> formErrors{};

            [[nodiscard]] bool HasErrors() const noexcept {
                return !parseErrors.empty() || !formErrors.empty();
            }

            void Merge(DeserializationErrors other) {
                parseErrors.insert(
                    parseErrors.end(),
                    std::make_move_iterator(other.parseErrors.begin()),
                    std::make_move_iterator(other.parseErrors.end())
                );

                formErrors.insert(
                    formErrors.end(),
                    std::make_move_iterator(other.formErrors.begin()),
                    std::make_move_iterator(other.formErrors.end())
                );
            }
        };

        template <typename T>
        [[nodiscard]]
        std::expected<std::vector<T*>, DeserializationErrors> deserialize(const Json::Value& val) noexcept {
            constexpr auto parse = [](const Json::Value& val) -> std::expected<T*, DeserializationErrors> {
                assert(val.isString());
                auto res = FormUtils::get_form_from_string<T>(val.asString());
                if (!res) {
                    auto reason = res.error();
                    DeserializationErrors errors{};
                    switch (reason) {
                        case FormUtils::FailureReason::FormNotInPlugin:
                        case FormUtils::FailureReason::PluginNotLoaded:
                        case FormUtils::FailureReason::PO3TweaksMissing:
                        case FormUtils::FailureReason::GenericFailure:
                            return nullptr; // silently skip tbh. None of those are errors, and "generic error" won't happen.
                        case FormUtils::FailureReason::WrongFormType:
                            errors.formErrors.emplace_back(
                                fmt::format<std::string>("{}: {}"sv, val.asString(), reason)
                            );
                            break;
                        case FormUtils::FailureReason::NoFormID:
                        case FormUtils::FailureReason::MalformedString:
                            errors.parseErrors.emplace_back(
                                fmt::format<std::string>("{}: {}"sv, val.asString(), reason)
                            );
                            break;
                        default: 
                            break;
                    }
                    return std::unexpected(errors);
                }
                return res.value();
            };

            assert(val.isObject() || val.isArray());
            if (val.isObject()) {
                auto res = parse(val);
                if (!res) {
                    return res.error();
                }
                if (!res.value()) {
                    return {};
                }
                return std::vector<T*>(res.value());
            }

            DeserializationErrors errors{};
            std::vector<T*> parsed{};
            parsed.reserve(val.size());
            for (const auto& element : val) {
                if (!element.isString()) {
                    continue;
                }

                auto res = parse(element);
                if (!res) {
                    errors.Merge(std::move(res.error()));
                    continue;
                }
                if (!res.value()) {
                    continue;
                }
                parsed.emplace_back(res.value());
            }

            if (errors.HasErrors()) {
                return std::unexpected(std::move(errors));
            }
            return parsed;
        }
    }
}

