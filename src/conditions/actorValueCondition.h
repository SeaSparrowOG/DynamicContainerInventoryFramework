#pragma once

#include "condition.h"

namespace Conditions
{
	class AVCondition : public Condition
	{
	public:
		bool IsValid(RE::TESObjectREFR* a_container) override;
		AVCondition(RE::ActorValue a_value, float a_minValue);

	private:
		RE::ActorValue value;
		float minValue;
	};
}