#include "JSONSettings.h"

namespace Settings::JSON
{
    inline static constexpr std::size_t MAX_RECURSION_DEPTH = 16;
    static_assert(MAX_RECURSION_DEPTH <= std::numeric_limits<std::uint8_t>::max() && MAX_RECURSION_DEPTH > 4u);

    class ParseError : public std::runtime_error
    {
    public:
        ParseError(std::string a_reason) : std::runtime_error(a_reason) {}
    };

    class DuplicateKeyException : public ParseError
    {
    public:
        DuplicateKeyException(std::string a_field) : ParseError(fmt::format("Duplicate Key in: {}", a_field)) {}
    };

    class MaxRecursionExceededException : public ParseError
    {
    public:
        MaxRecursionExceededException(std::string a_path) : ParseError(fmt::format("Max Recursion Depth exceeded at: {}", a_path)) {}
    };

    class EmptyKeyException : public ParseError
    {
    public:
        EmptyKeyException(std::string a_path) : ParseError(fmt::format("Empty Key found at: {}", a_path)) {}
    };

    class InvalidKeyException : public ParseError
    {
    public:
        InvalidKeyException(std::string a_type) : ParseError(fmt::format("Invalid JSON structure. Top level object is: {}", a_type)) {}
    };

    static std::vector<std::string> MakeKeysLowercase(Json::Value& a_obj, std::vector<std::string>& a_pathParts) {
        assert(a_obj.isObject());
        auto members = a_obj.getMemberNames();
        std::vector<std::string> lowercaseMembers{};
        lowercaseMembers.reserve(members.size());

        for (const auto& member : members) {
            auto lowercase = clib_util::string::tolower(member);
            if (member != lowercase && a_obj.isMember(lowercase)) {
                auto path = std::string("");
                for (const auto& part : a_pathParts) {
                    path.append(part);
                }
                throw DuplicateKeyException(fmt::format("{}|{}", path, member));
            }
            if (lowercase != member) {
                auto value = std::move(a_obj[member]);
                a_obj.removeMember(member);
                a_obj[lowercase] = std::move(value);
            }
            lowercaseMembers.emplace_back(lowercase);
        }
        return lowercaseMembers;
    }

    static void Canonicalize_Object(Json::Value& a_obj, std::vector<std::string>& a_pathParts, std::size_t depth = 0u) {
        assert(a_obj.isObject());

        depth++;
        if (depth > MAX_RECURSION_DEPTH) {
            auto path = std::string("");
            for (const auto& part : a_pathParts) {
                path.append(part);
            }
            throw MaxRecursionExceededException(path);
        }

        auto members = MakeKeysLowercase(a_obj, a_pathParts);
        for (const auto& member : members) {
            auto& val = a_obj[member];
            if (val.isObject()) {
                a_pathParts.push_back(fmt::format("|{}", member));
                Canonicalize_Object(val, a_pathParts, depth);
                a_pathParts.pop_back();
            }
            else if (val.isArray()) {
                auto end = val.size();
                for (unsigned int index = 0u; index < end; ++index) {
                    auto& valElement = val[index];
                    if (valElement.isObject()) {
                        a_pathParts.push_back(fmt::format("[{}]", index));
                        Canonicalize_Object(valElement, a_pathParts, depth);
                        a_pathParts.pop_back();
                    }
                }
            }
        }
    }

    static void Canonicalize(Json::Value& a_val) {
        if (a_val.isObject()) {
            if (a_val.empty()) {
                throw EmptyKeyException("Root");
            }

            auto path = std::vector<std::string>({ "Root" });
            Canonicalize_Object(a_val, path);
        }
        else if (a_val.isArray()) {
            if (a_val.empty()) {
                throw EmptyKeyException("Root[]");
            }

            for (unsigned int index = 0u; index < a_val.size(); ++index) {
                auto& arrayVal = a_val[index];
                if (arrayVal.empty()) {
                    throw EmptyKeyException(fmt::format("Root[{}]", index));
                }
                auto pathParts = std::vector<std::string>({ fmt::format("Root[{}]", index) });
                Canonicalize_Object(arrayVal, pathParts);
            }
        }
        else {
            throw InvalidKeyException(fmt::format("{}", GetFieldType(a_val)));
        }
    }

