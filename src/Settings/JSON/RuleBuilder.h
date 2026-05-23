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

			std::string RULE_CHANGES = "changes";
			std::string RULE_CONDITIONS = "conditions";
			std::string RULE_FRIENDLY_NAME = "friendlyname";

			std::unordered_set<std::string> _knownFields = {
				RULE_CHANGES,
				RULE_CONDITIONS,
				RULE_FRIENDLY_NAME
			};
		};

		inline static constexpr std::string_view TOP_LEVEL_RULES = "rules"sv;
		inline static constexpr std::string_view TOP_LEVEL_VERSION = "version"sv;
		inline static constexpr std::uint8_t     PARSER_VERSION = 2u;
	}
}