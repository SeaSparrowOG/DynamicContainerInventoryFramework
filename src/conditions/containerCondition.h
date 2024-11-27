#pragma once

#include "condition.h"

namespace Conditions
{
	class ContainerCondition : public Condition
	{
	public:
		bool IsValid(RE::TESObjectREFR* a_container) override;

		ContainerCondition(std::vector<RE::TESObjectCONT*> a_containers);

		void Print() override;
	private:
		std::vector<RE::TESObjectCONT*> validContainers;
	};
}