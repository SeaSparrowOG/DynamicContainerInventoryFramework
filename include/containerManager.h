#pragma once

namespace ContainerManager {
	struct SwapRule {
		std::vector<RE::TESBoundObject*> newForm;
		RE::TESBoundObject* oldForm;
		std::vector<std::string> locationIdentifiers;
	};

	class ContainerManager : public clib_util::singleton::ISingleton<ContainerManager> {
	public:
		bool CreateSwapRule(SwapRule a_rule);
		void HandleContainer(RE::TESObjectREFR* a_ref);

	private:
		std::vector<SwapRule> rules;
		std::unordered_map<RE::TESObjectREFR*, float> managedContainerMap;
	};
}