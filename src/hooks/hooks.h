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

		void RegisterRule(Json::Value& raw, std::vector<std::unique_ptr<Conditions::Condition>> a_conditions, bool a_safe, bool a_vendors, bool a_onlyVendors, bool a_random);

	private:
		struct Rule {
			std::vector<std::unique_ptr<Conditions::Condition>> conditions;

			bool allowVendors;
			bool onlyVendors;
			bool allowSafeBypass;
			bool randomAdd;
			uint32_t ruleCount;

			bool PreCheck(RE::TESObjectREFR* a_container);
			virtual void Apply(RE::TESObjectREFR* a_container) = 0;
		};

		struct AddRule : public Rule {
			std::vector<RE::TESBoundObject*> newForms;
			void Apply(RE::TESObjectREFR* a_container) override;
		};

		struct RemoveRule : public Rule {
			RE::TESBoundObject* form;
			void Apply(RE::TESObjectREFR* a_container) override;
		};

		struct RemoveKeywordRule : public Rule {
			std::vector<RE::BGSKeyword*> keywordsToRemove;
			void Apply(RE::TESObjectREFR* a_container) override;
		};

		struct ReplaceRule : public Rule {
			RE::TESBoundObject* oldForm;
			std::vector<RE::TESBoundObject*> newForms;
			void Apply(RE::TESObjectREFR* a_container) override;
		};

		struct ReplaceKeywordRule : public Rule {
			std::vector<RE::BGSKeyword*> keywordsToRemove;
			std::vector<RE::TESBoundObject*> newForms;
			void Apply(RE::TESObjectREFR* a_container) override;
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
	};
}