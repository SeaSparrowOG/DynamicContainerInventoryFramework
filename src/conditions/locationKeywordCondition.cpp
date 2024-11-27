#include "locationKeywordCondition.h"

#include "hooks/hooks.h"

namespace Conditions
{
	bool LocationKeywordCondition::IsValid(RE::TESObjectREFR* a_container)
	{
		auto currentLoc = a_container->GetCurrentLocation();
		currentLoc = currentLoc ? currentLoc : Hooks::ContainerManager::GetSingleton()->GetNearestMarkerLocation(a_container);
		if (!currentLoc) {
			return inverted;
		}

		for (const auto keyword : validKeywords) {
			if (currentLoc->HasKeyword(keyword)) {
				return !inverted;
			}
		}

		auto parent = currentLoc->parentLoc;
		while (parent) {
			for (const auto keyword : validKeywords) {
				if (parent->HasKeyword(keyword)) {
					return !inverted;
				}
			}
			parent = parent->parentLoc;
		}
		return inverted;
	}

	LocationKeywordCondition::LocationKeywordCondition(std::vector<RE::BGSKeyword*> a_keywords)
	{
		this->validKeywords = a_keywords;
	}

	void LocationKeywordCondition::Print()
	{
		logger::info("================================/");
		logger::info("|  Location Keyword Conditions /");
		logger::info("==============================/");
		for (const auto& form : validKeywords) {
			logger::info("  ->{}{}", inverted ? "Not " : "", form->GetFormEditorID());
		}
		logger::info("");
	}
}