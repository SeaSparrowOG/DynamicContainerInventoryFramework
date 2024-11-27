#include "locationCondition.h"

namespace Conditions
{
	bool LocationCondition::IsValid(RE::TESObjectREFR* a_container)
	{
		const auto currentLocation = a_container->GetCurrentLocation();
		if (currentLocation) {
			for (const auto other : validLocations) {
				if (other == currentLocation) {
					return !inverted;
				}
			}

			auto parent = currentLocation->parentLoc;
			while (parent) {
				for (const auto other : validLocations) {
					if (other == parent) {
						return !inverted;
					}
				}
				parent = parent->parentLoc;
			}
		}
		return false;
	}

	LocationCondition::LocationCondition(std::vector<RE::BGSLocation*> a_locations)
	{
		this->validLocations = a_locations;
	}
}