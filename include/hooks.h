#pragma once

namespace Hooks {
	struct TESObjectReference_Initialize
	{
		static bool   Install();
		static void   thunk(RE::TESObjectREFR* a_container, bool a_reset);
		inline static REL::Relocation<decltype(thunk)> func;
	};

	struct TESObjectReference_Reset
	{
		static bool   Install();
		static void   thunk(RE::TESObjectREFR* a_container, bool a_reset);
		inline static REL::Relocation<decltype(thunk)> func;
	};

	bool Install();
}