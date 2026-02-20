#pragma once

#include "ContainerManager/ContainerManager.h"

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
	Valid Condition sub-field key names
	*/
	inline static constexpr std::string_view CONDITION_FLAG_VALUE = "value";
	inline static constexpr std::string_view CONDITION_FLAG_OPERAND = "operand";
	inline static constexpr std::string_view CONDITION_FLAG_OPERAND_OR = "or";
	inline static constexpr std::string_view CONDITION_FLAG_OPERAND_AND = "and";
	inline static constexpr std::string_view CONDITION_FLAG_IGNORE_PARENTS = "ignoreparents";

	/*
	Valid Change key names.
	*/
	inline static constexpr std::string_view CHANGE_ADD = "add";
	inline static constexpr std::string_view CHANGE_REMOVE = "remove";
	inline static constexpr std::string_view CHANGE_REMOVE_KEYWORDS = "removebykeywords";

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

	static ConditionType ConditionTypeFromString(std::string_view key) {
		if (key.size() > 1u && key.substr(0, 0) == "!") {
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

	class RuleHelper
	{
	public:
		RuleHelper(const Json::Value& a_normalizedJSON, const std::string& a_configName);

	private:
		void AddReferenceCondition(const Json::Value& a_condition, bool a_negate);
		void AddContainerCondition(const Json::Value& a_condition, bool a_negate);
		void AddLocationCondition(const Json::Value& a_condition, bool a_negate);
		void AddWorldspaceCondition(const Json::Value& a_condition, bool a_negate);
		void AddLocationKeywordCondition(const Json::Value& a_condition, bool a_negate);

		void AddPlayerSkillCondition(const Json::Value& a_condition, bool a_negate);
		void AddGlobalCondition(const Json::Value& a_condition, bool a_negate);
		void AddQuestCondition(const Json::Value& a_condition, bool a_negate);

		void AddReplaceChange(const Json::Value& a_change);
		void AddAddChange(const Json::Value& a_change);
		void AddRemoveChange(const Json::Value& a_change);
		void AddRemoveByKeywordsChange(const Json::Value& a_change);
		void AddReplaceByKeywordsChange(const Json::Value& a_change);

		// Helpers
		bool AllPluginsPresent(Json::Value& a_plugins);

		// Flags to be applied to the rule.
		bool allowVendors{ false };
		bool onlyVendors{ false };
		bool bypassUnsafe{ false };
		bool randomAdd{ false };

		// Plugin check
		bool allPluginsPresent{ true };

		// Components
		std::vector<Condition> conditions{};
		std::vector<Change> changes{};

		// Internal state
		struct ErrorHolder
		{
			using StringVec = std::vector<std::string>;

			StringVec emptyRules{};
			StringVec missingFields{};
			StringVec missingPlugins{};
			StringVec mixedArrayTypes{};
			StringVec unexpectedErrors{};
			StringVec badFieldValueType{};
			StringVec badFieldValueFormat{};
		};

		bool valid{ true };
		ErrorHolder errors{};
	};

	[[nodiscard]] bool BuildConditions();
}