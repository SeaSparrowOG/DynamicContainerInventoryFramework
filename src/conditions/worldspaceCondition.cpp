#include "worldspaceCondition.h"

namespace Conditions
{
	bool WorldspaceCondition::IsValid(RE::TESObjectREFR* a_container)
	{
		const auto currentWorldspace = a_container->GetWorldspace();
		if (currentWorldspace) {
			for (const auto other : validWorldSpaces) {
				if (other == currentWorldspace) {
					return !inverted;
				}
			}
		}

		auto parent = currentWorldspace->parentWorld;
		while (parent) {
			for (const auto other : validWorldSpaces) {
				if (other == parent) {
					return !inverted;
				}
			}
			parent = parent->parentWorld;
		}
		return inverted;
	}

	WorldspaceCondition::WorldspaceCondition(std::vector<RE::TESWorldSpace*> a_worldspaces)
	{
		this->validWorldSpaces = a_worldspaces;
	}
}