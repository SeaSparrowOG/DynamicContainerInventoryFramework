#pragma once

#include "ContainerManager/ContainerManager.h"

#include "Conditions/AVCondition.h"

namespace ContainerManager
{
	/*
	Valid Condition key names.
	*/
	inline static constexpr std::string_view CONDITION_PLUGINS = "plugins";
	inline static constexpr std::string_view CONDITION_ALLOW_UNSAFE = "bypassunsafecontainers";
	inline static constexpr std::string_view CONDITION_ALLOW_VENDORS = "allowvendors";
	inline static constexpr std::string_view CONDITION_ONLY_VENDORS = "onlyVendors";
	inline static constexpr std::string_view CONDITION_RANDOM_ADD = "randomadd";

	inline static constexpr std::string_view CONDITION_REFRENCES = "references";
	inline static constexpr std::string_view CONDITION_CONTAINERS = "containers";
	inline static constexpr std::string_view CONDITION_LOCATIONS = "locations";
	inline static constexpr std::string_view CONDITION_WORLDSPACES = "worldspaces";
	inline static constexpr std::string_view CONDITION_LOC_KEYWORDS = "locationkeywords";

	inline static constexpr std::string_view CONDITION_PLAYER_SKILLS = "playerskills";
	inline static constexpr std::string_view CONDITION_GLOBALS = "globals";
	inline static constexpr std::string_view CONDITION_QUESTS = "questconditions";

	/*
	Valid top level key names.
	*/
	inline static constexpr std::string_view TOP_LEVEL_VERSION = "requiredversion"sv;
	inline static constexpr std::string_view TOP_LEVEL_FRIENDLY_NAME = "friendlyname"sv;
	inline static constexpr std::string_view TOP_LEVEL_CONDITIONS = "conditions"sv;
	inline static constexpr std::string_view TOP_LEVEL_CHANGES = "changes"sv;

	/*
	Change valid key names
	*/
	inline static constexpr std::string_view CHANGE_ADD = "add"sv;
	inline static constexpr std::string_view CHANGE_REMOVE = "remove"sv;
	inline static constexpr std::string_view CHANGE_REMOVE_BY_KEYWORD = "removebykeyword"sv;
	inline static constexpr std::string_view CHANGE_RANDOM_ADD{ "randomadd"sv };
	inline static constexpr std::string_view CHANGE_COUNT{ "count"sv };

	/*
	Current Parser Version. 
	*/
	inline static constexpr int PARSER_VERSION = 3;

	class RuleConstructor
	{
	public:
		RuleConstructor(const std::string& a_path);
		bool Build(const Json::Value& a_rule);

	private:
		enum class ConditionType
		{
			Plugins,          // Checked on rule reading. If invalid, rule is discarded.
			BypassUnsafe,     // Allows "NoReset" contaners to be affected by the rule.
			AllowVendors,     // Allows Vendors containers to be affected by the rule.
			OnlyVendors,      // Restricts the rule to ONLY vendor containers, and enables AllowVendors.
			RandomAdd,        // Instead of adding/removing <Count> of each item in the Change field, 
			// rule will now add 1 random item from the change field <Count> times.

			References,       // Restricts the rule to ONLY these references. OR by default.
			Containers,       // Restricts the rule to only references whose base form is this. OR.
			Locations,        // Restricts the rule to only these locations. OR.
			Worldpsaces,      // Restricts the rule to only these worldspaces. OR.
			LocationKeywords, // Restructs the rule to only Locations with these keywords. AND.

			PlayerSkills,     // Rule only applies if the player has required skill level. AND.
			Globals,          // Rule only applies if these globals match the required value. OR.
			Quests,           // Rule only applies if these quests are in the prerequisite state. AND.

			Invalid
		};

		inline ConditionType ConditionTypeFromString(std::string_view key) {
			if (key.starts_with("!")) {
				key = key.substr(1, key.size() - 1);
			}

			if (key == CONDITION_PLUGINS) return ConditionType::Plugins;
			if (key == CONDITION_ALLOW_UNSAFE) return ConditionType::BypassUnsafe;
			if (key == CONDITION_ALLOW_VENDORS) return ConditionType::AllowVendors;
			if (key == CONDITION_ONLY_VENDORS) return ConditionType::OnlyVendors;
			if (key == CONDITION_RANDOM_ADD) return ConditionType::RandomAdd;

			if (key == CONDITION_REFRENCES) return ConditionType::References;
			if (key == CONDITION_CONTAINERS) return ConditionType::Containers;
			if (key == CONDITION_LOCATIONS) return ConditionType::Locations;
			if (key == CONDITION_WORLDSPACES) return ConditionType::Worldpsaces;
			if (key == CONDITION_LOC_KEYWORDS) return ConditionType::LocationKeywords;

			if (key == CONDITION_PLAYER_SKILLS) return ConditionType::PlayerSkills;
			if (key == CONDITION_GLOBALS) return ConditionType::Globals;
			if (key == CONDITION_QUESTS) return ConditionType::Quests;

			return ConditionType::Invalid;
		}

		void RegisterChange(const Json::Value& a_changes, const std::string& a_path);
		void CreateConditions(const Json::Value& a_condition);
		void AddPlayerSkillCondition(const Json::Value& a_condition, bool a_invert);

		class TopLevelErrors : public Failure
		{
		public:
			virtual void Report(const std::string& a_prefix = "    ") const;

			TopLevelErrors(const std::string& a_path) { path = a_path; };
			bool Errored() const;

			void FlagUnknownKey(const std::string& a_name, const std::string& a_type) {
				unknownValues.emplace_back(fmt::format("{} - {}", a_name, a_type));
			}
			void FlagUnknownConditionKey(const std::string& a_name) {
				unknownConditionKeys.emplace_back(fmt::format("{}|Conditions|{}", path, a_name));
			}
			void FlagBadCondition(const std::string& a_name, std::unique_ptr<Failure> a_failure) {
				failures.emplace(a_name, std::move(a_failure));
			}
		private:
			std::string path{ "" };

			bool missingChanges{ false };
			std::vector<std::string> unknownValues{};
			std::vector<std::string> unknownConditionKeys{};
			std::map<std::string, std::unique_ptr<Failure>> failures{};
		};

		std::string path{ "" };
		TopLevelErrors errors{ path };
		InventorySwapper* inventorySwapper{ InventorySwapper::GetSingleton() };
		std::vector<std::unique_ptr<Condition>> pendingConditions{};
		std::vector<std::unique_ptr<Change>>    pendingChanges{};
	};

	class StructuredErrorMessages : public Failure
	{
	public:
		virtual void Report(const std::string& a_prefix = "    ") const;

		bool Errored() const;
		void SetName(const std::string& a_name) { name = a_name; };
		void FlagInvalidObject(const std::string& a_path, const std::string& a_type);
	private:
		bool empty{ false };

		using StringVec = std::vector<std::string>;
		std::string name{ "" };
		StringVec invalidObjectTypes{};
	};

	[[nodiscard]] bool BuildConditions();
}