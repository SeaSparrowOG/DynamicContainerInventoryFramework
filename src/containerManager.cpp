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
}

namespace ContainerManager {
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
		auto* ownerFaction = a_ref->GetFactionOwner();
		if (ownerFaction && ownerFaction->IsVendor()) {
			return;
		}

		for (auto& rule : this->replaceRules) {
			auto itemCount = a_ref->GetInventoryCounts()[rule.oldForm];
			if (itemCount < 1) continue;

			if (!(
				HasLocationKeywordMatch(&rule, a_ref) &&
				HasLocationMatch(&rule, a_ref) &&
				IsValidContainer(&rule, a_ref))) {
				continue;
			}

			size_t rng = clib_util::RNG().generate<size_t>(0, rule.newForm.size() - 1);
			RE::TESBoundObject* thingToAdd = rule.newForm.at(rng);
			a_ref->RemoveItem(rule.oldForm, itemCount, RE::ITEM_REMOVE_REASON::kRemove, nullptr, nullptr);
			a_ref->AddObjectToContainer(thingToAdd, nullptr, itemCount, nullptr);
		} //Replace Rule reading end.

		for (auto& rule : this->removeRules) {
			auto itemCount = a_ref->GetInventoryCounts()[rule.oldForm];
			if (itemCount < 1) continue;

			if (!(
				HasLocationKeywordMatch(&rule, a_ref) &&
				HasLocationMatch(&rule, a_ref) &&
				IsValidContainer(&rule, a_ref))) {
				continue;
			}

			int32_t ruleCount = rule.count;
			if (ruleCount > itemCount) {
				ruleCount = itemCount;
			}
			a_ref->RemoveItem(rule.oldForm, ruleCount, RE::ITEM_REMOVE_REASON::kRemove, nullptr, nullptr);
		} //Remove rule reading end.

		for (auto& rule : this->addRules) {
			if (!(
				HasLocationKeywordMatch(&rule, a_ref) &&
				HasLocationMatch(&rule, a_ref) &&
				IsValidContainer(&rule, a_ref))) {
				continue;
			}

			int32_t ruleCount = rule.count;
			size_t rng = clib_util::RNG().generate<size_t>(0, rule.newForm.size() - 1);
			RE::TESBoundObject* thingToAdd = rule.newForm.at(rng);
			a_ref->AddObjectToContainer(thingToAdd, nullptr, ruleCount, nullptr);
		} //Add rule reading end.
	}
}