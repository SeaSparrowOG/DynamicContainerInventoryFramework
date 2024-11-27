#pragma once

#include "condition.h"

namespace Conditions
{
	class ReferenceCondition : public Condition
	{
	public:
		bool IsValid(RE::TESObjectREFR* a_container) override;

		ReferenceCondition(std::vector<RE::FormID> a_references);

		void Print() override;
	private:
		std::vector<RE::FormID> validReferences;
	};
}