#pragma once

namespace ContainerManager {
	struct SwapRule {
		int                              count               { 1 };
		bool                             bypassSafeEdits     { false };
		std::string                      ruleName            { std::string() };
		std::vector<std::string>         removeKeywords      { };
		RE::TESBoundObject*              oldForm             { nullptr };
		std::vector<std::string>         locationKeywords    { std::vector<std::string>() };
		std::vector<RE::TESObjectCONT*>  container           { std::vector<RE::TESObjectCONT*>() };
		std::vector<RE::BGSLocation*>    validLocations      { std::vector<RE::BGSLocation*>() };
		std::vector<RE::TESWorldSpace*>  validWorldspaces    { std::vector<RE::TESWorldSpace*>() };
		std::vector<RE::TESBoundObject*> newForm             { std::vector<RE::TESBoundObject*>() };
		std::vector<RE::FormID>          references          { std::vector<RE::FormID>() };
	};

	class ContainerManager : public clib_util::singleton::ISingleton<ContainerManager> {
	public:
		void CreateSwapRule(SwapRule a_rule);
		std::unordered_map<RE::FormID, std::pair<bool, float>>* GetHandledContainers();
		std::unordered_map < RE::FormID, bool>* GetHandledUnsafeContainers();
		uint32_t GetResetDays(bool a_short);
		void HandleContainer(RE::TESObjectREFR* a_ref);
		bool HasRuleApplied(RE::TESObjectREFR* a_ref, bool a_unsafeContainer);
		bool IsRuleValid(SwapRule* a_rule, RE::TESObjectREFR* a_ref);
		void InitializeData();
		void RegisterInMap(RE::TESObjectREFR* a_ref, bool a_cleared, float a_resetTime, bool a_unsafeContainer);
		void SetMaxLookupRange(float a_range);

	private:
		float fMaxLookupRadius;
		uint32_t fResetDaysLong;
		uint32_t fResetDaysShort;
		std::vector<SwapRule> addRules;
		std::vector<SwapRule> removeRules;
		std::vector<SwapRule> replaceRules;
		std::unordered_map<RE::FormID, std::pair<bool, float>> handledContainers;
		std::unordered_map < RE::FormID, bool> handledUnsafeContainers;
		std::unordered_map<RE::BGSLocation*, std::vector<RE::BGSLocation*>> parentLocations;
		std::unordered_map<RE::TESWorldSpace*, std::vector<RE::TESObjectREFR*>> worldspaceMarker;
	};
}