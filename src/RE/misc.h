#pragma once

#include "offset.h"

namespace RE
{
	inline bool IsQuestStageDone(RE::TESQuest* a_this, uint16_t a_stage) {
		using func_t = decltype(&IsQuestStageDone);
		static REL::Relocation<func_t> func{ Offset::Quest::IsStageDone };
		return func(a_this, a_stage);
	}
}