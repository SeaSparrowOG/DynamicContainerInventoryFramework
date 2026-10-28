#include "JSONSettings.hpp"

namespace {
    [[nodiscard]]
    static std::string ascii_lowercase(const std::string_view str) noexcept {
        constexpr unsigned char diff = 'a' - 'A';
        std::string result(str);
        std::ranges::transform(result, result.begin(), [](unsigned char chr) {
            return chr >= 'A' && chr <= 'Z' ?
            static_cast<char>(chr + diff) :
            static_cast<char>(chr);
        });
        return result;
    }
}

namespace Settings::JSON
{
    bool Holder::Initialize() noexcept {
        std::string jsonFolder = fmt::format(R"(.\Data\SKSE\Plugins\{})"sv, Plugin::NAME);
        REX::INFO("  - Settings folder: {}."sv, jsonFolder);
        if (!std::filesystem::exists(jsonFolder) ||
            !std::filesystem::is_directory(jsonFolder)) {
            REX::INFO("    >No settings found - folder was missing."sv);
            return true;
        }
        if (jsonFolder.empty()) {
            REX::INFO("    >No settings found within settings folder."sv);
            return true;
        }
        std::vector<std::filesystem::path> paths{};
        const auto trimFrom = jsonFolder.size() - 1u;
        paths.reserve(16u); // this is a reasonable size start, source is "trust me".

        for (const auto& it : std::filesystem::directory_iterator(jsonFolder)) {
            const auto& path = it.path();
            std::string name = path.string().substr(trimFrom);
            if (!it.is_regular_file()) {
                REX::WARN("    >Discarding {} - not a file."sv, name);
                continue;
            }
            if (ascii_lowercase(path.extension().string()) != ".json") {
                REX::WARN("    >Discarding {} - not a json file.", name);
                continue;
            }
            const auto where = std::ranges::find_if(paths, [name](const auto& rhs) {
               return rhs.string() > name;
            });
            paths.emplace(where, path);
        }

        if (paths.empty()) {
            REX::INFO("  - No configuration files found."sv);
            return true;
        }

        bool success = true;
        const auto fileCount = paths.size();
        REX::INFO("  - Found {} configuration files", fileCount);
        for (const auto& path : paths) {
            std::string name = path.string().substr(trimFrom);
            REX::INFO("    >Pre-processing {}..."sv, name);
            PreProcess(path);
            if (status.HasError()) {
                status.LogError();
                success = false;
            }
        }
        return success;
    }

    void Holder::PreProcess(const std::filesystem::path& path) noexcept {
        status = PreProcessStatus();
        Json::CharReaderBuilder builder;
        builder["collectComments"] = false;

        try {
            std::ifstream raw(path);
            if (!raw.is_open()) {
                status.opened = false;
                return;
            }

            Json::Value val{};
            std::string errs{};
            if (!Json::parseFromStream(builder, raw, &val, &errs)) {
                status.jsonParseErrors = errs;
                return;
            }

            std::string pathAsString = "Root";
            pathAsString.reserve(256u);
            const auto trimTo = pathAsString.size();
            if (val.isObject()) {
                CanonicalizeObject(val, pathAsString);
            }
            else if (val.isArray()) {
                auto elements = std::views::iota(Json::ArrayIndex{0u}, val.size()) |
                    std::views::filter([&](const Json::ArrayIndex i) { return val[i].isObject(); });
                std::ranges::for_each(elements, [&](const Json::ArrayIndex i) {
                    pathAsString += "[" + std::to_string(i) + "]";
                    CanonicalizeObject(val[i], pathAsString);
                    pathAsString.resize(trimTo);
                });
            }
        }
        catch (const Json::Exception& e) {
            status.exception = std::string(e.what());
            return;
        }
        catch (const std::exception& e) {
            status.exception = std::string(e.what());
            return;
        }
        catch (...) {
            status.exception = "Unhandled unknown exception."sv;
            return;
        }
    }

    void Holder::CanonicalizeObject(Json::Value& obj, std::string& path) noexcept {
        assert(obj.isObject());
        const auto keys = obj.getMemberNames();
        const auto trimTo = path.size();
        std::unordered_map<std::string, std::string> mappings{};
        for (const auto& key : keys) {
            path += " -> " + key;
            const auto lower = ascii_lowercase(key);
            if (mappings.contains(lower)) {
                status.multipleKeyDefinitions.emplace_back(path);
                path.resize(trimTo);
                continue;
            }
            mappings[lower] = key;

            auto& val = obj[key];
            if (val.isObject()) {
                CanonicalizeObject(val, path);
            }
            else if (val.isArray()) {
                const auto arrayTrim = path.size();
                auto elements = std::views::iota(Json::ArrayIndex{0u}, val.size()) |
                    std::views::filter([&](const Json::ArrayIndex i) { return val[i].isObject(); });
                std::ranges::for_each(elements, [&](const Json::ArrayIndex i) {
                    path += "[" + std::to_string(i) + "]";
                    CanonicalizeObject(val[i], path);
                    path.resize(arrayTrim);
                });
            }
            path.resize(trimTo);
        }
        CanonicalizeKeys(obj, mappings);
    }

    void Holder::CanonicalizeKeys(Json::Value &json,
        const std::unordered_map<std::string, std::string> &mappings) noexcept
    {
        assert(json.isObject());
        for (const auto& [to, from] : mappings) {
            if (from == to) {
                continue;
            }
            assert(!json.isMember(to));
            json[to] = std::move(json[from]);
        }
    }

    bool Holder::PreProcessStatus::HasError() const noexcept {
        return !opened ||
            !exception.empty() ||
                !jsonParseErrors.empty() ||
                    !multipleKeyDefinitions.empty();
    }

    void Holder::PreProcessStatus::LogError() const noexcept {
        if (!opened) {
            REX::WARN("      - Failed to open file."sv);
            return;
        }
        if (!exception.empty()) {
            REX::WARN("      - Encountered this exception when parsing the file: {}", exception);
        }
        if (!multipleKeyDefinitions.empty()) {
            REX::WARN("      - The following keys were defined more than once:"sv);
            for (const auto& key : multipleKeyDefinitions) {
                REX::WARN("        >{}"sv, key);
            }
        }
    }
}
