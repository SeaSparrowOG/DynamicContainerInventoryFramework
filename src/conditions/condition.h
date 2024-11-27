#pragma once

namespace Conditions
{
	class Condition 
	{
	public:
		bool inverted;
		virtual bool IsValid(RE::TESObjectREFR* a_container) = 0;
	};
}