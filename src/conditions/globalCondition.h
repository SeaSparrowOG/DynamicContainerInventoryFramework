#pragma once

#include "condition.h"

namespace Conditions
{
	class GlobalCondition : public Condition
	{
	public:
		bool IsValid(RE::TESObjectREFR* a_container) override;

	private:
		RE::TESGlobal* global;
		float value;
	};
}