#pragma once

namespace Hooks {
	void Install();

	class ContainerManager {
	public:
		static void Install();

	private:
		static void Initialize(RE::TESObjectREFR* a_container, bool a3);
		static void Reset(RE::TESObjectREFR* a_container, bool a3);

		inline static REL::Relocation<decltype(&Initialize)> _initialize;
		inline static REL::Relocation<decltype(&Reset)> _reset;
	};
}