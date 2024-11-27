#include "containerCondition.h"

namespace Conditions
{
	bool ContainerCondition::IsValid(RE::TESObjectREFR* a_container)
	{
		const auto baseObj = a_container->GetBaseObject();
		const auto baseContainer = baseObj ? baseObj->As<RE::TESObjectCONT>() : nullptr;
		if (baseContainer) {
			for (const auto other : validContainers) {
				if (other == baseContainer) {
					return !inverted;
				}
			}
		}
		return inverted;
	}

	ContainerCondition::ContainerCondition(std::vector<RE::TESObjectCONT*> a_containers)
	{
		this->validContainers = a_containers;
	}
}