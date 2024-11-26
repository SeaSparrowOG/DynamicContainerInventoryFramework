#pragma once

#include "condition.h"

namespace Conditions
{
	class LocationCondition : public Condition
	{
	public:
		bool IsValid(RE::TESObjectREFR* a_container) override;

	private:
		std::vector<RE::BGSLocation*> validLocations;
	};
}