#pragma once

namespace ContainerManager {
	struct QuestCondition {
		std::string questEDID;
		uint16_t questStage;
		bool questCompleted{ false };

		enum Comparison {
			Equal,
			NotEqual,
			Greater,
			GreaterOrEqual,
			Smaller,
			SmallerOrEqual,
			Completed,
			Invalid
		};
		Comparison comparisonValue{ Invalid };

		static Comparison GetComparison(std::string a_str) {
			if (a_str == "==") {
				return Comparison::Equal;
			}
			else if (a_str == "!=") {
				return Comparison::NotEqual;
			}
			else if (a_str == ">=") {
				return Comparison::GreaterOrEqual;
			}
			else if (a_str == ">") {
				return Comparison::Greater;
			}
			else if (a_str == "<=") {
				return Comparison::SmallerOrEqual;
			}
			else if (a_str == "<") {
				return Comparison::Smaller;
			}
			else {
				return Comparison::Invalid;
			}
		}

		bool IsValid() {
			if (comparisonValue == Invalid) return true;

			auto* quest = RE::TESForm::LookupByEditorID<RE::TESQuest>(questEDID);
			if (!quest) return false;

			switch (comparisonValue) {
			case Equal:
				return quest->GetCurrentStageID() == questStage;
			case NotEqual:
				return quest->GetCurrentStageID() != questStage;
			case Greater:
				return quest->GetCurrentStageID() > questStage;
			case GreaterOrEqual:
				return quest->GetCurrentStageID() >= questStage;
			case Smaller:
				return quest->GetCurrentStageID() < questStage;
			case SmallerOrEqual:
				return quest->GetCurrentStageID() <= questStage;
			case Completed:
				return quest->IsCompleted() == questCompleted;
			default:
				return false;
			}
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