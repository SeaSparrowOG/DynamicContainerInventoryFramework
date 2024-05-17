#include "containerManager.h"

namespace ContainerManager {
	bool ContainerManager::CreateSwapRule(SwapRule a_rule) {
		this->rules.push_back(a_rule);
		return true;
	}

	void ContainerManager::HandleContainer(RE::TESObjectREFR* a_ref) {
		bool addRemoveEnabled = true;
		auto* containerObject = a_ref->GetBaseObject()->As<RE::TESObjectCONT>();

		if (this->managedContainerMap.find(a_ref) != this->managedContainerMap.end()) {
			float dayAttached = this->managedContainerMap.at(a_ref);
			float currentDay = RE::Calendar::GetSingleton()->gameDaysPassed->value;
			if (currentDay - dayAttached < 30.0f) {
				addRemoveEnabled = false;
			}
		}

		for (auto& rule : this->rules) {
			//Location name and keyword match.
			bool needsLocationMatch = false;
			bool hasLocationMatch = false;
			if (!rule.locationIdentifiers.empty()) {
				needsLocationMatch = true;
				auto* parentLocation = a_ref->GetCurrentLocation();
				if (parentLocation) {
					std::string locationName = parentLocation->GetName();
					auto it = std::find(rule.locationIdentifiers.begin(), rule.locationIdentifiers.end(), locationName);
					if (it != rule.locationIdentifiers.end()) {
						hasLocationMatch = true;
					}
					else {
						for (auto identifier : rule.locationIdentifiers) {
							if (parentLocation->HasKeywordString(identifier)) {
								hasLocationMatch = true;
							}
						}
					}
				}
			} //End of location search block.

			if (needsLocationMatch && !hasLocationMatch) continue;
			auto count = a_ref->GetInventoryCounts().at(rule.oldForm);
			if (count < 1) continue;

			size_t rng = clib_util::RNG().generate<size_t>(0, rule.newForm.size() - 1);
			RE::TESBoundObject* thingToAdd = rule.newForm.at(rng);
			a_ref->RemoveItem(rule.oldForm, count, RE::ITEM_REMOVE_REASON::kRemove, nullptr, nullptr);
			a_ref->AddObjectToContainer(thingToAdd, nullptr, count, nullptr);
		} //Rule reading end.
	}
}