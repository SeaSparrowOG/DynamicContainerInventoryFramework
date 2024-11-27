#include "containerCondition.h"

#include "utilities/utilities.h"

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

	void ContainerCondition::Print()
	{
		std::string litmus = Utilities::EDID::GetEditorID(validContainers.front());
		if (!litmus.empty()) {
			logger::info("=========================/");
			logger::info("|  Container Conditions /");
			logger::info("=======================/");
			for (const auto& form : validContainers) {
				logger::info("  ->{}{}", inverted ? "Not " : "", Utilities::EDID::GetEditorID(form));
			}
		}
		else {
			logger::info("PO3's Tweaks are required to view Container EDIDs!");
		}
		logger::info("");
	}
}