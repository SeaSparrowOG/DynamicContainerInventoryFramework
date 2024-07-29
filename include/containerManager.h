#pragma once

namespace ContainerManager {
	struct SwapRule {
		int                                                             count               { 1 };
		bool                                                            allowVendors        { false };
		bool                                                            onlyVendors         { false };
		bool                                                            bypassSafeEdits     { false };

		/*
			If set to true, this reverts distribution to pre-2.0 behavior (one of the array items)
			But must be set explicitly. Therefore, if not set, 2.0+ behavior (distribute ALL) still applies.
		*/
		bool															isPickAtRandom		= false;


		std::string                                                     ruleName            { std::string() };
		std::vector<std::string>                                        removeKeywords      { };
		RE::TESBoundObject*                                             oldForm             { nullptr };
		std::vector<std::string>                                        locationKeywords    { std::vector<std::string>() };
		std::vector<RE::TESObjectCONT*>                                 container           { std::vector<RE::TESObjectCONT*>() };
		std::vector<RE::BGSLocation*>                                   validLocations      { std::vector<RE::BGSLocation*>() };
		std::vector<RE::TESWorldSpace*>                                 validWorldspaces    { std::vector<RE::TESWorldSpace*>() };
		std::vector<RE::TESBoundObject*>                                newForm             { std::vector<RE::TESBoundObject*>() };
		std::vector<RE::FormID>                                         references          { std::vector<RE::FormID>() };

		std::vector<QuestCondition>										requiredQuestStages { std::vector<QuestCondition>() };

		std::vector<std::pair<RE::ActorValue, std::pair<float, float>>> requiredAVs         { std::vector<std::pair<RE::ActorValue, std::pair<float, float>>>() };
		std::vector<std::pair<RE::TESGlobal*, float>>                   requiredGlobalValues{ std::vector<std::pair<RE::TESGlobal*, float>>() };
	};

	class ContainerManager : public clib_util::singleton::ISingleton<ContainerManager> {
	public:
		void CreateSwapRule(SwapRule a_rule);
		void HandleContainer(RE::TESObjectREFR* a_ref);
		bool IsRuleValid(SwapRule* a_rule, RE::TESObjectREFR* a_ref);
		void InitializeData();
		void SetMaxLookupRange(float a_range);

	private:
		float fMaxLookupRadius;
		std::vector<SwapRule> addRules;
		std::vector<SwapRule> removeRules;
		std::vector<SwapRule> replaceRules;
		std::unordered_map<RE::BGSLocation*, std::vector<RE::BGSLocation*>> parentLocations;
		std::unordered_map<RE::TESWorldSpace*, std::vector<RE::TESObjectREFR*>> worldspaceMarker;
	};
}