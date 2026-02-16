#pragma once

#include "ContainerManager/ContainerManager.h"

namespace ContainerManager
{
	namespace RuleBuilder
	{
		struct Response
		{
			std::optional<Rule>         rule{};
			std::optional<ConfigErrors> errors{};
		};

		class Builder
		{
		public:
			void AddConditions(const Json::Value& a_conditions);
			void AddChanges(const Json::Value& a_changes);

			Response Build();
		private:
			enum class ConditionType
			{
				ActorValue,
				BaseForm,
				Global,
				Location,
				LocationKeyword,
				Quest,
				ReferenceCondition,
				Worldspace,
				References,
				Invalid
			};

			constexpr ConditionType ConditionTypeFromString(std::string_view key) noexcept {
				if (key == "playerSkills"sv || key == "!playerSkills"sv) return ConditionType::ActorValue;
				if (key == "containers"sv || key == "!containers"sv) return ConditionType::BaseForm;
				if (key == "locations"sv || key == "!locations"sv) return ConditionType::Global;
				if (key == "worldspaces"sv || key == "!worldspaces"sv) return ConditionType::Location;
				if (key == "locationKeywords"sv || key == "!locationKeywords"sv) return ConditionType::LocationKeyword;
				if (key == "globals"sv || key == "!globals"sv) return ConditionType::Quest;
				if (key == "questConditions"sv || key == "!questConditions"sv) return ConditionType::ReferenceCondition;
				if (key == "references"sv || key == "!references"sv) return ConditionType::Worldspace;
				return ConditionType::Invalid;
			}

			Builder& AddAVCondition(const Json::Value& a_condition, bool a_negate);
			Builder& AddBaseFormCondition(const Json::Value& a_condition, bool a_negate);
			Builder& AddGlobalCondition(const Json::Value& a_condition, bool a_negate);
			Builder& AddLocationCondition(const Json::Value& a_condition, bool a_negate);
			Builder& AddLocationKeywordCondition(const Json::Value& a_condition, bool a_negate);
			Builder& AddQuestCondition(const Json::Value& a_condition, bool a_negate);
			Builder& AddReferenceCondition(const Json::Value& a_condition, bool a_negate);
			Builder& AddWorldspaceCondition(const Json::Value& a_condition, bool a_negate);
			Builder& AddReferencesCondition(const Json::Value& a_condition, bool a_negate);

			Builder& AddReplaceChange(const Json::Value& a_change);
			Builder& AddAddChange(const Json::Value& a_change);
			Builder& AddRemoveChange(const Json::Value& a_change);
			Builder& AddRemoveByKeywordsChange(const Json::Value& a_change);
			Builder& AddReplaceByKeywordsChange(const Json::Value& a_change);

			bool valid{ true };
			Rule rule{};
			ConfigErrors errors{};
		};
	}
}