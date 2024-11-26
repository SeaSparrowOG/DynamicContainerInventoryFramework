#pragma once

#include "condition.h"

namespace Conditions
{
	class AVCondition : public Condition
	{
	public:
		bool IsValid(RE::TESObjectREFR* a_container) override;

	private:
		RE::ActorValue value;
		float minValue;
	};
}