#pragma once

#include "condition.h"

namespace Conditions
{
	class GlobalCondition : public Condition
	{
	public:
		bool IsValid(RE::TESObjectREFR* a_container) override;

		GlobalCondition(RE::TESGlobal* a_global, float a_value);

		void Print() override;
	private:
		RE::TESGlobal* global;
		float value;
	};
}