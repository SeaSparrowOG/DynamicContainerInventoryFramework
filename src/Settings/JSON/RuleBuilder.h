#pragma once

#include "ContainerManager/ContainerManager.h"

namespace Settings
{
	namespace JSON
	{
		class RuleBuilder
		{
		public:
			bool Build(const Json::Value& a_rule); // True - worked, False - fatal error
			RuleBuilder(const std::string& a_root);
			~RuleBuilder();                        // Pass changes and conditions to InventorySwapper, and mark errors.

		private:
			inline static constexpr std::string_view CONDITIONS = "conditions"sv;
			inline static constexpr std::string_view CHANGES = "changes"sv;

			inline static constexpr std::string_view ADD = "add";
			inline static constexpr std::string_view REMOVE = "remove";
			inline static constexpr std::string_view REMOVE_KEYWORDS = "removebykeywords";
			inline static constexpr std::string_view COUNT = "count";

			void AddConditions(const Json::Value& a_conditions);
			void AddChanges(const Json::Value& a_changes, const std::string& a_overwriteRoot = "");

			class TopLevelError : public ContainerManager::ParseFailure
			{
			public:
				bool Recoverable() const override { return false; };
				virtual void Report(const std::string& a_prefix) const override;

				void AddInvalidTopLevelField(const std::string& a_fieldPath, const std::string& a_fieldType);
				void AddInvalidChangeField(const std::string& a_fieldPath, const std::string& a_fieldType);
				void FlagMissingRequiredChangeFields(const std::string& a_path);

			private:
				bool missingRequiredChangeFields{ false };
				std::vector<std::string> unknownTopLevelFields{}; // Format: [Path] - [Type]
				std::vector<std::string> invalidChangeFields{};   // Format: [Path] - [Type]
			};

			// Necessitates some 
			// There are 2 types of errors - unrecoverable and recoverable. 
			// Example of a recoverable error:
			//		Add: [ <Found Form>, <Found Form>, <Missing Form>]
			// Example of an unrecoverable error:
			//      "LocationKeywords" exists, is an object, BUT has no <values> field.
			bool unrecoverableError{ false };

			std::string root{ "" };
			std::vector<std::unique_ptr<ContainerManager::ParseFailure>> errors{};
			std::vector<std::unique_ptr<ContainerManager::Condition>> conditions{};
			std::vector<std::unique_ptr<ContainerManager::Change>> changes{};

			TopLevelError topLevelErrors{};
		};
	}
}