#pragma once

#include "condition.h"

namespace Conditions
{
	class WorldspaceCondition : public Condition
	{
	public:
		bool IsValid(RE::TESObjectREFR* a_container) override;

	private:
		std::vector<RE::TESWorldSpace*> validWorldSpaces; //Also contains parent worldspaces
	};
}