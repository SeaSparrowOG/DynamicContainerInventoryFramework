#include "locationKeywordCondition.h"

namespace Conditions
{
	bool LocationKeywordCondition::IsValid(RE::TESObjectREFR* a_container)
	{
		const auto currentLoc = a_container->GetCurrentLocation();
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
}