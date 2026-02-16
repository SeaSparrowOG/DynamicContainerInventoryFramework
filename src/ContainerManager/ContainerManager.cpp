#include "ContainerManager.h"

#include "Changes/AddRule.h"
#include "Changes/RemoveByKeywordsRule.h"
#include "Changes/RemoveRule.h"
#include "Changes/ReplaceByKeywordsRule.h"
#include "Changes/ReplaceRule.h"

#include "Conditions/AVCondition.h"
#include "Conditions/BaseFormCondition.h"
#include "Conditions/GlobalCondition.h"
#include "Conditions/LocationCondition.h"
#include "Conditions/LocationKeywordCondition.h"
#include "Conditions/QuestCondition.h"
#include "Conditions/ReferenceCondition.h"
#include "Conditions/WorldspaceCondition.h"

namespace ContainerManager
{
	bool InventorySwapper::Initialize()
	{
		return true;
	}

	void InventorySwapper::ManipulateInventory(RE::TESObjectREFR* a_container)
	{
		ConditionCheckParams params = ConditionCheckParams(a_container);
		for (const auto& rule : rules) {
			rule->Apply(params);
		}
	}

	ConditionCheckParams::ConditionCheckParams(RE::TESObjectREFR* a_container)
	{
		reference = a_container;
	}

	void Rule::Apply(ConditionCheckParams& a_params)
	{
		if (!CheckConditions(a_params)) {
			return;
		}
		for (const auto& change : changes) {
			if (change->CanApply(a_params)) {
				change->Apply(a_params);
			}
		}
	}

	bool Rule::CheckConditions(const ConditionCheckParams& a_params)
	{
		for (const auto& condition : conditions) {
			if (!condition->IsValid(a_params)) {
				return false;
			}
		}
		return true;
	}

	void ConfigErrors::PrintErrors() const
	{
		if (!hasErrors) {
			logger::info("  >{} loaded correctly."sv, configName);
			return;
		}
		logger::info("  >{} encountered some errors while loading."sv);
		if (!invalidFieldNames.empty()) {
			logger::info("    Config contains the following fields which are not recognized by the framework:"sv);
			for (const auto& field : invalidFieldNames) {
				logger::info("      >{}"sv, field);
			}
		}
		if (!invalidFieldValues.empty()) {
			logger::info("    Config has certain fields specified, but those fields contain unexpected values."sv);
			for (const auto& field : invalidFieldValues) {
				logger::info("      >{}"sv, field);
			}
		}
		if (!unresolvedConditions.empty()) {
			logger::info("    Config has at least one swap rule that has an empty condition. While you can omit conditions, an unresolved condition indicates that a filter came up empty, and the swap was ignored."sv);
			for (const auto& field : unresolvedConditions) {
				logger::info("      >{}"sv, field);
			}
		}
		if (!missingRequiredFields.empty()) {
			logger::info("    Config is missing some required fields. This means that it is either missing friendlyName, or the changes field is missing at least an add or remove field."sv);
			for (const auto& field : invalidFieldValues) {
				logger::info("      >{}"sv, field);
			}
		}
	}
}