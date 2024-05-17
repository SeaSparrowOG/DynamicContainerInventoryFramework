#include "containerManager.h"

namespace ContainerManager {
	bool ContainerManager::CreateSwapRule(SwapRule a_rule) {
		this->rules.push_back(a_rule);

		_loggerInfo("Registered new rule:");
		_loggerInfo("    >Form to be replaced: {}", a_rule.oldForm->GetName());

		if (a_rule.newForm.size() == 1) {
			_loggerInfo("    >Replaced with: {}", a_rule.newForm.at(0)->GetName());
		}
		else {
			_loggerInfo("    >Can be replaced with:");
			for (auto candidate : a_rule.newForm) {
				_loggerInfo("        {}", candidate->GetName());
			}
		}

		if (!(a_rule.locationKeywords.empty() && a_rule.validLocations.empty())) {
			if (!a_rule.locationKeywords.empty()) {
				_loggerInfo("    >Location keywords:");
				for (auto locationKeyword : a_rule.locationKeywords) {
					_loggerInfo("        {}", locationKeyword);
				}
			}

			if (!a_rule.validLocations.empty()) {
				_loggerInfo("    >Locations:");
				for (auto location : a_rule.validLocations) {
					_loggerInfo("        {}", clib_util::editorID::get_editorID(location));
				}
			}
		}
		_loggerInfo("----------------------------------------------");
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