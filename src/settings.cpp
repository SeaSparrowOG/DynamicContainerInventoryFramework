#include "containerManager.h"
#include "settings.h"
#include "utility.h"

namespace {
	struct SwapData {
		std::vector<std::string> missingOldField;
		std::vector<std::string> missingNewField;
		std::vector<std::string> validConfigs;
	};

	void ReadReport(SwapData a_report) {
		if (!a_report.validConfigs.empty()) {
			_loggerInfo("==============================================");
			_loggerInfo("         Reading successes report");
			_loggerInfo("==============================================");

			for (auto it : a_report.validConfigs) {
				_loggerInfo("    >{}", it);
			}
		}

		if (a_report.missingOldField.empty() && a_report.missingNewField.empty()) return;
		_loggerInfo("==============================================");
		_loggerInfo("         Reading result error report");
		_loggerInfo("==============================================");
		if (!a_report.missingOldField.empty()) {
			_loggerInfo("Missing misc item for \"old\" field:");
			for (auto it : a_report.missingOldField) {
				_loggerInfo("    >{}", it);
			}
			_loggerInfo("");
		}
		if (!a_report.missingNewField.empty()) {
			_loggerInfo("Missing misc item for \"new\" field:");
			for (auto it : a_report.missingNewField) {
				_loggerInfo("    >{}", it);
			}
			_loggerInfo("");
		}
	}
}

namespace Settings {
	bool ReadConfig(Json::Value a_config, SwapData* a_report) {

		if (!a_config.isObject()) return false;
		auto names = a_config.getMemberNames();

		for (auto name : names) {
			auto entry = a_config[name];
			std::vector<std::string> validLocationKeywords;
			std::vector<RE::BGSLocation*> validLocationIdentifiers;
			auto conditions = entry["conditions"]; //Note - Conditions are optional.
			auto changes = entry["changes"];
			if (!changes || !changes.isArray()) return false;

			if (conditions) {
				if (!conditions.isObject()) {
					continue;
				}

				//Plugins - array containing names of plugins that must be PRESENT for this config to take effect.
				bool shouldStop = false;
				auto plugins = conditions["plugins"];
				if (plugins && plugins.isArray()) {
					for (auto& plugin : plugins) {
						if (!plugin.isString()) continue;
						auto pluginString = plugin.asString();
						if (!Utility::IsModPresent(pluginString)) {
							shouldStop = true;
						}
					}
				}
				if (shouldStop) continue;

				//Locations - array containing locations.
				auto locations = conditions["locations"];
				if (locations && locations.isArray()) {
					for (auto identifier : locations) {
						if (!identifier.isString()) {
							continue;
						}

						auto components = clib_util::string::split(identifier.asString(), "|"sv);
						if (components.size() != 2) continue;
						auto* location = Utility::GetObjectFromMod<RE::BGSLocation>(components.at(0), components.at(1));

						if (!location) continue;
						validLocationIdentifiers.push_back(location);
					}
				}

				auto locationKeywords = conditions["locationKeywords"];
				if (locationKeywords && locationKeywords.isArray()) {
					for (auto identifier : locations) {
						if (!identifier.isString()) {
							continue;
						}
						validLocationKeywords.push_back(identifier.asString());
					}
				}
			}

			//Changes here.
			for (auto change : changes) {
				if (!change.isObject()) continue;

				ContainerManager::SwapRule newRule;
				newRule.validLocations = validLocationIdentifiers;
				newRule.locationKeywords = validLocationKeywords;

				auto oldId = change["old"];
				if (!oldId || !oldId.isString()) continue;
				auto oldIdString = oldId.asString();

				auto components = clib_util::string::split(oldIdString, "|"sv);
				if (components.size() != 2) continue;

				auto* oldChangeForm = Utility::GetObjectFromMod<RE::TESObjectMISC>(components.at(0), components.at(1));
				if (!oldChangeForm) {
					a_report->missingOldField.push_back(name);
					continue;
				}
				newRule.oldForm = oldChangeForm;

				auto newIdArray = change["new"];
				if (!newIdArray || !newIdArray.isArray()) continue;
				for (auto newId : newIdArray) {
					auto newIdString = newId.asString();

					auto newComponents = clib_util::string::split(newIdString, "|"sv);
					if (newComponents.size() != 2) continue;

					auto* newChangeForm = Utility::GetObjectFromMod<RE::TESObjectMISC>(newComponents.at(0), newComponents.at(1));
					if (!newChangeForm) {
						a_report->missingNewField.push_back(name);
						continue;
					}
					newRule.newForm.push_back(newChangeForm);
				}
				ContainerManager::ContainerManager::GetSingleton()->CreateSwapRule(newRule);
				a_report->validConfigs.push_back(name);
			}
		}
		return true;
	}

	bool ReadSettings() {
		std::vector<std::string> configFiles = std::vector<std::string>();
		try {
			configFiles = clib_util::distribution::get_configs(configPath, ""sv, ".json"sv);
		}
		catch (std::exception e) { //Directory not existing, for example?
			SKSE::log::warn("WARNING - caught exception {} while attempting to fetch configs.", e.what());
			return false;
		}
		if (configFiles.empty()) return true;

		SwapData report;
		for (const auto& config : configFiles) {
			try {
				std::ifstream rawJSON(config);
				Json::Reader  JSONReader;
				Json::Value   JSONFile;
				JSONReader.parse(rawJSON, JSONFile);
				ReadConfig(JSONFile, &report);
			}
			catch (std::exception e) { //Unlikely to be thrown, unless there are some weird characters involved.
				SKSE::log::warn("WARNING - caught exception {} while reading a file.", e.what());
			}
		}

		ReadReport(report);
		return true;
	}
}