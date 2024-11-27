#include "locationCondition.h"

#include "hooks/hooks.h"

namespace Conditions
{
	bool LocationCondition::IsValid(RE::TESObjectREFR* a_container)
	{
		auto currentLoc = a_container->GetCurrentLocation();
		currentLoc = currentLoc ? currentLoc : Hooks::ContainerManager::GetSingleton()->GetNearestMarkerLocation(a_container);
		if (!currentLoc) {
			return inverted;
		}

		if (currentLoc) {
			for (const auto other : validLocations) {
				if (other == currentLoc) {
					return !inverted;
				}
			}

			auto parent = currentLoc->parentLoc;
			while (parent) {
				for (const auto other : validLocations) {
					if (other == parent) {
						return !inverted;
					}
				}
				parent = parent->parentLoc;
			}
		}
		return inverted;
	}

	LocationCondition::LocationCondition(std::vector<RE::BGSLocation*> a_locations)
	{
		this->validLocations = a_locations;
	}

	void LocationCondition::Print()
	{
		std::string litmus = Utilities::EDID::GetEditorID(validLocations.front());
		if (!litmus.empty()) {
			logger::info("========================/");
			logger::info("|  Location Conditions /");
			logger::info("======================/");
			for (const auto& form : validLocations) {
				logger::info("  ->{}{}", inverted ? "Not " : "", Utilities::EDID::GetEditorID(form));
			}
		}
		else {
			logger::info("PO3's Tweaks are required to view Location EDIDs!");
		}
		logger::info("");
	}
}