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

		auto parent = currentWorldspace ? currentWorldspace->parentWorld : nullptr;
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

	void WorldspaceCondition::Print()
	{
		logger::info("==========================/");
		logger::info("|  Worldspace Conditions /");
		logger::info("========================/");
		for (const auto& form : validWorldSpaces) {
			logger::info("  ->{}{}",inverted ? "Not " : "", form->GetName());
		}
		logger::info("");
	}
}