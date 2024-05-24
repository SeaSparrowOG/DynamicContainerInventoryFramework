#pragma once

namespace ContainerManager {
	struct SwapRule {
		int                              count               { 1 };
		std::string                      ruleName            { std::string() };
		std::string                      removeKeyword       { std::string() };
		RE::TESBoundObject*              oldForm             { nullptr };
		std::vector<std::string>         locationKeywords    { std::vector<std::string>() };
		std::vector<RE::TESObjectCONT*>  container           { std::vector<RE::TESObjectCONT*>() };
		std::vector<RE::BGSLocation*>    validLocations      { std::vector<RE::BGSLocation*>() };
		std::vector<RE::TESBoundObject*> newForm             { std::vector<RE::TESBoundObject*>() };
		std::vector<RE::FormID>          references          { std::vector<RE::FormID>() };
	};

	class ContainerManager : public clib_util::singleton::ISingleton<ContainerManager> {
	public:
		void CreateSwapRule(SwapRule a_rule);
		void HandleContainer(RE::TESObjectREFR* a_ref);
		bool HasRuleApplied(RE::TESObjectREFR* a_ref);
		bool IsRuleValid(SwapRule* a_rule, RE::TESObjectREFR* a_ref);
		void InitializeData();

		std::vector<SwapRule> addRules;
		std::vector<SwapRule> removeRules;
		std::vector<SwapRule> replaceRules;
		std::unordered_map<RE::TESObjectREFR*, float> handledContainers;
		std::unordered_map<RE::BGSLocation*, std::vector<RE::BGSLocation*>> parentLocations;
	};
}