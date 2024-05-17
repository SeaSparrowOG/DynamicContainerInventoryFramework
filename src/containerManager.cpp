#include "containerManager.h"

namespace ContainerManager {
	bool ContainerManager::CreateSwapRule(SwapRule a_rule) {
		this->rules.push_back(a_rule);
		return true;
	}

	void ContainerManager::HandleContainer(RE::TESObjectREFR* a_ref) {
		auto* parentLocation = a_ref->GetCurrentLocation();

		for (auto& rule : this->rules) {
			auto count = a_ref->GetInventoryCounts()[rule.oldForm];
			if (count < 1) continue;
			bool needsMatch = false;
			bool hasMatch = false;

			//Location name match.
			if (!rule.validLocations.empty()) {
				needsMatch = true;
				if (parentLocation) {
					auto it = std::find(rule.validLocations.begin(), rule.validLocations.end(), parentLocation);
					if (it != rule.validLocations.end()) {
						hasMatch = true;
					}
				}
			} //End of location search block.

			//Location keyword match.
			if (!rule.locationKeywords.empty() && parentLocation) {
				needsMatch = true;
				for (auto keywordString : rule.locationKeywords) {
					if (parentLocation->HasKeywordString(keywordString)) {
						hasMatch = true;
					}
				}
			}

			if (needsMatch && !hasMatch) continue;
			size_t rng = clib_util::RNG().generate<size_t>(0, rule.newForm.size() - 1);
			RE::TESBoundObject* thingToAdd = rule.newForm.at(rng);
			a_ref->RemoveItem(rule.oldForm, count, RE::ITEM_REMOVE_REASON::kRemove, nullptr, nullptr);
			a_ref->AddObjectToContainer(thingToAdd, nullptr, count, nullptr);

			_loggerInfo("Added {} {} to {} in {}.", count, thingToAdd->GetName(), a_ref->GetBaseObject()->As<RE::TESObjectCONT>()->GetName(), a_ref->GetCurrentLocation()->GetName());
		} //Rule reading end.
	}
}