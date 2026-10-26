#include "INISettings.hpp"

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include "ClibUtil/simpleINI.hpp" // this devil pulls in windows.h.
#undef ERROR

namespace Settings::INI
{
    bool Holder::Initialize() noexcept {
        const std::string path = fmt::format(R"(.\Data\SKSE\Plugins\{}.ini)"sv, Plugin::NAME);
        const std::string customPath = fmt::format(R"(.\Data\SKSE\Plugins\{}_Custom.ini)"sv, Plugin::NAME);
        REX::INFO("Looking for:"sv);
        REX::INFO("  - {}"sv, path);
        REX::INFO("  - {}", customPath);

        if (!std::filesystem::exists(path) || !std::filesystem::is_regular_file(path)) {
            REX::WARN("Couldn't find regular INI. Default values will be used."sv);
            return true;
        }
        try {
            parse(path.data());
        }
        catch (std::exception& e) {
            REX::ERROR("Failed to read INI. Error: {} - Default settings will be used."sv, e.what());
            _settings.clear();
            return true;
        }

        if (!std::filesystem::exists(path) || !std::filesystem::is_regular_file(path)) {
            REX::ERROR("Couldn't find regular INI. Default values will be used."sv);
            return true;
        }
        try {
            parse(customPath.data());
        }
        catch (std::exception& e) {
            REX::WARN("Failed to read Custom INI. Error: {}."sv, e.what());
            return true;
        }
        return true;
    }

    void Holder::parse(const char* path) noexcept {
        CSimpleIniA ini{};
		ini.SetUnicode();
		ini.LoadFile(path);

		std::list<CSimpleIniA::Entry> sections{};
		ini.GetAllSections(sections);

		std::string setting{};
		std::list<CSimpleIniA::Entry> sectionKeys{};

		for (const auto& section : sections) {
			ini.GetAllKeys(section.pItem, sectionKeys);
			for (const auto& key : sectionKeys) {
				setting = fmt::format<std::string>("{}|{}"sv, section.pItem, key.pItem);
				_settings[setting] = ini.GetValue(section.pItem, key.pItem);
			}
		}
    }
}