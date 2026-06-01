#pragma once

#include "ContainerManager/ContainerManager.h"

namespace Settings
{
	namespace JSON
	{
		[[nodiscard]] bool ParseArray(const Json::Value& array, std::string& path);
		[[nodiscard]] bool ParseObject(const Json::Value& object, const std::string& name);
		[[nodiscard]] bool ParseSuccessfulConfigs();

		class RuleParser
		{
		public:
			RuleParser(const Json::Value& canonicalizedRule, const std::string& configName);
			~RuleParser();
			[[nodiscard]] bool BuildRule();

		private:
			using Change = std::unique_ptr<ContainerManager::Change>;
			using Condition = std::unique_ptr<ContainerManager::Condition>;
			using ParseFailure = std::unique_ptr<ContainerManager::ParseFailure>;

			bool                      _errored{ false };
			Json::Value               _rule{};
			std::string               _configName{};
			std::vector<Change>       _changes{};
			std::vector<Condition>    _conditions{};
			std::vector<ParseFailure> _failures{};

			/*
			* --------------------------
			*	Top Level Fields:
			* --------------------------
			*/
			std::string RULE_CHANGES = "changes";
			std::string RULE_CONDITIONS = "conditions";
			std::string RULE_FRIENDLY_NAME = "friendlyname";

			std::unordered_set<std::string> _knownFields = {
				RULE_CHANGES,
				RULE_CONDITIONS,
				RULE_FRIENDLY_NAME
			};

			/*
			* --------------------------
			*	Special Condition Flags:
			* --------------------------
			*/
			bool        _onlyVendors = false;
			std::string CONDITIONS_ONLY_VENDORS = "onlyvendors";
			bool        _allowVendors = false;
			std::string CONDITIONS_ALLOW_VENDORS = "allowvendors";
			bool        _randomAdd = false;
			std::string CONDITIONS_RANDOM_ADD = "randomadd";
			bool        _allowNoReset = false;
			std::string CONDITIONS_ALLOW_NO_RESET = "allownoreset";
			std::string CONDITIONS_ALLOW_NO_RESET_OLD = "bypassunsafecontainers";

			/*
			* --------------------------
			*	Recognized Conditions
			* --------------------------
			*/
			std::string AV_CONDITION = "playerskills";
			std::string BASEFORM_CONDITION = "containers";

			std::unordered_set<std::string> _knownConditionFields = {
				CONDITIONS_ONLY_VENDORS,
				CONDITIONS_ALLOW_VENDORS,
				CONDITIONS_RANDOM_ADD,
				CONDITIONS_ALLOW_NO_RESET,
				CONDITIONS_ALLOW_NO_RESET_OLD,

				AV_CONDITION,
				BASEFORM_CONDITION
			};

			/*
			* ---------------------------
			*	Failures
			* ---------------------------
			*/
			ContainerManager::UnknownFieldFailure     _unknownFields = _configName;
			ContainerManager::MissingFieldFailure     _missingFields = _configName;
			ContainerManager::InvalidFieldTypeFailure _invalidFields = _configName;

			[[nodiscard]] bool ParseChanges(const Json::Value& changes);
		};

		inline static constexpr std::string_view TOP_LEVEL_RULES = "rules"sv;
		inline static constexpr std::string_view TOP_LEVEL_VERSION = "version"sv;
		inline static constexpr std::uint8_t     PARSER_VERSION = 2u;
	}
}