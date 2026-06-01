#include "ContainerManager.h"

namespace ContainerManager
{
	void InventorySwapper::Report() const {
		if (!_encounteredErrors.empty()) {
			for (const auto& [config, errors] : _encounteredErrors) {
				logger::error("    - {}:", config);
				for (const auto& err : errors) {
					err->Report("      ");
				}
			}
		}
		if (!conditions.empty()) {
			logger::info("  Created Conditions:"sv);
			for (std::size_t i = 0u; i < conditions.size(); ++i) {
				logger::info("  ---- Condition ID: [{}] ----"sv, i);
				conditions[i]->PrintCondition("    ");
			}
		}
		if (!changes.empty()) {
			logger::info("  Created Changes:"sv);
			for (const auto& change : changes) {
				logger::info("  ----"sv);
				change->Report("    ");
			}
		}
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

	void InventorySwapper::RegisterConfigError(const std::string& configName, 
		std::unique_ptr<Errors::IError> err)
	{
		auto it = _encounteredErrors.find(configName);
		if (it == _encounteredErrors.end()) {
			_encounteredErrors[configName] = std::vector<std::unique_ptr<Errors::IError>>();
			_encounteredErrors[configName].emplace_back(std::move(err));
		}
		else {
			it->second.emplace_back(std::move(err));
		}
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

	void Change::DefineConditions(const std::vector<std::size_t>& a_ids) {
		ids = a_ids;
	}

	void PrintSwaps() {
		logger::info("Parsing completed successfully. Attempting to print rules..."sv);
		const auto* manager = InventorySwapper::GetSingleton();
		if (!manager) {
			logger::critical("  >Failed to retrieve internal manager singleton!"sv);
			return;
		}
		manager->Report();
	}
}