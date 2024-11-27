#include "JSONSettings.h"

#include "hooks/hooks.h"
#include "utilities/utilities.h"

#include "conditions/actorValueCondition.h"
#include "conditions/containerCondition.h"
#include "conditions/globalCondition.h"
#include "conditions/locationCondition.h"
#include "conditions/locationKeywordCondition.h"
#include "conditions/questCondition.h"
#include "conditions/referenceCondition.h"
#include "conditions/worldspaceCondition.h"

namespace Settings::JSON
{
	static std::vector<std::string> findJsonFiles()
	{
		static constexpr std::string_view directory = R"(Data/SKSE/Plugins/ContainerDistributionFramework)";
		std::vector<std::string> jsonFilePaths;
		for (const auto& entry : std::filesystem::directory_iterator(directory)) {
			if (entry.is_regular_file() && entry.path().extension() == ".json") {
				jsonFilePaths.push_back(entry.path().string());
			}
		}

		std::sort(jsonFilePaths.begin(), jsonFilePaths.end());
		return jsonFilePaths;
	}

	void ReadConfig(Json::Value& a_config, std::string& a_path) {
		auto& rules = a_config["rules"];
		if (!rules || !rules.isArray()) return;

		for (auto& data : rules) {
			auto& friendlyName = data["friendlyName"];
			if (!friendlyName || !friendlyName.isString()) {
				logger::warn("Config <{}> is missing friendly name, or friendly name is not a string.", a_path);
				return;
			}
			auto& conditions = data["conditions"];
			auto& changes = data["changes"];
			if (!changes || !changes.isArray()) {
				logger::warn("Config <{}>/[{}] is either missing the changes field, or it is not an array.", a_path, friendlyName.asString());
				return;
			}

			const auto dataHandler = RE::TESDataHandler::GetSingleton();
			assert(dataHandler);
			if (!dataHandler) {
				logger::critical("FAILED TO GET DATA HANDLER, YOU WILL PROBABLY CRASH.");
				return;
			}

			//Initializing condition results so that they may be used in changes.
			bool bypassUnsafeContainers = false;
			bool distributeToVendors = false;
			bool onlyVendors = false;
			bool randomAdd = false;
			std::vector<std::unique_ptr<Conditions::Condition>> newConditions{};

			struct QuestStruct {
				bool completed = false;
				std::vector<uint16_t> completedStages;
				RE::TESQuest* quest;
			};
			std::vector<QuestStruct> requiredQuests{};

			if (conditions) {
				//Plugins Check
				auto& plugins = conditions["plugins"];
				if (plugins) {
					if (!plugins.isArray()) {
						logger::warn("Config <{}>/[{}] has plugins specified, but plugins are not an array. Config will be ignored.", a_path, friendlyName.asString());
						return;
					}

					for (auto& plugin : plugins) {
						if (!plugin.isString()) {
							logger::warn("Config <{}>/[{}] has plugins specified, and a plugin is not a string. Config will be ignored.", a_path, friendlyName.asString());
							return;
						}

						if (!dataHandler->LookupModByName(plugin.asString())) {
							logger::info("Note that config <{}>/[{}] requires mod {} to work, which is not present.", a_path, friendlyName.asString(), plugin.asString());
							return;
						}
					}
				} // End of plugins

				//Bypass Check.
				auto& bypassField = conditions["bypassUnsafeContainers"];
				if (bypassField) {
					if (!bypassField.isBool()) {
						logger::warn("Config <{}>/[{}] has bypassUnsafeContainers specified, but it is not a bool value. Config will be ignored.", a_path, friendlyName.asString());
						return;
					}
					bypassUnsafeContainers = bypassField.asBool();
				}

				//Vendors Check.
				auto& vendorsField = conditions["allowVendors"];
				if (vendorsField) {
					if (!vendorsField.isBool()) {
						logger::warn("Config <{}>/[{}] has allowVendors specified, but it is not a bool value. Config will be ignored.", a_path, friendlyName.asString());
						return;
					}
					distributeToVendors = vendorsField.asBool();
				}

				//Vendors only Check.
				auto& vendorsOnlyField = conditions["onlyVendors"];
				if (vendorsOnlyField) {
					if (!vendorsOnlyField.isBool()) {
						logger::warn("Config <{}>/[{}] has onlyVendors specified, but it is not a bool value. Config will be ignored.", a_path, friendlyName.asString());
						return;
					}

					if (!distributeToVendors && vendorsOnlyField.asBool()) {
						distributeToVendors = true;
						onlyVendors = true;
					}
					else if (vendorsOnlyField.asBool()) {
						onlyVendors = true;
					}
				}

				//Random add
				auto& randomAddField = conditions["randomAdd"];
				if (randomAddField) {
					if (!randomAddField.isBool()) {
						logger::warn("Config <{}>/[{}] has randomAdd specified, but it is not a bool value. Config will be ignored.", a_path, friendlyName.asString());
						return;
					}
					randomAdd = randomAddField.asBool();
				}
				
				//Container check
				auto& containerField = conditions["containers"];
				auto& reverseContainersField = conditions["!containers"];
				if (containerField) {
					if (!containerField.isArray()) {
						logger::warn("Config <{}>/[{}] has containers specified, but it is not an array value. Config will be ignored.", a_path, friendlyName.asString());
						return;
					}

					std::vector<RE::TESObjectCONT*> validContainers{};
					for (auto& identifier : containerField) {
						if (!identifier.isString()) {
							logger::warn("Config <{}>/[{}] has containers specified, but an element is not a string. Config will be ignored.", a_path, friendlyName.asString());
							return;
						}

						const auto container = Utilities::Forms::GetFormFromString<RE::TESObjectCONT>(identifier.asString());
						if (!container) {
							logger::info("Config <{}>/[{}] requires container {}, but it is not present. This is not fatal.", a_path, friendlyName.asString(), identifier.asString());
							continue;
						}
						validContainers.push_back(container);
					}
					Conditions::ContainerCondition newCondition{ validContainers };
					newCondition.inverted = false;
					auto newConditionPtr = std::make_unique<Conditions::ContainerCondition>(newCondition);
					newConditions.push_back(std::move(newConditionPtr));
				}

				if (reverseContainersField) {
					if (!reverseContainersField.isArray()) {
						logger::warn("Config <{}>/[{}] has containers specified, but it is not an array value. Config will be ignored.", a_path, friendlyName.asString());
						return;
					}

					std::vector<RE::TESObjectCONT*> validContainers{};
					for (auto& identifier : reverseContainersField) {
						if (!identifier.isString()) {
							logger::warn("Config <{}>/[{}] has containers specified, but an element is not a string. Config will be ignored.", a_path, friendlyName.asString());
							return;
						}

						const auto container = Utilities::Forms::GetFormFromString<RE::TESObjectCONT>(identifier.asString());
						if (!container) {
							logger::info("Config <{}>/[{}] requires container {}, but it is not present. This is not fatal.", a_path, friendlyName.asString(), identifier.asString());
							continue;
						}
						validContainers.push_back(container);
					}
					Conditions::ContainerCondition newCondition{ validContainers };
					newCondition.inverted = true;
					auto newConditionPtr = std::make_unique<Conditions::ContainerCondition>(newCondition);
					newConditions.push_back(std::move(newConditionPtr));
				}

				//Location check
				auto& locations = conditions["locations"];
				auto& reverseLocations = conditions["!locations"];
				if (locations) {
					if (!locations.isArray()) {
						logger::warn("Config <{}>/[{}] has locations specified, but it is not an array value. Config will be ignored.", a_path, friendlyName.asString());
						return;
					}

					std::vector<RE::BGSLocation*> validLocationIdentifiers{};
					for (auto& identifier : locations) {
						if (!identifier.isString()) {
							logger::warn("Config <{}>/[{}] has locations specified, but an element is not a string. Config will be ignored.", a_path, friendlyName.asString());
							return;
						}

						const auto location = Utilities::Forms::GetFormFromString<RE::BGSLocation>(identifier.asString());
						if (!location) {
							logger::info("Config <{}>/[{}] requires location {}, but it is not present. This is not fatal.", a_path, friendlyName.asString(), identifier.asString());
							continue;
						}
						validLocationIdentifiers.push_back(location);
					}

					Conditions::LocationCondition newCondition{ validLocationIdentifiers };
					newCondition.inverted = false;
					auto newConditionPtr = std::make_unique<Conditions::LocationCondition>(newCondition);
					newConditions.push_back(std::move(newConditionPtr));
				}

				if (reverseLocations) {
					if (!locations.isArray()) {
						logger::warn("Config <{}>/[{}] has locations specified, but it is not an array value. Config will be ignored.", a_path, friendlyName.asString());
						return;
					}

					std::vector<RE::BGSLocation*> validLocationIdentifiers{};
					for (auto& identifier : locations) {
						if (!identifier.isString()) {
							logger::warn("Config <{}>/[{}] has locations specified, but an element is not a string. Config will be ignored.", a_path, friendlyName.asString());
							return;
						}

						const auto location = Utilities::Forms::GetFormFromString<RE::BGSLocation>(identifier.asString());
						if (!location) {
							logger::info("Config <{}>/[{}] requires location {}, but it is not present. This is not fatal.", a_path, friendlyName.asString(), identifier.asString());
							continue;
						}
						validLocationIdentifiers.push_back(location);
					}

					Conditions::LocationCondition newCondition{ validLocationIdentifiers };
					newCondition.inverted = true;
					auto newConditionPtr = std::make_unique<Conditions::LocationCondition>(newCondition);
					newConditions.push_back(std::move(newConditionPtr));
				}

				//Worldspace check
				auto& worldspaces = conditions["worldspaces"];
				auto& reverseWorldspaces = conditions["!worldspaces"];
				if (worldspaces) {
					if (!worldspaces.isArray()) {
						logger::warn("Config <{}>/[{}] has worldspaces specified, but it is not an array value. Config will be ignored.", a_path, friendlyName.asString());
						return;
					}

					std::vector<RE::TESWorldSpace*> validWorldspaceIdentifiers{};
					for (auto& identifier : worldspaces) {
						if (!identifier.isString()) {
							logger::warn("Config <{}>/[{}] has worldspaces specified, but an element is not a string. Config will be ignored.", a_path, friendlyName.asString());
							return;
						}

						const auto worldspace = Utilities::Forms::GetFormFromString<RE::TESWorldSpace>(identifier.asString());
						if (!worldspace) {
							logger::info("Config <{}>/[{}] requires worldspace {}, but it is not present. This is not fatal.", a_path, friendlyName.asString(), identifier.asString());
							continue;
						}
						validWorldspaceIdentifiers.push_back(worldspace);
					}

					Conditions::WorldspaceCondition newCondition{ validWorldspaceIdentifiers };
					newCondition.inverted = false;
					auto newConditionPtr = std::make_unique<Conditions::WorldspaceCondition>(newCondition);
					newConditions.push_back(std::move(newConditionPtr));
				}

				if (reverseWorldspaces) {
					if (!reverseWorldspaces.isArray()) {
						logger::warn("Config <{}>/[{}] has worldspaces specified, but it is not an array value. Config will be ignored.", a_path, friendlyName.asString());
						return;
					}

					std::vector<RE::TESWorldSpace*> validWorldspaceIdentifiers{};
					for (auto& identifier : reverseWorldspaces) {
						if (!identifier.isString()) {
							logger::warn("Config <{}>/[{}] has worldspaces specified, but an element is not a string. Config will be ignored.", a_path, friendlyName.asString());
							return;
						}

						const auto worldspace = Utilities::Forms::GetFormFromString<RE::TESWorldSpace>(identifier.asString());
						if (!worldspace) {
							logger::info("Config <{}>/[{}] requires worldspace {}, but it is not present. This is not fatal.", a_path, friendlyName.asString(), identifier.asString());
							continue;
						}
						validWorldspaceIdentifiers.push_back(worldspace);
					}

					Conditions::WorldspaceCondition newCondition{ validWorldspaceIdentifiers };
					newCondition.inverted = true;
					auto newConditionPtr = std::make_unique<Conditions::WorldspaceCondition>(newCondition);
					newConditions.push_back(std::move(newConditionPtr));
				}

				//Location Keywords check
				auto& locationKeywords = conditions["locationKeywords"];
				auto& reverseLocationKeywords = conditions["!locationKeywords"];
				if (locationKeywords) {
					if (!locationKeywords.isArray()) {
						logger::warn("Config <{}>/[{}] has locationKeywords specified, but it is not an array value. Config will be ignored.", a_path, friendlyName.asString());
						return;
					}

					std::vector<RE::BGSKeyword*> validLocationKeywords{};
					for (auto& identifier : locationKeywords) {
						if (!identifier.isString()) {
							logger::warn("Config <{}>/[{}] has locationKeywords specified, but an element is not a string. Config will be ignored.", a_path, friendlyName.asString());
							return;
						}

						const auto keyword = RE::TESForm::LookupByEditorID<RE::BGSKeyword>(identifier.asString());
						if (!keyword) {
							logger::info("Config <{}>/[{}] requires keyword {}, but it is not present. This is not fatal.", a_path, friendlyName.asString(), identifier.asString());
							continue;
						}
						validLocationKeywords.push_back(keyword);
					}
					Conditions::LocationKeywordCondition newCondition{ validLocationKeywords };
					auto newConditionPtr = std::make_unique<Conditions::LocationKeywordCondition>(newCondition);
					newCondition.inverted = false;
					newConditions.push_back(std::move(newConditionPtr));
				}

				if (reverseLocationKeywords) {
					if (!reverseLocationKeywords.isArray()) {
						logger::warn("Config <{}>/[{}] has locationKeywords specified, but it is not an array value. Config will be ignored.", a_path, friendlyName.asString());
						return;
					}

					std::vector<RE::BGSKeyword*> validLocationKeywords{};
					for (auto& identifier : reverseLocationKeywords) {
						if (!identifier.isString()) {
							logger::warn("Config <{}>/[{}] has locationKeywords specified, but an element is not a string. Config will be ignored.", a_path, friendlyName.asString());
							return;
						}

						const auto keyword = RE::TESForm::LookupByEditorID<RE::BGSKeyword>(identifier.asString());
						if (!keyword) {
							logger::info("Config <{}>/[{}] requires keyword {}, but it is not present. This is not fatal.", a_path, friendlyName.asString(), identifier.asString());
							continue;
						}
						validLocationKeywords.push_back(keyword);
					}
					Conditions::LocationKeywordCondition newCondition{ validLocationKeywords };
					newCondition.inverted = true;
					auto newConditionPtr = std::make_unique<Conditions::LocationKeywordCondition>(newCondition);
					newConditions.push_back(std::move(newConditionPtr));
				}

				//player skill check
				auto& playerSkillsField = conditions["playerSkills"];
				auto& reversePlayerSkillsField = conditions["!playerSkills"];
				if (playerSkillsField) {
					if (!playerSkillsField.isArray()) {
						logger::warn("Config <{}>/[{}] has playerSkills specified, but it is not an array value. Config will be ignored.", a_path, friendlyName.asString());
						return;
					}

					std::vector<std::pair<std::string, float>> requiredAVs;
					for (auto& identifier : playerSkillsField) {
						if (!identifier.isString()) {
							logger::warn("Config <{}>/[{}] has playerSkills specified, but an element is not a string. Config will be ignored.", a_path, friendlyName.asString());
							return;
						}

						auto components = Utilities::String::split(identifier.asString(), "|");
						if (components.size() != 2) {
							logger::warn("Config <{}>/[{}] has playerSkills specified, but an element ({}) is not formatted correctly (Skill|Level). Config will be ignored.", a_path, friendlyName.asString(), identifier.asString());
							return;
						}
						float requiredLevel = -1.0f;
						try {
							requiredLevel = std::stof(components.at(1));
						}
						catch (std::exception& e) {
							logger::warn("Config <{}>/[{}] has playerSkills specified, but an element ({}) is not formatted correctly (Skill|Level). Config will be ignored.", a_path, friendlyName.asString(), identifier.asString());
							logger::warn("Error: {}", e.what());
							return;
						}
						if (requiredLevel < 0.0f) {
							logger::warn("Don't use negative valued for playerSkills.");
						}

						requiredAVs.push_back({ components.at(0), requiredLevel });
					}
					for (const auto& pair : requiredAVs) {
						Conditions::AVCondition newCondition{ pair.first, pair.second };
						newCondition.inverted = false;
						auto newConditionPtr = std::make_unique<Conditions::AVCondition>(newCondition);
						newConditions.push_back(std::move(newConditionPtr));
					}
				}

				if (reversePlayerSkillsField) {
					if (!reversePlayerSkillsField.isArray()) {
						logger::warn("Config <{}>/[{}] has playerSkills specified, but it is not an array value. Config will be ignored.", a_path, friendlyName.asString());
						return;
					}

					std::vector<std::pair<std::string, float>> requiredAVs;
					for (auto& identifier : reversePlayerSkillsField) {
						if (!identifier.isString()) {
							logger::warn("Config <{}>/[{}] has playerSkills specified, but an element is not a string. Config will be ignored.", a_path, friendlyName.asString());
							return;
						}

						auto components = Utilities::String::split(identifier.asString(), "|");
						if (components.size() != 2) {
							logger::warn("Config <{}>/[{}] has playerSkills specified, but an element ({}) is not formatted correctly (Skill|Level). Config will be ignored.", a_path, friendlyName.asString(), identifier.asString());
							return;
						}
						float requiredLevel = -1.0f;
						try {
							requiredLevel = std::stof(components.at(1));
						}
						catch (std::exception& e) {
							logger::warn("Config <{}>/[{}] has playerSkills specified, but an element ({}) is not formatted correctly (Skill|Level). Config will be ignored.", a_path, friendlyName.asString(), identifier.asString());
							logger::warn("Error: {}", e.what());
							return;
						}
						if (requiredLevel < 0.0f) {
							logger::warn("Don't use negative valued for playerSkills.");
						}

						requiredAVs.push_back({ components.at(0), requiredLevel });
					}
					for (const auto& pair : requiredAVs) {
						Conditions::AVCondition newCondition{ pair.first, pair.second };
						newCondition.inverted = true;
						auto newConditionPtr = std::make_unique<Conditions::AVCondition>(newCondition);
						newConditions.push_back(std::move(newConditionPtr));
					}
				}

				//Global check
				auto& globalsField = conditions["globals"];
				auto& reverseGlobalsField = conditions["!globals"];
				if (globalsField) {
					if (!globalsField.isArray()) {
						logger::warn("Config <{}>/[{}] has globals specified, but it is not an array value. Config will be ignored.", a_path, friendlyName.asString());
						return;
					}

					std::vector<std::pair<RE::TESGlobal*, float>> requiredGlobals;
					for (auto& identifier : globalsField) {
						if (!identifier.isString()) {
							logger::warn("Config <{}>/[{}] has globals specified, but an element is not a string. Config will be ignored.", a_path, friendlyName.asString());
							return;
						}

						auto components = Utilities::String::split(identifier.asString(), "|"sv);
						if (components.size() != 2) {
							logger::warn("Config <{}>/[{}] has globals specified, but an element ({}) is not formatted correctly (Global|Value). Config will be ignored.", a_path, friendlyName.asString(), identifier.asString());
							return;
						}

						const auto global = RE::TESForm::LookupByEditorID<RE::TESGlobal>(components.at(0));
						if (!global) {
							logger::info("Config <{}>/[{}] requires global {}, but it is not present. This is not fatal.", a_path, friendlyName.asString(), identifier.asString());
							continue;
						}

						float globalValue = -1.0f;
						try {
							globalValue = std::stof(components.at(1));
						}
						catch (std::exception& e) {
							logger::warn("Config <{}>/[{}] has globals specified, but an element ({}) is not formatted correctly (Global|Value). Config will be ignored.", a_path, friendlyName.asString(), identifier.asString());
							logger::warn("Error: {}", e.what());
							return;
						}
						if (globalValue < 0.0f) {
							logger::warn("Don't use negative valued for globals.");
						}
						requiredGlobals.push_back({ global, globalValue });
					}
					for (const auto& pair : requiredGlobals) {
						Conditions::GlobalCondition newCondition{ pair.first, pair.second };
						newCondition.inverted = false;
						auto newConditionPtr = std::make_unique<Conditions::GlobalCondition>(newCondition);
						newConditions.push_back(std::move(newConditionPtr));
					}
				}

				if (reverseGlobalsField) {
					if (!reverseGlobalsField.isArray()) {
						logger::warn("Config <{}>/[{}] has globals specified, but it is not an array value. Config will be ignored.", a_path, friendlyName.asString());
						return;
					}

					std::vector<std::pair<RE::TESGlobal*, float>> requiredGlobals;
					for (auto& identifier : reverseGlobalsField) {
						if (!identifier.isString()) {
							logger::warn("Config <{}>/[{}] has globals specified, but an element is not a string. Config will be ignored.", a_path, friendlyName.asString());
							return;
						}

						auto components = Utilities::String::split(identifier.asString(), "|"sv);
						if (components.size() != 2) {
							logger::warn("Config <{}>/[{}] has globals specified, but an element ({}) is not formatted correctly (Global|Value). Config will be ignored.", a_path, friendlyName.asString(), identifier.asString());
							return;
						}

						const auto global = RE::TESForm::LookupByEditorID<RE::TESGlobal>(components.at(0));
						if (!global) {
							logger::info("Config <{}>/[{}] requires global {}, but it is not present. This is not fatal.", a_path, friendlyName.asString(), identifier.asString());
							continue;
						}

						float globalValue = -1.0f;
						try {
							globalValue = std::stof(components.at(1));
						}
						catch (std::exception& e) {
							logger::warn("Config <{}>/[{}] has globals specified, but an element ({}) is not formatted correctly (Global|Value). Config will be ignored.", a_path, friendlyName.asString(), identifier.asString());
							logger::warn("Error: {}", e.what());
							return;
						}
						if (globalValue < 0.0f) {
							logger::warn("Don't use negative valued for globals.");
						}
						requiredGlobals.push_back({ global, globalValue });
					}
					for (const auto& pair : requiredGlobals) {
						Conditions::GlobalCondition newCondition{ pair.first, pair.second };
						newCondition.inverted = true;
						auto newConditionPtr = std::make_unique<Conditions::GlobalCondition>(newCondition);
						newConditions.push_back(std::move(newConditionPtr));
					}
				}

				//Quest check
				auto& questConditionField = conditions["questConditions"];
				if (questConditionField) {
					if (!questConditionField.isObject()) {
						logger::warn("Config <{}>/[{}] has questConditions specified, but it is not an object value. Config will be ignored.", a_path, friendlyName.asString());
						return;
					}

					if (!questConditionField["questID"] || !questConditionField["questID"].isString()) {
						logger::warn("Config <{}>/[{}] has questConditions specified, but an element is missing questID (or it is not a string).", a_path, friendlyName.asString());
						return;
					}

					if (!(questConditionField["stageDone"] || questConditionField["completed"])) {
						logger::warn("Config <{}>/[{}] has questConditions specified, but is missing the actual condition (stagedone/completed).", a_path, friendlyName.asString());
						return;
					}

					QuestStruct candidate{};

					const auto quest = RE::TESForm::LookupByEditorID<RE::TESQuest>(questConditionField["questID"].asString());
					if (!quest) {
						logger::info("Config <{}>/[{}] requires quest {}, but it is not present. This is not fatal.", a_path, friendlyName.asString(), questConditionField["questID"].asString());
						continue;
					}

					bool completed = questConditionField["completed"] && questConditionField["completed"].isBool() ? questConditionField["completed"].asBool() : true;

					candidate.quest = quest;
					candidate.completed = completed;

					if (questConditionField["stageDone"] && questConditionField["stageDone"].isArray()) {
						for (const auto& field : questConditionField["stageDone"]) {
							if (!field.isUInt()) {
								logger::warn("Config <{}>/[{}] requires quest {}, but at least one stage specified is not a number, config will be ignored.", a_path, friendlyName.asString(), questConditionField["questID"].asString());
								return;
							}
							candidate.completedStages.push_back(static_cast<uint16_t>(field.asUInt()));
						}
					}
					Conditions::QuestCondition newCondition{ quest, candidate.completedStages, completed };
					newCondition.inverted = false;
					auto newConditionPtr = std::make_unique<Conditions::QuestCondition>(newCondition);
					newConditions.push_back(std::move(newConditionPtr));
				}
			} //End of Conditions

			//This is just verification, so forms are parsed twice. Improve this.
			for (auto& change : changes) {
				const auto& add = change["add"];
				const auto& remove = change["remove"];
				const auto& removeKeywords = change["removeByKeywords"];
				const auto& count = change["count"];
				if (!add && !remove && !removeKeywords && !count) {
					logger::warn("No changes detected, was this meant?");
					continue;
				}

				if (count) {
					if (!count.isUInt()) {
						logger::warn("Config <{}>/[{}], rule has invalid count.", a_path, friendlyName.asString());
						continue;
					}
				}
				if (remove) {
					if (!remove.isString()) {
						logger::warn("config <{}>/[{}], rule has invalid remove data.", a_path, friendlyName.asString());
						continue;
					}

					const auto obj = Utilities::Forms::GetFormFromString<RE::TESBoundObject>(remove.asString());
					if (!obj) {
						logger::warn("Config <{}>/[{}] contains invalid remove data - missing form {}.", a_path, friendlyName.asString(), remove.asString());
						continue;
					}
				}
				if (add) {
					if (!add.isArray()) {
						logger::warn("config <{}>/[{}], rule has invalid add data.", a_path, friendlyName.asString());
						continue;
					}

					bool shouldSkip = false;
					for (auto it = add.begin(); !shouldSkip && it != add.end(); ++it) {
						const auto& entry = *it;
						if (!entry.isString()) {
							logger::warn("Config <{}>/[{}] contains invalid add data.", a_path, friendlyName.asString());
							shouldSkip = true;
							continue;
						}

						auto* obj = Utilities::Forms::GetFormFromString<RE::TESBoundObject>(entry.asString());
						if (!obj) {
							logger::warn("Config <{}>/[{}] contains invalid add data - missing form {}.", a_path, friendlyName.asString(), entry.asString());
							shouldSkip = true;
							continue;
						}
					}
					if (shouldSkip) {
						continue;
					}
				}
				if (removeKeywords) {
					if (!removeKeywords.isArray()) {
						logger::warn("config <{}>/[{}], rule has invalid removeKeywords data.", a_path, friendlyName.asString());
						continue;
					}

					bool shouldSkip = false;
					for (auto it = removeKeywords.begin(); !shouldSkip && it != removeKeywords.end(); ++it) {
						const auto& entry = *it;
						if (!entry.isString()) {
							logger::warn("Config <{}>/[{}] contains invalid removeKeywords data.", a_path, friendlyName.asString());
							shouldSkip = true;
							continue;
						}

						const auto keyword = Utilities::Forms::GetFormFromString<RE::BGSKeyword>(entry.asString());
						if (!keyword) {
							logger::warn("Config <{}>/[{}] contains invalid removeKeywords data - missing form {}.", a_path, friendlyName.asString(), entry.asString());
							shouldSkip = true;
							continue;
						}
					}
					if (shouldSkip) {
						continue;
					}
				}
				Hooks::ContainerManager::GetSingleton()->RegisterRule(change, std::move(newConditions), bypassUnsafeContainers, distributeToVendors, onlyVendors, randomAdd);
			}
		}
	}

	void Read()
	{
		std::vector<std::string> paths{};
		try {
			paths = findJsonFiles();
		}
		catch (const std::exception& e) {
			logger::warn("Caught {} while reading files.", e.what());
			return;
		}
		if (paths.empty()) {
			logger::info("No settings found");
			return;
		}

		for (auto& path : paths) {
			Json::Reader JSONReader;
			Json::Value JSONFile;
			try {
				std::ifstream rawJSON(path);
				JSONReader.parse(rawJSON, JSONFile);
			}
			catch (const Json::Exception& e) {
				logger::warn("Caught {} while reading files.", e.what());
				continue;
			}
			catch (const std::exception& e) {
				logger::error("Caught unhandled exception {} while reading files.", e.what());
				continue;
			}

			if (!JSONFile.isObject()) {
				logger::warn("<{}> is not an object. File will be ignored.", path);
				continue;
			}

			ReadConfig(JSONFile, path);
		}
	}
}