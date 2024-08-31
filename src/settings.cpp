#include "containerManager.h"
#include "settings.h"
#include "utility.h"

namespace {
	struct SwapData {
		bool unableToOpen = false;
		bool hasError = false;
		bool hasBatData = false;
		bool missingName = false;
		bool noChanges = false;
		std::string name;
		std::string changesError;
		std::string conditionsError;
		std::string conditionsBadBypassError;
		std::string conditionsVendorsError;
		std::string conditionsPluginTypeError;
		std::vector<std::string> badStringField;
		std::vector<std::string> badObjectField;
		std::vector<std::string> missingForm;
		std::vector<std::string> objectNotArray;
		std::vector<std::string> badStringFormat;
	};

	void ReadReport(SwapData a_report) {
		if (!a_report.hasError) return;

		_loggerInfo("{} has errors:", a_report.name);
		if (a_report.hasBatData) {
			_loggerInfo("Swap has bad data.");
			_loggerInfo("");
		}

		if (a_report.missingName) {
			_loggerInfo("Missing friendlyName, or field not a string.");
			_loggerInfo("");
		}

		if (a_report.noChanges) {
			_loggerInfo("    Missing changes field, or field is invalid.");
			_loggerInfo("");
		}

		if (!a_report.badStringField.empty()) {
			_loggerInfo("The following fields are not strings when they should be:");
			for (auto it : a_report.badStringField) {
				_loggerInfo("    >{}", it);
			}
			_loggerInfo("");
		}

		if (!a_report.objectNotArray.empty()) {
			_loggerInfo("The following fields are not arrays when they should be:");
			for (auto it : a_report.objectNotArray) {
				_loggerInfo("    >{}", it);
			}
			_loggerInfo("");
		}

		if (!a_report.badStringFormat.empty()) {
			_loggerInfo("The following fields are not formatted as they should be:");
			for (auto it : a_report.badStringFormat) {
				_loggerInfo("    >{}", it);
			}
			_loggerInfo("");
		}

		if (!a_report.changesError.empty()) {
			_loggerInfo("There are no changes specified for this rule.");
			_loggerInfo("");
		}

		if (!a_report.conditionsError.empty()) {
			_loggerInfo("The condition field is unreadable, but present.");
			_loggerInfo("");
		}

		if (!a_report.conditionsBadBypassError.empty()) {
			_loggerInfo("The unsafe container bypass field is unreadable, but present.");
			_loggerInfo("");
		}

		if (!a_report.conditionsVendorsError.empty()) {
			_loggerInfo("The allow vendors field is unreadable, but present.");
			_loggerInfo("");
		}

		if (!a_report.conditionsPluginTypeError.empty()) {
			_loggerInfo("The plugins field is unreadable, but present.");
			_loggerInfo("");
		}

		if (!a_report.missingForm.empty()) {
			_loggerInfo("The following fields specify a form in a plugin that is loaded, but the form doesn't exist:");
			for (auto it : a_report.missingForm) {
				_loggerInfo("    >{}", it);
			}
			_loggerInfo("");
		}

		if (a_report.unableToOpen) {
			_loggerInfo("Unable to open the config. Make sure the JSON is valid (no missing commas, brackets are closed...)");
			_loggerInfo("");
		}
	}
}

