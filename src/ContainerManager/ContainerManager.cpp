#include "ContainerManager.h"

namespace ContainerManager
{
	void InventorySwapper::RegisterChange(std::unique_ptr<Change> a_change) {
		if (changes.empty()) {
			changes.emplace_back(std::move(a_change));
			return;
		}

		auto type = a_change->GetType();
		auto it = std::lower_bound(
			changes.begin(),
			changes.end(),
			type,
			[](const std::unique_ptr<Change>& change, auto value)
			{
				return change->GetType() < value;
			});

		changes.emplace(it, std::move(a_change));
	}

	std::size_t InventorySwapper::RegisterCondition(std::unique_ptr<Condition> a_condition) {
		auto placedIn = conditions.size();
		conditions.emplace_back(std::move(a_condition));
		return placedIn;
	}

	void InventorySwapper::ManipulateInventory(RE::TESObjectREFR* a_container) {
		if (conditions.empty()) {
			return;
		}

		ConditionCheckParams params = ConditionCheckParams(a_container);
		std::unordered_set<std::size_t> validConditions{};
		for (std::size_t i = 0u; i < conditions.size(); ++i) {
			if (conditions.at(i)->IsValid(params)) {
				validConditions.insert(i);
			}
		}
		if (validConditions.empty()) {
			return;
		}

		for (const auto& change : changes) {
			if (change->CanApply(validConditions)) {
				change->Apply(a_container);
			}
		}
	}

	ConditionCheckParams::ConditionCheckParams(RE::TESObjectREFR* a_container) {
		reference = a_container;
	}

	bool Change::CanApply(const std::unordered_set<std::size_t> a_applicableConditions) const {
		for (const auto& id : ids) {
			if (!a_applicableConditions.contains(id)) {
				return false;
			}
		}
		return true;
	}

	void Change::DefineConditions(std::vector<std::size_t> a_ids) {
		ids = std::move(a_ids);
	}
}