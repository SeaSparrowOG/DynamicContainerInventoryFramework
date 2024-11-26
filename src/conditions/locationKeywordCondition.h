#pragma once

#include "condition.h"

namespace Conditions
{
	class LocationKeywordCondition : public Condition
	{
	public:
		bool IsValid(RE::TESObjectREFR* a_container) override;

	private:
		std::vector<RE::BGSKeyword*> validKeywords;
	};
}