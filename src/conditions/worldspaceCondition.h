#pragma once

#include "condition.h"

namespace Conditions
{
	class WorldspaceCondition : public Condition
	{
	public:
		bool IsValid(RE::TESObjectREFR* a_container) override;

		WorldspaceCondition(std::vector<RE::TESWorldSpace*> a_worldspaces);

		void Print() override;
	private:
		std::vector<RE::TESWorldSpace*> validWorldSpaces;
	};
}