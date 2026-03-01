#include "ContainerManager.h"

namespace ContainerManager
{
	bool InventorySwapper::Report() const {
#ifndef NDEBUG 
		if (!conditions.empty()) {
			std::size_t size = conditions.size();
			logger::info("  Found {} conditions."sv, size);
			for (std::size_t i = 0u; i < size; ++i) {
				const auto& condition = conditions.at(i);
				condition->PrintCondition("    ");
			}
		}
		else {
			logger::info("  No conditions found."sv);
		}
		if (!changes.empty()) {
			std::size_t size = changes.size();
			logger::info("  Found {} changes."sv, size);
			for (std::size_t i = 0u; i < size; ++i) {
				const auto& change = changes.at(i);
				change->Report("    ");
			}
		}
		else {
			logger::info("  No changes found."sv);
		}
		if (!failures.empty()) {
			std::size_t size = failures.size();
			logger::info("  Found {} failures."sv, size);
			for (const auto& [config, configFailures] : failures) {
				logger::info("    >{}", config);
				for (const auto& failure : configFailures) {
					failure->Report("      ");
				}
			}
		}
		else {
			logger::info("  No failures found"sv);
		}
		return true;
#else
		if (!changes.empty()) {
			for (const auto& change : changes) {
				change->Report("    ");
			}
		}
		else {
			logger::info("    No rules built."sv);
		}
		return true;
#endif
	}

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

	void InventorySwapper::RegisterFailure(const std::string& a_config, std::unique_ptr<Failure> a_failure) {
		auto it = failures.find(a_config);
		if (it == failures.end()) {
			std::vector<std::unique_ptr<Failure>> failure;
			failure.emplace_back(std::move(a_failure));
			failures[a_config] = std::move(failure);
			return;
		}
		(*it).second.emplace_back(std::move(a_failure));
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

		ContainerDeltas deltas{};
		for (const auto& change : changes) {
			if (change->CanApply(validConditions, deltas)) {
				change->Apply(a_container, deltas);
			}
		}
	}

	ConditionCheckParams::ConditionCheckParams(RE::TESObjectREFR* a_container) {
		reference = a_container;
	}

	bool Change::CanApply(const std::unordered_set<std::size_t> a_applicableConditions, [[maybe_unused]] const ContainerDeltas& a_deltas) const {
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

	bool PrintSwaps() {
		logger::info("Parsing completed successfully. Attempting to print rules..."sv);
		const auto* manager = InventorySwapper::GetSingleton();
		if (!manager) {
			logger::critical("  >Failed to retrieve internal manager singleton!"sv);
			return false;
		}
		return manager->Report();
	}
}