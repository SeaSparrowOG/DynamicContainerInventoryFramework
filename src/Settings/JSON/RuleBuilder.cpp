#include "RuleBuilder.h"

#include "Common/JSONUtils.hpp"

#include "ContainerManager/Changes/AddRule.h"

namespace Settings::JSON
{
	bool RuleBuilder::Build(const Json::Value& a_rule) {
		// Guarantees:
		// Non-Empty Object
		// If the Object was an old format config ({ rules : []}), we get each member of the rules array.

		auto members = a_rule.getMemberNames();
		for (const auto& member : members) {
			const auto& obj = a_rule[member];
			if (member == CONDITIONS) {
				AddConditions(obj);
			}
			else if (member == CHANGES) {
				AddChanges(obj);
			}
			else {
				topLevelErrors.AddInvalidTopLevelField(fmt::format("{}|{}", root, member), JSONUtils::GetObjectType(obj));
			}
		}

		if (changes.empty()) {
			logger::warn("Rule {} created no changes. This may be expected."sv, root);
			return true;
		}
		if (errors.empty()) {
			return true;
		}
		bool success = true;
		for (const auto& error : errors) {
			success &= error->Recoverable();
		}
		return success;
	}

	RuleBuilder::RuleBuilder(const std::string& a_root) {
		root = a_root;
	}

	RuleBuilder::~RuleBuilder() {
		if (changes.empty()) {
			return;
		}

		std::vector<std::size_t> ids{};
		auto* manager = ContainerManager::InventorySwapper::GetSingleton();
		if (!conditions.empty()) {
			ids.reserve(conditions.size());
			for (auto& condition : conditions) {
				auto id = manager->RegisterCondition(std::move(condition));
				ids.emplace_back(id);
			}
		}
		for (auto& change : changes) {
			change->DefineConditions(ids);
			manager->RegisterChange(std::move(change));
		}
	}

	void RuleBuilder::AddChanges(const Json::Value& a_changes, const std::string& a_overwriteRoot) {
		if (a_changes.isArray()) {
			std::string extendedPath = fmt::format("{}|{}", root, CHANGES);
			auto trimTo = extendedPath.size();
			for (unsigned int i = 0u; i < a_changes.size(); ++i) {
				extendedPath = fmt::format("{}[{}]", i);
				const auto& arrayObj = a_changes[i];
				if (arrayObj.isObject()) {
					AddChanges(arrayObj, extendedPath);
				}
				else {
					topLevelErrors.AddInvalidTopLevelField(extendedPath, JSONUtils::GetObjectType(arrayObj));
				}
				extendedPath.resize(trimTo);
			}
		}
		else if (a_changes.isObject()) {
			/*
			Expected:
			{
			  "Add"
			  "Remove"
			  "RemoveByKeyword"
			  "Count"
			}
			Guaranteed to not be empty
			*/
			bool hasAdd = false;
			bool hasRemove = false;
			bool hasRemoveKeywords = false;
			bool hasCount = false;
			auto members = a_changes.getMemberNames();
			for (const auto& member : members) {
				if (member == ADD) {
					hasAdd = true;
				}
				else if (member == REMOVE) {
					hasRemove = true;
				}
				else if (member == REMOVE_KEYWORDS) {
					hasRemoveKeywords = true;
				}
				else if (member == COUNT) {
					hasCount = true;
				}
				else {
					if (a_overwriteRoot.empty()) {
						topLevelErrors.AddInvalidTopLevelField(fmt::format("{}|{}|{}", root, CHANGES, member), JSONUtils::GetObjectType(a_changes));
					}
					else {
						topLevelErrors.AddInvalidTopLevelField(fmt::format("{}|{}", a_overwriteRoot, member), JSONUtils::GetObjectType(a_changes));
					}
				}
			}
			if (!hasAdd && !hasRemove && !hasRemoveKeywords) {
				if (a_overwriteRoot.empty()) {
					topLevelErrors.FlagMissingRequiredChangeFields(fmt::format("{}|{}", root, CHANGES));
				}
				else {
					topLevelErrors.FlagMissingRequiredChangeFields(a_overwriteRoot);
				}
				return;
			}

			// TODO: Fill this in.
			if (hasRemoveKeywords) {
				if (hasAdd) {

				}
				else {

				}
			}
			else {
				if (hasAdd && hasRemove) {

				}
				else if (hasRemove) {

				}
				else if (hasAdd) {
					auto addResult = ContainerManager::Changes::CreateAddRule(a_changes[ADD.data()], hasCount, a_changes[COUNT.data()]);
					if (!addResult) {
						std::unique_ptr<ContainerManager::ParseFailure> addFailure =
							std::make_unique<ContainerManager::Changes::AddChangeFailure>(addResult.error());
						errors.emplace_back(std::move(addFailure));
					}
					else {
						std::unique_ptr<ContainerManager::Change> addChange =
							std::make_unique<ContainerManager::Changes::AddChange>(addResult);
						changes.emplace_back(std::move(addChange));
					}
				}
			}
		}
		else {
			if (!a_overwriteRoot.empty()) {
				topLevelErrors.AddInvalidTopLevelField(a_overwriteRoot, JSONUtils::GetObjectType(a_changes));
			}
			else {
				topLevelErrors.AddInvalidTopLevelField(fmt::format("{}|{}", root, CHANGES), JSONUtils::GetObjectType(a_changes));
			}
		}
	}

	void RuleBuilder::TopLevelError::AddInvalidTopLevelField(const std::string& a_fieldPath, const std::string& a_fieldType) {
		std::string formattedError = fmt::format("{} - {}", a_fieldPath, a_fieldType);
		unknownTopLevelFields.emplace_back(std::move(formattedError));
	}

	void RuleBuilder::TopLevelError::FlagMissingRequiredChangeFields(const std::string& a_path) {
		missingRequiredChangeFields = true;
	}
}