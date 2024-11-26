#pragma once

#include "condition.h"

namespace Conditions
{
	class AVCondition : public Condition
	{
	public:
		bool IsValid(RE::TESObjectREFR* a_container) override;
		AVCondition(const char* a_value, float a_minValue);

	private:
		const char* value;
		float minValue;
	};
}