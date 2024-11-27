#pragma once

#include "condition.h"

namespace Conditions
{
	class LocationKeywordCondition : public Condition
	{
	public:
		bool IsValid(RE::TESObjectREFR* a_container) override;

		LocationKeywordCondition(std::vector<RE::BGSKeyword*> a_keywords);

		void Print() override;
	private:
		std::vector<RE::BGSKeyword*> validKeywords;
	};
}