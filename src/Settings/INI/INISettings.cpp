#include "INISettings.hpp"

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include "ClibUtil/simpleINI.hpp"
#undef ERROR
#undef NOMINMAX
#undef WIN32_LEAN_AND_MEAN

namespace Settings::INI
{
    bool Load() {
        REX::INFO("Loading INI settings..."sv);

        auto* holder = SettingsHolder::GetSingleton();
        if (!holder) {
            REX::ERROR("  - Failed to fetch internal Settings Holder."sv);
            return false;
        }
        return holder->LoadSettings();
    }

    void SettingsHolder::Print() const {
        if (_settings.empty()) {
            REX::INFO("No settings."sv);
            return;
        }
        REX::INFO("Stored {} settings:"sv, _settings.size());
        for (const auto& [key, val] : _settings) {
            if (!val.has_value()) {
                REX::WARN("  >{} is invalid."sv, key);
                continue;
            }
            const auto& actualValue = val.value();
            const auto index = actualValue.index();
            if (index == BOOL_INDEX) {
                REX::INFO("  >{}: {} [Bool]"sv, key, std::get<bool>(actualValue));
            }
            else if (index == INT_INDEX) {
                REX::INFO("  >{}: {} [Int]"sv, key, std::get<int>(actualValue));
            }
            else if (index == FLOAT_INDEX) {
                REX::INFO("  >{}: {} [Float]"sv, key, std::get<float>(actualValue));
            }
            else if (index == STRING_INDEX) {
                REX::INFO("  >{}: {} [String]"sv, key, std::get<std::string>(actualValue));
            }
        }
    }

    bool SettingsHolder::Reload() {
        Clear();
        return true;
    }

    void SettingsHolder::OverrideSettings() {
        REX::INFO("  - Checking for overrides..."sv);
        std::string iniPath = fmt::format(R"(.\Data\SKSE\Plugins\{}_custom.ini)"sv, Plugin::NAME);
		CSimpleIniA ini{};
		size_t settingCount = 0;

        if (!std::filesystem::exists(iniPath)) {
			REX::INFO("    >Custom INI not found."sv);
			return;
		}

		try {
			ini.SetUnicode();
			ini.LoadFile(iniPath.data());

			std::list<CSimpleIniA::Entry> sections{};
			ini.GetAllSections(sections);

			if (sections.empty()) {
				return;
			}

			for (const auto& section : sections) {
				std::list<CSimpleIniA::Entry> sectionKeys{};
				ini.GetAllKeys(section.pItem, sectionKeys);

				if (sectionKeys.empty()) {
					continue;
				}

				settingCount += sectionKeys.size();
				for (const auto& key : sectionKeys) {
					const std::string foundSetting = fmt::format<std::string>("{}|{}"sv, section.pItem, key.pItem);

					const auto settingKeyName = std::string(key.pItem);
					const auto settingType = settingKeyName.substr(0, 1);
                    Setting value = std::nullopt;
					if (settingType == "s") {
						value = ini.GetValue(section.pItem, key.pItem);
					}
					else if (settingType == "f") {
						const double raw = ini.GetDoubleValue(section.pItem, key.pItem);
						value = raw > std::numeric_limits<float>::max() ?
							std::numeric_limits<float>::max() :
							raw < std::numeric_limits<float>::lowest() ?
							std::numeric_limits<float>::lowest() :
							static_cast<float>(raw);
					}
					else if (settingType == "b") {
						value = ini.GetBoolValue(section.pItem, key.pItem);
					}
					else if (settingType == "i") {
						value = ini.GetLongValue(section.pItem, key.pItem);
					}
					else {
						continue;
					}
                    _settings[foundSetting] = std::move(value);
				}
			}
		}
		catch (std::exception& e) {
			REX::ERROR("Caught exception {} while fetching INI settings.", e.what());
		}
    }

    bool SettingsHolder::LoadSettings() {
        Clear();

		std::string iniPath = fmt::format(R"(.\Data\SKSE\Plugins\{}.ini)"sv, Plugin::NAME);
		CSimpleIniA ini{};
		size_t settingCount = 0;
		REX::INFO("  - Reading and validating INI settings from {}.ini"sv, Plugin::NAME);

		try {
			ini.SetUnicode();
			ini.LoadFile(iniPath.data());

			std::list<CSimpleIniA::Entry> sections{};
			ini.GetAllSections(sections);

			if (sections.empty()) {
				return true;
			}

			for (const auto& section : sections) {
				std::list<CSimpleIniA::Entry> sectionKeys{};
				ini.GetAllKeys(section.pItem, sectionKeys);

				if (sectionKeys.empty()) {
					REX::WARN("    >INI section {} has no settings.", section.pItem);
					continue;
				}

				settingCount += sectionKeys.size();
				for (const auto& key : sectionKeys) {
					const std::string foundSetting = fmt::format<std::string>("{}|{}"sv, section.pItem, key.pItem);
                    if (_settings.contains(foundSetting)) {
                        continue;
                    }

					const auto settingKeyName = std::string(key.pItem);
					const auto settingType = settingKeyName.substr(0, 1);
                    Setting value = std::nullopt;
					if (settingType == "s") {
						value = ini.GetValue(section.pItem, key.pItem);
					}
					else if (settingType == "f") {
						const double raw = ini.GetDoubleValue(section.pItem, key.pItem);
						value = raw > std::numeric_limits<float>::max() ?
							std::numeric_limits<float>::max() :
							raw < std::numeric_limits<float>::lowest() ?
							std::numeric_limits<float>::lowest() :
							static_cast<float>(raw);
					}
					else if (settingType == "b") {
						value = ini.GetBoolValue(section.pItem, key.pItem);
					}
					else if (settingType == "i") {
						value = ini.GetLongValue(section.pItem, key.pItem);
					}
					else {
						REX::ERROR("    >Invalid setting {}. Settings must be prefixed by s, f, b, or i."sv, foundSetting);
					}
                    _settings[foundSetting] = std::move(value);
				}
			}
		}
		catch (std::exception& e) {
			REX::ERROR("Caught exception {} while fetching INI settings.", e.what());
			return false;
		}

		REX::INFO("    >Finished reading {} settings.", std::to_string(settingCount));

		OverrideSettings();
		Print();
        return true;
    }

    Setting SettingsHolder::GetSetting(const std::string& str) const
    {
        auto it = _settings.find(str);
        if (it == _settings.end()) {
            return std::nullopt;
        }
        return it->second;
    }
}