namespace Settings {
	void ReadConfig(Json::Value a_config, std::string a_reportName, SwapData* a_report) {
		if (!a_config.isObject()) return;
		auto& rules = a_config["rules"];
		if (!rules.isArray()) return;

		for (auto& data : rules) {
			if (!data.isObject()) {
				a_report->hasBatData = true;
				a_report->hasError = true;
				continue;
			}

			auto& friendlyName = data["friendlyName"]; //Friendly Name
			auto& conditions = data["conditions"];     //Conditions
			auto& changes = data["changes"];           //Changes

			bool conditionsAreValid = true;            //If conditions are present but invalid, this stops the rule from registering.
			std::string ruleName = friendlyName.asString(); ruleName += " >> ";  ruleName += a_reportName; //Rule name. Used for logging.

			//Initializing condition results so that they may be used in changes.
			bool bypassUnsafeContainers = false;   
			bool distributeToVendors = false;
			bool onlyVendors = false;
			bool randomAdd = false;
			ContainerManager::QuestCondition questCondition{};
			std::vector<std::string> validLocationKeywords{};
			std::vector<RE::BGSLocation*> validLocationIdentifiers{};
			std::vector<RE::TESWorldSpace*> validWorldspaceIdentifiers{};
			std::vector<RE::TESObjectCONT*> validContainers{};
			std::vector<RE::FormID> validReferences{};
			std::vector<std::pair<RE::ActorValue, std::pair<float, float>>> requiredAVs{};
			std::vector<std::pair<RE::TESGlobal*, float>> requiredGlobals{};

			//Missing name check. Missing name is used in the back end for rule checking.
			if (!friendlyName || !friendlyName.isString()) {
				a_report->hasBatData = true;
				a_report->hasError = true;
				a_report->missingName = true;
				continue;
			}
			auto friendlyNameString = friendlyName.asString();

			//Missing changes check. If there are no changes, there is no need to check conditions.
			if (!changes || !changes.isArray()) {
				a_report->changesError = friendlyNameString;
				a_report->hasError = true;
				a_report->noChanges = true;
				continue;
			}

			if (conditions) {
				if (!conditions.isObject()) {
					a_report->conditionsError = friendlyNameString;
					a_report->hasError = true;
					conditionsAreValid = false;
					continue;
				}

				//Plugins check.
				//Will not read the rest of the config if at least one plugin is not present.
				auto& plugins = conditions["plugins"];
				if (plugins) {
					if (!plugins.isArray()) {
						a_report->conditionsPluginTypeError = friendlyNameString;
						a_report->hasError = true;
						conditionsAreValid = false;
						continue;
					}

					for (auto& plugin : plugins) {
						if (!plugin.isString()) {
							a_report->conditionsPluginTypeError = friendlyNameString;
							a_report->hasError = true;
							conditionsAreValid = false;
							continue;
						}

						if (!Utility::IsModPresent(plugin.asString())) {
							conditionsAreValid = false;
						}
					}
				} //End of plugins check

				//Bypass Check.
				auto& bypassField = conditions["bypassUnsafeContainers"];
				if (bypassField) {
					if (!bypassField.isBool()) {
						a_report->conditionsPluginTypeError = friendlyNameString;
						a_report->hasError = true;
						a_report->conditionsBadBypassError = true;
						conditionsAreValid = false;
						continue;
					}
					bypassUnsafeContainers = bypassField.asBool();
				}

				//Vendors Check.
				auto& vendorsField = conditions["allowVendors"];
				if (vendorsField) {
					if (!vendorsField.isBool()) {
						a_report->conditionsPluginTypeError = friendlyNameString;
						a_report->hasError = true;
						a_report->conditionsBadBypassError = true;
						conditionsAreValid = false;
						continue;
					}
					distributeToVendors = vendorsField.asBool();
				}

				//Vendors only Check.
				auto& vendorsOnlyField = conditions["onlyVendors"];
				if (vendorsOnlyField) {
					if (!vendorsOnlyField.isBool()) {
						a_report->conditionsPluginTypeError = friendlyNameString;
						a_report->hasError = true;
						a_report->conditionsBadBypassError = true;
						conditionsAreValid = false;
						continue;
					}

					if (!distributeToVendors && vendorsOnlyField.asBool()) {
						distributeToVendors = true;
						onlyVendors = true;
					}
					 else if (vendorsOnlyField.asBool()) {
						onlyVendors = true;
					}
				}

				//Random add check.
				auto& randomAddField = conditions["randomAdd"];
				if (randomAddField) {
					if (!randomAddField.isBool()) {
						a_report->conditionsPluginTypeError = friendlyNameString;
						a_report->hasError = true;
						a_report->conditionsBadBypassError = true;
						conditionsAreValid = false;
						continue;
					}
					randomAdd = randomAddField.asBool();
				}

				//Container check.
				auto& containerField = conditions["containers"];
				if (containerField) {
					if (!containerField.isArray()) {
						std::string name = friendlyNameString; name += " -> add";
						a_report->objectNotArray.push_back(name);
						a_report->hasError = true;
						conditionsAreValid = false;
						continue;
					}

					for (auto& identifier : containerField) {
						if (!identifier.isString()) {
							std::string name = friendlyNameString; name += " -> containers";
							a_report->badStringField.push_back(name);
							a_report->hasError = true;
							conditionsAreValid = false;
							continue;
						}

						auto containerID = Utility::ParseFormID(identifier.asString());
						if (containerID == 0) {
							continue;
						}

						auto* container = RE::TESForm::LookupByID<RE::TESObjectCONT>(containerID);
						if (!container) {
							std::string name = friendlyNameString; name += " -> containers -> "; name += identifier.asString();
							a_report->missingForm.push_back(name);
							a_report->hasError = true;
							conditionsAreValid = false;
							continue;
						}
						validContainers.push_back(container);
					}
				}

				//Location check.
				auto& locations = conditions["locations"];
				if (locations) {
					if (!locations.isArray()) {
						std::string name = friendlyNameString; name += " -> locations";
						a_report->objectNotArray.push_back(name);
						a_report->hasError = true;
						conditionsAreValid = false;
						continue;
					}
					for (auto& identifier : locations) {
						if (!identifier.isString()) {
							std::string name = friendlyNameString; name += " -> locations";
							a_report->badStringField.push_back(name);
							a_report->hasError = true;
							conditionsAreValid = false;
							continue;
						}

						auto locationID = Utility::ParseFormID(identifier.asString());
						if (locationID == 0) {
							continue;
						}

						auto* location = RE::TESForm::LookupByID<RE::BGSLocation>(locationID);
						if (!location) {
							std::string name = friendlyNameString; name += " -> locations -> "; name += identifier.asString();
							a_report->missingForm.push_back(name);
							a_report->hasError = true;
							conditionsAreValid = false;
							continue;
						}
						validLocationIdentifiers.push_back(location);
					}
				}

				//Location check.
				//If a location is null, it will not error.
				auto& worldspaces = conditions["worldspaces"];
				if (worldspaces) {
					if (!worldspaces.isArray()) {
						std::string name = friendlyNameString; name += " -> worldspaces";
						a_report->objectNotArray.push_back(name);
						a_report->hasError = true;
						conditionsAreValid = false;
						continue;
					}

					for (auto& identifier : worldspaces) {
						if (!identifier.isString()) {
							std::string name = friendlyNameString; name += " -> worldspaces";
							a_report->badStringField.push_back(name);
							a_report->hasError = true;
							conditionsAreValid = false;
							continue;
						}
						auto worldspaceID = Utility::ParseFormID(identifier.asString());
						if (worldspaceID == 0) {
							continue;
						}

						auto* worldspace = RE::TESForm::LookupByID<RE::TESWorldSpace>(worldspaceID);
						if (!worldspace) {
							std::string name = friendlyNameString; name += " -> locations -> "; name += identifier.asString();
							a_report->missingForm.push_back(name);
							a_report->hasError = true;
							conditionsAreValid = false;
							continue;
						}
						validWorldspaceIdentifiers.push_back(worldspace);
					}
				}

				//Location keywords.
				//No verification, for KID reasons.
				auto& locationKeywords = conditions["locationKeywords"];
				if (locationKeywords) {
					if (!locationKeywords.isArray()) {
						std::string name = friendlyNameString; name += " -> locationKeywords";
						a_report->objectNotArray.push_back(name);
						a_report->hasError = true;
						conditionsAreValid = false;
						continue;
					}

					for (auto& identifier : locationKeywords) {
						if (!identifier.isString()) {
							std::string name = friendlyNameString; name += " -> locationKeywords";
							a_report->badStringField.push_back(name);
							a_report->hasError = true;
							conditionsAreValid = false;
							continue;
						}
						validLocationKeywords.push_back(identifier.asString());
					}
				}

				//Reference check.
				auto& referencesField = conditions["references"];
				if (referencesField && referencesField.isArray()) {
					for (auto& identifier : referencesField) {
						if (!identifier.isString()) {
							std::string name = friendlyNameString; name += " -> references";
							a_report->badStringField.push_back(name);
							a_report->hasError = true;
							conditionsAreValid = false;
							continue;
						}

						auto components = clib_util::string::split(identifier.asString(), "|"sv);
						if (components.size() != 2) {
							std::string name = friendlyNameString; name += " -> references -> "; name += identifier.asString();
							a_report->badStringFormat.push_back(name);
							a_report->hasError = true;
							conditionsAreValid = false;
							continue;
						}

						if (!Utility::IsModPresent(components.at(1))) continue;
						validReferences.push_back(Utility::ParseFormID(identifier.asString()));
					}
				}

				//Player skill check.
				auto& playerSkillsField = conditions["playerSkills"];
				if (playerSkillsField) {
					if (!playerSkillsField.isArray()) {
						std::string name = friendlyNameString; name += " -> playerSkills";
						a_report->objectNotArray.push_back(name);
						a_report->hasError = true;
						conditionsAreValid = false;
						continue;
					}

					for (auto& identifier : playerSkillsField) {
						if (!identifier.isString()) {
							std::string name = friendlyNameString; name += " -> playerSkills";
							a_report->badStringField.push_back(name);
							a_report->hasError = true;
							conditionsAreValid = false;
							continue;
						}

						auto components = clib_util::string::split(identifier.asString(), "|"sv);
						if (components.size() != 2 && components.size() != 3) {
							std::string name = friendlyNameString; name += " -> playerSkills -> "; name += identifier.asString();
							a_report->badStringFormat.push_back(name);
							a_report->hasError = true;
							conditionsAreValid = false;
							continue;
						}

						//Format is: 1 Skill, 2 min level, 3 (optional) max level. Valid skills are HARDCODED skill values, such as conjuration and one-handed.
						auto skillName = clib_util::string::tolower(components.at(0));
						auto skillLevelMin = components.at(1);
						std::string skillLevelMax = "";
						if (components.size() == 3) skillLevelMax = components.at(2);

						float skillNumLevel = -1.0f;
						float skillNumLevelMax = -1.0f;
						try {
							skillNumLevel = std::stof(skillLevelMin);
						}
						catch (std::exception e) {
							_loggerError("Exception {} caught while reading config: {} -> playerSkills. Make sure everything is formatted correctly.", e.what(), friendlyNameString);
							continue;
						}

						if (!skillLevelMax.empty()) {
							try {
								skillNumLevelMax = std::stof(skillLevelMax);
							}
							catch (std::exception e) {
								_loggerError("Exception {} caught while reading config: {} -> playerSkills. Make sure everything is formatted correctly.", e.what(), friendlyNameString);
								continue;
							}
						}

						auto requiredAV = RE::ActorValue::kAggression;
						if (skillName == "illusion") {
							requiredAV = RE::ActorValue::kIllusion;
						}
						else if (skillName == "conjuration") {
							requiredAV = RE::ActorValue::kConjuration;
						}
						else if (skillName == "destruction") {
							requiredAV = RE::ActorValue::kDestruction;
						}
						else if (skillName == "restoration") {
							requiredAV = RE::ActorValue::kRestoration;
						}
						else if (skillName == "alteration") {
							requiredAV = RE::ActorValue::kAlteration;
						}
						else if (skillName == "enchanting") {
							requiredAV = RE::ActorValue::kEnchanting;
						}
						else if (skillName == "smithing") {
							requiredAV = RE::ActorValue::kSmithing;
						}
						else if (skillName == "heavyarmor") {
							requiredAV = RE::ActorValue::kHeavyArmor;
						}
						else if (skillName == "block") {
							requiredAV = RE::ActorValue::kBlock;
						}
						else if (skillName == "twohanded") {
							requiredAV = RE::ActorValue::kTwoHanded;
						}
						else if (skillName == "onehanded") {
							requiredAV = RE::ActorValue::kOneHanded;
						}
						else if (skillName == "marksman") {
							requiredAV = RE::ActorValue::kArchery;
						}
						else if (skillName == "lightarmor") {
							requiredAV = RE::ActorValue::kLightArmor;
						}
						else if (skillName == "sneak") {
							requiredAV = RE::ActorValue::kSneak;
						}
						else if (skillName == "speechcraft") {
							requiredAV = RE::ActorValue::kSpeech;
						}
						else if (skillName == "lockpicking") {
							requiredAV = RE::ActorValue::kLockpicking;
						}
						else if (skillName == "pickpocket") {
							requiredAV = RE::ActorValue::kPickpocket;
						}
						else if (skillName == "alchemy") {
							requiredAV = RE::ActorValue::kAlchemy;
						}

						if (requiredAV == RE::ActorValue::kAggression) {
							std::string name = friendlyNameString; name += " -> playerSkills -> "; name += identifier.asString();
							a_report->missingForm.push_back(name);
							continue;
						}
						auto floatPair = std::make_pair(skillNumLevel, skillNumLevelMax);
						auto newPair = std::make_pair(requiredAV, floatPair);
						requiredAVs.push_back(newPair);
					}
				}

				//Global check. Note: has optimization that can be done. Check at the end.
				auto& globalsField = conditions["globals"];
				if (globalsField) {
					if (!globalsField.isArray()) {
						std::string name = friendlyNameString; name += " -> globals";
						a_report->objectNotArray.push_back(name);
						a_report->hasError = true;
						conditionsAreValid = false;
						continue;
					}

					for (auto& identifier : globalsField) {
						if (!identifier.isString()) {
							std::string name = friendlyNameString; name += " -> globals";
							a_report->badStringField.push_back(name);
							a_report->hasError = true;
							conditionsAreValid = false;
							continue;
						}

						auto components = clib_util::string::split(identifier.asString(), "|"sv);
						if (components.size() != 2) {
							std::string name = friendlyNameString; name += " -> globals -> "; name += identifier.asString();
							a_report->badStringFormat.push_back(name);
							a_report->hasError = true;
							conditionsAreValid = false;
							continue;
						}

						//Format is: 1 Global EDID, 2 Rerquired val (float). This will FLOOR the value if the global needs a long/short.
						auto globalName = clib_util::string::tolower(components.at(0));
						auto globalValueStr = components.at(1);
						float globalValueFloat = -1.0f;

						try {
							globalValueFloat = std::stof(globalValueStr) / 1.0f;
						}
						catch (std::exception e) {
							_loggerError("Exception {} caught while reading config: {} -> globals. Make sure everything is formatted correctly.", e.what(), friendlyNameString);
							continue;
						}

						auto* requiredGlobal = RE::TESForm::LookupByEditorID<RE::TESGlobal>(globalName);
						if (!requiredGlobal) {
							std::string name = friendlyNameString; name += " -> globals -> "; name += globalName;
							a_report->missingForm.push_back(name);
							continue;
						}
						
						//Potential optimization: Constant form flag evaluated HERE, not in container manager. How do I get constant flag?
						auto newPair = std::make_pair(requiredGlobal, globalValueFloat);
						requiredGlobals.push_back(newPair);
					}
				}

				//Quest condition check.
				auto& questConditionField = conditions["questConditions"];
				if (questConditionField) {
					if (!questConditionField.isObject()) {
						std::string name = friendlyNameString; name += " -> questConditions";
						a_report->badObjectField.push_back(name);
						a_report->hasError = true;
						conditionsAreValid = false;
						continue;
					}

					if (!questConditionField["questID"] || !questConditionField["questID"].isString()) {
						continue;
					}

					if (!(questConditionField["stageDone"] || questConditionField["completed"])) {
						continue;
					}

					if (auto& stageDoneField = questConditionField["stageDone"]) {
						if (!stageDoneField.isArray()) {
							continue;
						}

						bool shouldSkip = false;
						for (auto element : stageDoneField) {
							if (!element.isUInt64()) {
								shouldSkip = true;
							}
						}

						if (shouldSkip) {
							continue;
						}
					}

					if (auto& completedField = questConditionField["completed"]) {
						if (!completedField.isBool()) continue;
					}

					ContainerManager::QuestCondition::QuestCompletion completion = 
						ContainerManager::QuestCondition::QuestCompletion::kIgnored;
					std::string questName = "";
					std::vector<uint16_t> sortedStages{};

					if (auto& stageDoneField = questConditionField["stageDone"]) {
						for (auto& element : stageDoneField) {
							uint16_t stageNum = element.asUInt64();
							sortedStages.push_back(stageNum);
						}
					}

					if (auto& compltedField = questConditionField["completed"]) {
						bool check = compltedField.asBool();
						if (check) {
							completion =
								ContainerManager::QuestCondition::QuestCompletion::kCompleted;
						}
						else {
							completion =
								ContainerManager::QuestCondition::QuestCompletion::kOngoing;
						}
					}

					std::sort(sortedStages.begin(), sortedStages.end());
					questName = questConditionField["questID"].asString();

					questCondition.questEDID = questName;
					questCondition.requiredStages = sortedStages;
					questCondition.questCompleted = completion;
				}
			} //End of conditions check
			if (!conditionsAreValid) continue;

			//Changes tracking here.
			for (auto& change : changes) {
				ContainerManager::SwapRule newRule;
				newRule.bypassSafeEdits = bypassUnsafeContainers;
				newRule.randomAdd = randomAdd;
				newRule.allowVendors = distributeToVendors;
				newRule.onlyVendors = onlyVendors;
				newRule.questCondition = questCondition;
				newRule.validLocations = validLocationIdentifiers;
				newRule.validWorldspaces = validWorldspaceIdentifiers;
				newRule.locationKeywords = validLocationKeywords;
				newRule.container = validContainers;
				newRule.references = validReferences;
				newRule.ruleName = ruleName;
				newRule.requiredAVs = requiredAVs;
				newRule.requiredGlobalValues = requiredGlobals;

				bool changesAreValid = true;

				auto& oldId = change["remove"];
				auto& newId = change["add"];
				auto& countField = change["count"];
				auto& removeKeywords = change["removeByKeywords"];
				if (!(oldId || newId || removeKeywords)) {
					a_report->hasBatData = true;
					a_report->changesError = friendlyNameString;
					changesAreValid = false;
					continue;
				}

				if (oldId) {
					if (!oldId.isString()) {
						std::string name = friendlyNameString; name += " -> remove";
						a_report->badStringField.push_back(name);
						a_report->hasError = true;
						changesAreValid = false;
						continue;
					}
					
					auto parsedFormID = Utility::ParseFormID(oldId.asString());
					if (parsedFormID == 0) {
						continue;
					}

					auto* parsedForm = RE::TESForm::LookupByID<RE::TESBoundObject>(parsedFormID);
					if (!parsedForm) continue;
					newRule.oldForm = parsedForm;
				} //Old object check.

				if (newId) {
					if (!newId.isArray()) {
						std::string name = friendlyNameString; name += " -> add";
						a_report->objectNotArray.push_back(name);
						a_report->hasError = true;
						changesAreValid = false;
						continue;
					}

					for (auto& addition : newId) {
						if (!addition.isString()) {
							std::string name = friendlyNameString; name += " -> add";
							a_report->badStringField.push_back(name);
							a_report->hasError = true;
							changesAreValid = false;
							continue;
						}

						auto parsedFormID = Utility::ParseFormID(addition.asString());
						if (parsedFormID == 0) {
							continue;
						}

						auto* parsedForm = RE::TESForm::LookupByID<RE::TESBoundObject>(parsedFormID);
						if (!parsedForm) continue;
						newRule.newForm.push_back(parsedForm);
					}
				} //New ID Check

				if (removeKeywords) {
					if (!removeKeywords.isArray()) {
						a_report->conditionsPluginTypeError = friendlyNameString;
						a_report->hasError = true;
						conditionsAreValid = false;
						changesAreValid = false;
						continue;
					}

					std::vector<std::string> validRemoveKeywords{};
					for (auto& removeKeyword : removeKeywords) {
						if (!removeKeyword.isString()) {
							std::string name = friendlyNameString; name += " -> removeByKeywords";
							a_report->badStringField.push_back(name);
							a_report->hasError = true;
							changesAreValid = false;
							continue;
						}
						validRemoveKeywords.push_back(removeKeyword.asString());
					}
					newRule.removeKeywords = validRemoveKeywords;
				}
				if (!changesAreValid) continue;

				if (countField && countField.isInt()) {
					newRule.count = countField.asInt();
				}
				else {
					newRule.count = -1;
				}

				ContainerManager::ContainerManager::GetSingleton()->CreateSwapRule(newRule);
			} //End of changes check
		} //End of data loop
	} //End of read config

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

		std::vector<SwapData> reports;
		for (const auto& config : configFiles) {
			try {
				std::ifstream rawJSON(config);
				Json::Reader  JSONReader;
				Json::Value   JSONFile;
				JSONReader.parse(rawJSON, JSONFile);

				SwapData  report;
				std::string configName = config.substr(config.rfind("/") + 1, config.length() - 1);
				report.name = configName;
				ReadConfig(JSONFile, configName, &report);
				reports.push_back(report);
			}
			catch (std::exception e) { //Unlikely to be thrown, unless there are some weird characters involved.
				SKSE::log::warn("WARNING - caught exception {} while reading a file.", e.what());
			}
		}

		for (auto& report : reports) {
			ReadReport(report);
		}
		return true;
	}
}