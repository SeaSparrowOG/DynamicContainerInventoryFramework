#include "globalCondition.h"

namespace Conditions
{
	bool GlobalCondition::IsValid(RE::TESObjectREFR* a_container)
	{
		(void)a_container;
		if (global->value >= value) {
			return !inverted;
		}
		return inverted;
	}

	GlobalCondition::GlobalCondition(RE::TESGlobal* a_global, float a_value)
	{
		this->global = a_global;
		this->value = a_value;
	}

	void GlobalCondition::Print()
	{
		logger::info("======================/");
		logger::info("|  Global Conditions /");
		logger::info("====================/");
		logger::info("  ->{}{}: [{}]", inverted ? "Not " : "", global->GetFormEditorID(), value);
		logger::info("");
	}
}