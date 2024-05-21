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
	bool ContainerManager::CreateSwapRule(SwapRule a_rule) {
		this->rules.push_back(a_rule);
		_loggerInfo("Registered new rule");
		_loggerInfo("Type: {}", a_rule.changeType == Settings::ChangeType::ADD ? "Add" :
			a_rule.changeType == Settings::ChangeType::REMOVE ? "Remove" : "Replace");
		if (a_rule.oldForm) {
			if (a_rule.changeType == Settings::ChangeType::REMOVE) {
				_loggerInfo("Form to remove: {}", a_rule.oldForm->GetName());
				_loggerInfo("Count: {}", a_rule.count);
			}
			else {
				_loggerInfo("Form to replace: {}", a_rule.oldForm->GetName());
			}
		}

		if (!a_rule.newForm.empty()) {
			if (a_rule.changeType == Settings::ChangeType::ADD) {
				if (a_rule.newForm.size() == 1) {
					_loggerInfo("Form to add: {}", a_rule.newForm.at(0)->GetName());
				}
				else {
					_loggerInfo("Forms to add:");
					for (auto form : a_rule.newForm) {
						_loggerInfo("    >{}", form->GetName());
					}
				}
				_loggerInfo("Count: {}", a_rule.count);
			}
			else {
				if (a_rule.newForm.size() == 1) {
					_loggerInfo("Form to substitute: {}", a_rule.newForm.at(0)->GetName());
				}
				else {
					_loggerInfo("Forms to substitute:");
					for (auto form : a_rule.newForm) {
						_loggerInfo("    >{}", form->GetName());
					}
				}
			}
		}

		_loggerInfo("----------------------------------------------");
		return true;
	}

	void ContainerManager::HandleContainer(RE::TESObjectREFR* a_ref) {
		for (auto& rule : this->rules) {
			auto count = a_ref->GetInventoryCounts()[rule.oldForm];
			if (rule.changeType == Settings::ChangeType::REMOVE || rule.changeType == Settings::ChangeType::REPLACE) {
				if (count < 1) continue;
			}

			if (!(
				HasLocationKeywordMatch(&rule, a_ref) && 
				HasLocationMatch(&rule, a_ref) &&
				IsValidContainer(&rule, a_ref))) {
				continue;
			}

			size_t rng = clib_util::RNG().generate<size_t>(0, rule.newForm.size() - 1);
			RE::TESBoundObject* thingToAdd = rule.newForm.at(rng);
			int32_t ruleCount = rule.count;

			if (rule.changeType == Settings::ChangeType::ADD) {
				a_ref->AddObjectToContainer(thingToAdd, nullptr, ruleCount, nullptr);
			} 
			else if (rule.changeType == Settings::ChangeType::REMOVE) {
				a_ref->RemoveItem(rule.oldForm, ruleCount, RE::ITEM_REMOVE_REASON::kRemove, nullptr, nullptr);
			}
			else {
				a_ref->RemoveItem(rule.oldForm, count, RE::ITEM_REMOVE_REASON::kRemove, nullptr, nullptr);
				a_ref->AddObjectToContainer(thingToAdd, nullptr, count, nullptr);
			}
		} //Rule reading end.
	}
}