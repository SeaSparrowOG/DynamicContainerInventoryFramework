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
			REX::INFO("========================/");
			REX::INFO("|  Location Conditions /");
			REX::INFO("======================/");
			for (const auto& form : validLocations) {
				REX::INFO("  ->{}{}", inverted ? "Not " : "", Utilities::EDID::GetEditorID(form));
			}
		}
		else {
			REX::INFO("PO3's Tweaks are required to view Location EDIDs!");
		}
		REX::INFO("");
	}
}