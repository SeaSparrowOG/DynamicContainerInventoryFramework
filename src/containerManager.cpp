#include "containerManager.h"

namespace {
	bool HasLocationMatch(ContainerManager::SwapRule* a_rule, RE::TESObjectREFR* a_ref) {
		auto* refLoc = a_ref->GetEditorLocation();
		if (!refLoc || a_rule->validLocations.empty()) return true;

		for (auto* location : a_rule->validLocations) {
			if (location == refLoc) {
				return true;
			}
		}

		return false;
	}

	bool HasLocationKeywordMatch(ContainerManager::SwapRule* a_rule, RE::TESObjectREFR* a_ref) {
		auto* refLoc = a_ref->GetEditorLocation();
		if (!refLoc || a_rule->locationKeywords.empty()) return true;

		for (auto& locationKeyword : a_rule->locationKeywords) {
			if (refLoc->HasKeywordString(locationKeyword)) {
				return true;
			}
		}

		return false;
	}

	bool IsValidContainer(ContainerManager::SwapRule* a_rule, RE::TESObjectREFR* a_ref) {
		auto* refBaseContainer = a_ref->GetBaseObject()->As<RE::TESObjectCONT>();
		if (a_rule->container.empty()) return true;

		for (auto& container : a_rule->container) {
			if (refBaseContainer == container) {
				return true;
			}
		}

		return false;
	}

	bool IsValidReference(ContainerManager::SwapRule* a_rule, RE::TESObjectREFR* a_ref) {
		if (!a_rule->references.empty()) {
			std::stringstream stream;
			stream << std::hex << a_ref->formID;
			if (std::find(a_rule->references.begin(), a_rule->references.end(), a_ref->formID) != a_rule->references.end()) {
				return true;
			}
			return false;
		}
		return true;
	}
}

namespace ContainerManager {
	bool ContainerManager::IsRuleValid(SwapRule* a_rule, RE::TESObjectREFR* a_ref) {
		return (!HasRuleApplied(a_rule, a_ref) &&
			IsValidReference(a_rule, a_ref) &&
			HasLocationKeywordMatch(a_rule, a_ref) &&
			HasLocationMatch(a_rule, a_ref) &&
			IsValidContainer(a_rule, a_ref));
	}

	bool ContainerManager::HasRuleApplied(SwapRule* a_rule, RE::TESObjectREFR* a_ref) {
		if (this->handledContainers.find(a_ref) != this->handledContainers.end()) {
			auto& rulePair = this->handledContainers[a_ref];
			if (a_rule->ruleName != rulePair.second) return false;
			if (RE::Calendar::GetSingleton()->GetDaysPassed() > rulePair.first + 10) return false;
			return true;
		}
		return false;
	}

	void ContainerManager::CreateSwapRule(SwapRule a_rule) {
		if (!a_rule.oldForm) {
			this->addRules.push_back(a_rule);
			_loggerInfo("Registered bew <Add> rule.");
			_loggerInfo("    Form(s) that can be added:");
			for (auto form : a_rule.newForm) {
				_loggerInfo("        >{}.", form->GetName());
			}
			_loggerInfo("    Ammount to be added: {}.", a_rule.count);
			_loggerInfo("----------------------------------------------");
		}
		else if (a_rule.newForm.empty()) {
			this->removeRules.push_back(a_rule);

			_loggerInfo("Registered bew <Remove> rule.");
			_loggerInfo("    Form that will be removed: {}.", a_rule.oldForm->GetName());
			_loggerInfo("    Ammount to be removed: {}.", a_rule.count);
			_loggerInfo("----------------------------------------------");
		}
		else {
			this->replaceRules.push_back(a_rule);

			_loggerInfo("Registered bew <Replace> rule.");
			_loggerInfo("    Form(s) that can be added:");
			for (auto form : a_rule.newForm) {
				_loggerInfo("        >{}.", form->GetName());
			}
			_loggerInfo("    Form that will be removed: {}.", a_rule.oldForm->GetName());
			_loggerInfo("----------------------------------------------");
		}
	}

	void ContainerManager::HandleContainer(RE::TESObjectREFR* a_ref) {
		bool isVendorContainer = false;
		auto* ownerFaction = a_ref->GetFactionOwner();
		if (ownerFaction && ownerFaction->IsVendor()) {
			isVendorContainer = true;
		}
		std::vector<std::string> appliedRules;

		for (auto& rule : this->replaceRules) {
			if (isVendorContainer && !rule.distributeToVendors) continue;
			auto itemCount = a_ref->GetInventoryCounts()[rule.oldForm];
			if (itemCount < 1) continue;
			if (!IsRuleValid(&rule, a_ref)) continue;

			size_t rng = clib_util::RNG().generate<size_t>(0, rule.newForm.size() - 1);
			RE::TESBoundObject* thingToAdd = rule.newForm.at(rng);
			a_ref->RemoveItem(rule.oldForm, itemCount, RE::ITEM_REMOVE_REASON::kRemove, nullptr, nullptr);
			a_ref->AddObjectToContainer(thingToAdd, nullptr, itemCount, nullptr);

			appliedRules.push_back(rule.ruleName);
		} //Replace Rule reading end.

		for (auto& rule : this->removeRules) {
			if (isVendorContainer && !rule.distributeToVendors) continue;
			auto itemCount = a_ref->GetInventoryCounts()[rule.oldForm];
			if (itemCount < 1) continue;
			if (!IsRuleValid(&rule, a_ref)) continue;

			int32_t ruleCount = rule.count;
			if (ruleCount > itemCount) {
				ruleCount = itemCount;
			}
			a_ref->RemoveItem(rule.oldForm, ruleCount, RE::ITEM_REMOVE_REASON::kRemove, nullptr, nullptr);

			appliedRules.push_back(rule.ruleName);
		} //Remove rule reading end.

		for (auto& rule : this->addRules) {
			if (isVendorContainer && !rule.distributeToVendors) continue;
			if (!IsRuleValid(&rule, a_ref)) continue;

			int32_t ruleCount = rule.count;
			size_t rng = clib_util::RNG().generate<size_t>(0, rule.newForm.size() - 1);
			RE::TESBoundObject* thingToAdd = rule.newForm.at(rng);
			a_ref->AddObjectToContainer(thingToAdd, nullptr, ruleCount, nullptr);

			appliedRules.push_back(rule.ruleName);
		} //Add rule reading end.

		float daysPassed = RE::Calendar::GetSingleton()->GetDaysPassed();
		for (auto rule : appliedRules) {
			std::pair<float, std::string> rulePair;
			rulePair.first = daysPassed;
			rulePair.second = rule;
			this->handledContainers[a_ref] = rulePair;
		}
	}

	void ContainerManager::LoadMap() {
		this->handledContainers.clear();
	}
}