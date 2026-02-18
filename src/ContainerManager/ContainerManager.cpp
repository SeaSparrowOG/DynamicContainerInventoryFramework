#include "ContainerManager.h"

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
			rule.Apply(params);
		}
	}

	ConditionCheckParams::ConditionCheckParams(RE::TESObjectREFR* a_container)
	{
		reference = a_container;
	}

	void Rule::Apply(ConditionCheckParams& a_params) const
	{
		if (!CheckConditions(a_params)) {
			return;
		}
		for (const auto& change : changes) {
			if (change.CanApply(a_params)) {
				change.Apply(a_params);
			}
		}
	}

	bool Rule::CheckConditions(const ConditionCheckParams& a_params) const
	{
		for (const auto& condition : conditions) {
			if (!condition.IsValid(a_params)) {
				return false;
			}
		}
		return true;
	}

	void ConfigErrors::PrintErrors() const
	{
	}
}