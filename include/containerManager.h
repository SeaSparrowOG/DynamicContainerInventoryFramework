#pragma once

namespace ContainerManager {
	struct SwapRule {
		RE::TESBoundObject*              oldForm;
		std::vector<std::string>         locationKeywords;
		std::vector<RE::BGSLocation*>    validLocations;
		std::vector<RE::TESBoundObject*> newForm;
	};

	class ContainerManager : public clib_util::singleton::ISingleton<ContainerManager> {
	public:
		bool CreateSwapRule(SwapRule a_rule);
		void HandleContainer(RE::TESObjectREFR* a_ref);

	private:
		std::vector<SwapRule> rules;
	};
}