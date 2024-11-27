#pragma once

#include "condition.h"

namespace Conditions
{
	class AVCondition : public Condition
	{
	public:
		bool IsValid(RE::TESObjectREFR* a_container) override;
		AVCondition(std::string a_value, float a_minValue);

		void Print() override;
	private:
		std::string value;
		float minValue;
	};
}