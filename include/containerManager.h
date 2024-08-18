#pragma once

namespace ContainerManager {
	struct QuestCondition {
		std::string questEDID{};
		uint16_t questStageMin{ 0 };
		uint16_t questStageMax{ UINT16_MAX };
		bool questCompleted{ false };

		bool IsValid() {
			if (questEDID.empty()) return true;
			auto* quest = RE::TESForm::LookupByEditorID<RE::TESQuest>(questEDID);
			if (!quest) return false;

			if (questCompleted && quest->IsCompleted()) return true;
			return quest->currentStage >= questStageMin && quest->currentStage <= questStageMax;
		}
	};

	struct SwapRule {
		int                                                             count               { 1 };
		bool                                                            allowVendors        { false };
		bool                                                            onlyVendors         { false };
		bool                                                            bypassSafeEdits     { false };
		bool                                                            randomAdd           { false };
		std::string                                                     ruleName            { std::string() };
		QuestCondition                                                  questCondition      { };
		std::vector<std::string>                                        removeKeywords      { };
		RE::TESBoundObject*                                             oldForm             { nullptr };
		std::vector<std::string>                                        locationKeywords    { std::vector<std::string>() };
		std::vector<RE::TESObjectCONT*>                                 container           { std::vector<RE::TESObjectCONT*>() };
		std::vector<RE::BGSLocation*>                                   validLocations      { std::vector<RE::BGSLocation*>() };
		std::vector<RE::TESWorldSpace*>                                 validWorldspaces    { std::vector<RE::TESWorldSpace*>() };
		std::vector<RE::TESBoundObject*>                                newForm             { std::vector<RE::TESBoundObject*>() };
		std::vector<RE::FormID>                                         references          { std::vector<RE::FormID>() };
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