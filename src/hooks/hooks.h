#pragma once

#include "ClibUtil/rng.hpp"
#include "conditions/condition.h"
#include "utilities/utilities.h"

namespace Hooks {
	void Install();

	class ContainerManager : public Utilities::Singleton::ISingleton<ContainerManager> 
	{
	public:
		static void Install();

		RE::BGSLocation* GetNearestMarkerLocation(RE::TESObjectREFR* a_container);
		void RegisterDistance(float a_newDistance);
		void RegisterRule(Json::Value& raw, std::vector<size_t> a_conditions, bool a_safe, bool a_vendors, bool a_onlyVendors, bool a_random);
		void WarmCache();
		void PrettyPrint();

		std::vector<std::unique_ptr<Conditions::Condition>> storedConditions;
	private:
		struct Rule {
			std::vector<size_t> conditions;

			bool allowVendors;
			bool onlyVendors;
			bool allowSafeBypass;
			bool randomAdd;
			uint32_t ruleCount;

			bool PreCheck(RE::TESObjectREFR* a_container);
			virtual void Apply(RE::TESObjectREFR* a_container) = 0;
			virtual void Print() = 0;
		};

		struct AddRule : public Rule {
			std::vector<RE::TESBoundObject*> newForms;
			void Apply(RE::TESObjectREFR* a_container) override;
			void Print() override;
		};

		struct RemoveRule : public Rule {
			RE::TESBoundObject* form;
			void Apply(RE::TESObjectREFR* a_container) override;
			void Print() override;
		};

		struct RemoveKeywordRule : public Rule {
			std::vector<RE::BGSKeyword*> keywordsToRemove;
			void Apply(RE::TESObjectREFR* a_container) override;
			void Print() override;
		};

		struct ReplaceRule : public Rule {
			RE::TESBoundObject* oldForm;
			std::vector<RE::TESBoundObject*> newForms;
			void Apply(RE::TESObjectREFR* a_container) override;
			void Print() override;
		};

		struct ReplaceKeywordRule : public Rule {
			std::vector<RE::BGSKeyword*> keywordsToRemove;
			std::vector<RE::TESBoundObject*> newForms;
			void Apply(RE::TESObjectREFR* a_container) override;
			void Print() override;
		};

		static void Initialize(RE::TESObjectREFR* a_container, bool a3);
		static void Reset(RE::TESObjectREFR* a_container, bool a3);

		inline static REL::Relocation<decltype(&Initialize)> _initialize;
		inline static REL::Relocation<decltype(&Reset)> _reset;

		void ProcessContainer(RE::TESObjectREFR* a_container);

		std::vector<AddRule> adds;
		std::vector<RemoveRule> removes;
		std::vector<RemoveKeywordRule> removeKeywords;
		std::vector<ReplaceRule> replaces;
		std::vector<ReplaceKeywordRule> replaceKeywords;

		float maxLookupDistance;
		std::unordered_map<RE::TESWorldSpace*, std::vector<RE::TESObjectREFR*>> worldspaceMarkers;
	};
}