	bool Read() {
        logger::info("Reading configuration files..."sv);
		std::string directory = R"(Data/SKSE/Plugins/ContainerDistributionFramework)";
        std::string_view extension = ".json"sv;
		std::vector<std::string> jsonFilePaths{};

        auto* configHolder = ConfigHolder::GetSingleton();
        if (!configHolder) {
            logger::critical("  Failed to get internal config holder."sv);
            return false;
        }

        try {
            for (const auto& entry : std::filesystem::directory_iterator(directory)) {
                if (entry.is_regular_file() && entry.path().extension() == extension) {
                    jsonFilePaths.push_back(entry.path().string());
                }
            }
            std::sort(jsonFilePaths.begin(), jsonFilePaths.end());
            logger::info("Finished collecting {} files:"sv, jsonFilePaths.size());
        }
        catch (const std::filesystem::filesystem_error& e) {
            logger::critical("  Caught filesystem exception while gathering configuration files: {}"sv, e.what());
            return false;
        }
        catch (const std::exception& e) {
            logger::critical("  Caught exception while gathering configuration files: {}"sv, e.what());
            return false;
        }
        catch (...) {
            logger::critical("  Caught unexpected exception while gathering configuration files."sv);
            return false;
        }

        Json::Reader JSONReader;
        Json::Value JSONFile;
        auto trimFrom = directory.size() + 1u;
        bool errored = false;

        logger::info("Beginning parsing..."sv);
        try {
            for (const auto& path : jsonFilePaths) {
                auto trimTo = path.size() - extension.size();
                auto configName = path.substr(trimFrom, trimTo);
                std::ifstream rawJSON(path);
                Json::CharReaderBuilder builder;
                std::string errs;

                logger::info("  Parsing {}"sv, configName);
                if (!Json::parseFromStream(builder, rawJSON, &JSONFile, &errs)) {
                    configHolder->AddFailedConfig(configName, errs);
                    errored = true;
                    continue;
                }
                try {
                    Canonicalize(JSONFile);
                    configHolder->AddGoodConfig(configName, JSONFile);
                }
                catch (const ParseError& e) {
                    configHolder->AddFailedConfig(configName, e.what());
                    errored = true;
                    continue;
                }
                catch (const Json::Exception& e) {
                    configHolder->AddFailedConfig(configName, e.what());
                    errored = true;
                    continue;
                }
            }
        }
        catch (const std::exception& e) {
            logger::critical("    Caught unexpected exception of type: {}"sv, e.what());
            return false;
        }
        catch (...) {
            logger::critical("  >Unexpected exception caught."sv);
            SKSE::stl::report_and_fail(
                fmt::format("Caught unexpected error while parsing a file. Check the log at My Games/Skyrim Special Edition/SKSE/ContainerDistributionFramework.log for more information. You can open this with Notepad."sv)
            ); // Likely bad enough that we need to scream IMMEDIATELY.
        }
        configHolder->Report();
		return !errored;
	}

	std::string GetFieldType(const Json::Value& a_field) {
		auto type = a_field.type();
		switch (type) {
		case Json::ValueType::arrayValue: return "Array";
		case Json::ValueType::booleanValue: return "Boolean";
		case Json::ValueType::intValue: return "Integer";
		case Json::ValueType::nullValue: return "Null";
		case Json::ValueType::objectValue: return "Object";
		case Json::ValueType::realValue: return "Float";
		case Json::ValueType::stringValue: return "String";
		case Json::ValueType::uintValue: return "Unsigned Integer";
		default: return "Undefined";
		}
	}

    void ConfigHolder::AddFailedConfig(const std::string& configName, const std::string& a_reason) {
        FailedConfig failure{};
        failure.config = configName;
        failure.reason = a_reason;
        failedConfigs.emplace_back(std::move(failure));
    }

    void ConfigHolder::AddGoodConfig(const std::string& a_configName, Json::Value a_value) {
        configs.emplace(a_configName, std::move(a_value));
    }

    void ConfigHolder::Report() const
    {
        if (configs.empty()) {
            logger::info("  No configs loaded."sv);
        }
        else {
            logger::info("  Successfully parsed:"sv);
            for (const auto& [config, value] : configs) {
                (void)value;
                logger::info("    >{}"sv, config);
            }
        }
        if (!failedConfigs.empty()) {
            if (failedConfigs.size() > 1) {
                logger::error("  Some configs failed to load:");
            }
            else {
                logger::error("  This config failed to load:");
            }
            for (const auto& failure : failedConfigs) {
                failure.PrintReason();
            }
        }
    }

    void ConfigHolder::FailedConfig::PrintReason(std::string a_prefix) const {
        logger::error("{}>{}:"sv, a_prefix, config);
        logger::error("{}  {}"sv, a_prefix, reason);
    }
}