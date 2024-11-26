#pragma once

#include "conditions/condition.h"
#include "conditions/actorValueCondition.h"
#include "conditions/containerCondition.h"
#include "conditions/globalCondition.h"
#include "conditions/locationCondition.h"
#include "conditions/locationKeywordCondition.h"
#include "conditions/questCondition.h"
#include "conditions/referenceCondition.h"
#include "conditions/worldspaceCondition.h"
#include "utilities/utilities.h"

namespace Hooks {
	void Install();

	class ContainerManager : public Utilities::Singleton::ISingleton<ContainerManager> 
	{
	public:
		static void Install();

	private:
		struct Rule {
			std::vector<Conditions::Condition> conditions;

			bool allowVendors;
			bool onlyVendors;
			bool allowSafeBypass;
			bool randomAdd;

			bool PreCheck(RE::TESObjectREFR* a_container);
			virtual void Apply(RE::TESObjectREFR* a_container, RE::TESObjectREFR::InventoryItemMap& a_inventory) = 0;
		};

		struct AddRule : public Rule {
			std::int32_t count;
			std::vector<RE::TESBoundObject*> newForms;
			void Apply(RE::TESObjectREFR* a_container, RE::TESObjectREFR::InventoryItemMap& a_inventory);
		};

		struct RemoveRule : public Rule {
			int32_t count;
			RE::TESBoundObject* form;
			void Apply(RE::TESObjectREFR* a_container, RE::TESObjectREFR::InventoryItemMap& a_inventory);
		};

		struct ReplaceRule : public Rule {
			RE::TESBoundObject* oldForm;
			std::vector<RE::TESBoundObject*> newForms;
			void Apply(RE::TESObjectREFR* a_container, RE::TESObjectREFR::InventoryItemMap& a_inventory);
		};

		static void Initialize(RE::TESObjectREFR* a_container, bool a3);
		static void Reset(RE::TESObjectREFR* a_container, bool a3);

		inline static REL::Relocation<decltype(&Initialize)> _initialize;
		inline static REL::Relocation<decltype(&Reset)> _reset;

		void ProcessContainer(RE::TESObjectREFR* a_container);

		std::vector<AddRule> adds;
		std::vector<RemoveRule> removes;
		std::vector<ReplaceRule> replaces;
	};
}