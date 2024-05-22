#pragma once

namespace ContainerManager {
	struct SwapRule {
		int                              count               { 1 };
		bool                             distributeToVendors { false };
		RE::TESBoundObject*              oldForm             { nullptr };
		std::vector<std::string>         locationKeywords    { std::vector<std::string>() };
		std::vector<RE::TESObjectCONT*>  container           { std::vector<RE::TESObjectCONT*>() };
		std::vector<RE::BGSLocation*>    validLocations      { std::vector<RE::BGSLocation*>() };
		std::vector<RE::TESBoundObject*> newForm             { std::vector<RE::TESBoundObject*>() };
	};

	class ContainerManager : public clib_util::singleton::ISingleton<ContainerManager> {
	public:
		void CreateSwapRule(SwapRule a_rule);
		void HandleContainer(RE::TESObjectREFR* a_ref);

	private:
		std::vector<SwapRule> addRules;
		std::vector<SwapRule> removeRules;
		std::vector<SwapRule> replaceRules;
	};
}