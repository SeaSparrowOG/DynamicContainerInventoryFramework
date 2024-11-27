#include "Hooks/hooks.h"

#include "merchantCache/merchantCache.h"
#include "utilities/utilities.h"
#include "RE/offset.h"

namespace Hooks {
	void Install()
	{
		ContainerManager::Install();
	}

	void ContainerManager::Install()
	{
		auto& trampoline = SKSE::GetTrampoline();
		REL::Relocation<std::uintptr_t> initializeTarget{ RE::Offset::TESObjectREFR::Initialize, 0x78C };
		REL::Relocation<std::uintptr_t> resetTarget{ RE::Offset::TESObjectREFR::Reset, 0x12B };

		_initialize = trampoline.write_call<5>(initializeTarget.address(), Initialize);
		_reset = trampoline.write_call<5>(resetTarget.address(), Reset);
	}

	void ContainerManager::RegisterRule(Json::Value& raw, std::vector<std::unique_ptr<Conditions::Condition>> a_conditions, bool a_safe, bool a_vendors, bool a_onlyVendors, bool a_random)
	{
		//json is valid here, checked in Settings::JSON::Read()
		const auto& add = raw["add"];
		const auto& remove = raw["remove"];
		const auto& removeKeywordsField = raw["removeByKeywords"];
		const auto& count = raw["count"];

		if (add && removeKeywordsField) {
			ReplaceKeywordRule newRule{};
			newRule.conditions = std::move(a_conditions);
			newRule.allowSafeBypass = a_safe;
			newRule.allowVendors = a_vendors;
			newRule.onlyVendors = a_onlyVendors;
			newRule.randomAdd = a_random;
			for (const auto& entry : add) {
				const auto obj = Utilities::Forms::GetFormFromString<RE::TESBoundObject>(entry.asString());
				newRule.newForms.push_back(obj);
			}
			for (const auto& entry : removeKeywordsField) {
				const auto keyword = Utilities::Forms::GetFormFromString<RE::BGSKeyword>(entry.asString());
				newRule.keywordsToRemove.push_back(keyword);
			}
		}
		else if (add && remove) {
			ReplaceRule newRule{};
			newRule.conditions = std::move(a_conditions);
			newRule.allowSafeBypass = a_safe;
			newRule.allowVendors = a_vendors;
			newRule.onlyVendors = a_onlyVendors;
			newRule.randomAdd = a_random;
			for (const auto& entry : add) {
				const auto obj = Utilities::Forms::GetFormFromString<RE::TESBoundObject>(entry.asString());
				newRule.newForms.push_back(obj);
			}
			for (const auto& entry : remove) {
				newRule.oldForm = Utilities::Forms::GetFormFromString<RE::TESBoundObject>(entry.asString());
			}
		}
		else if (removeKeywordsField) {
			RemoveKeywordRule newRule{};
			newRule.conditions = std::move(a_conditions);
			newRule.allowSafeBypass = a_safe;
			newRule.allowVendors = a_vendors;
			newRule.onlyVendors = a_onlyVendors;
			newRule.randomAdd = a_random;
			for (const auto& entry : removeKeywordsField) {
				const auto keyword = Utilities::Forms::GetFormFromString<RE::BGSKeyword>(entry.asString());
				newRule.keywordsToRemove.push_back(keyword);
			}
		}
		else if (remove) {
			RemoveRule newRule{};
			newRule.conditions = std::move(a_conditions);
			newRule.allowSafeBypass = a_safe;
			newRule.allowVendors = a_vendors;
			newRule.onlyVendors = a_onlyVendors;
			newRule.randomAdd = a_random;
			for (const auto& entry : remove) {
				newRule.form = Utilities::Forms::GetFormFromString<RE::TESBoundObject>(entry.asString());
			}

			if (count) {
				newRule.ruleCount = count.asUInt();
			}
			else {
				newRule.ruleCount = 0;
			}
		}
		else if (add) {
			AddRule newRule{};
			newRule.conditions = std::move(a_conditions);
			newRule.allowSafeBypass = a_safe;
			newRule.allowVendors = a_vendors;
			newRule.onlyVendors = a_onlyVendors;
			newRule.randomAdd = a_random;
			for (const auto& entry : add) {
				const auto obj = Utilities::Forms::GetFormFromString<RE::TESBoundObject>(entry.asString());
				newRule.newForms.push_back(obj);
			}

			if (count) {
				newRule.ruleCount = count.asUInt();
			}
			else {
				newRule.ruleCount = 0;
			}
		}
	}

	void ContainerManager::Initialize(RE::TESObjectREFR* a_container, bool a3)
	{
		_initialize(a_container, a3);
		if (a_container && a_container->GetBaseObject() && a_container->GetBaseObject()->As<RE::TESObjectCONT>()) {
			ContainerManager::GetSingleton()->ProcessContainer(a_container);
		}
	}

	void ContainerManager::Reset(RE::TESObjectREFR* a_container, bool a3)
	{
		_reset(a_container, a3);
		if (a_container && a_container->GetBaseObject() && a_container->GetBaseObject()->As<RE::TESObjectCONT>()) {
			ContainerManager::GetSingleton()->ProcessContainer(a_container);
		}
	}

	void ContainerManager::ProcessContainer(RE::TESObjectREFR* a_container)
	{
		for (auto& add : adds) {
			add.Apply(a_container);
		}
		for (auto& remove : removes) {
			remove.Apply(a_container);
		}
		for (auto& remove : removeKeywords) {
			remove.Apply(a_container);
		}
		for (auto& replace : replaces) {
			replace.Apply(a_container);
		}
		for (auto& replace : replaceKeywords) {
			replace.Apply(a_container);
		}
	}

	void ContainerManager::AddRule::Apply(RE::TESObjectREFR* a_container)
	{		
		if (!PreCheck(a_container)) {
			return;
		}

		size_t count = ruleCount;
		if (count == 0) {
			count = 1;
		}
		if (randomAdd) {
			for (auto i = (size_t)0; i < count; ++i) {
				const auto index = clib_util::RNG().generate((size_t)0, newForms.size() - 1);
				a_container->AddObjectToContainer(newForms.at(index), nullptr, 1, nullptr);
			}
		}
		else {
			for (const auto& baseObj : newForms) {
				a_container->AddObjectToContainer(baseObj, nullptr, count, nullptr);
			}
		}
	}

	void ContainerManager::RemoveRule::Apply(RE::TESObjectREFR* a_container)
	{
		auto inventory = a_container->GetInventory();
		if (!inventory.contains(form)) {
			return;
		}
		if (!PreCheck(a_container)) {
			return;
		}

		int32_t countToRemove = ruleCount;
		if (countToRemove == 0 || countToRemove > inventory[form].first) {
			countToRemove = inventory[form].first;
		}
		a_container->RemoveItem(form, countToRemove, RE::ITEM_REMOVE_REASON::kRemove, nullptr, nullptr);
	}

	void ContainerManager::ReplaceRule::Apply(RE::TESObjectREFR* a_container)
	{
		auto inventory = a_container->GetInventory();
		if (!inventory.contains(oldForm)) {
			return;
		}
		if (!PreCheck(a_container)) {
			return;
		}
		int32_t count = inventory[oldForm].first;
		a_container->RemoveItem(oldForm, count, RE::ITEM_REMOVE_REASON::kRemove, nullptr, nullptr);
		if (randomAdd) {
			for (auto i = (size_t)0; i < count; ++i) {
				const auto index = clib_util::RNG().generate((size_t)0, newForms.size() - 1);
				a_container->AddObjectToContainer(newForms.at(index), nullptr, 1, nullptr);
			}
		}
		else {
			for (const auto& baseObj : newForms) {
				a_container->AddObjectToContainer(baseObj, nullptr, count, nullptr);
			}
		}
	}

	bool ContainerManager::Rule::PreCheck(RE::TESObjectREFR* a_container)
	{
		const auto owner = a_container->GetFactionOwner();
		auto isMerchant = owner ? owner->IsVendor() : false;
		if (!isMerchant) {
			isMerchant = MerchantCache::MerchantCache::GetSingleton()->IsMerchantContainer(a_container);
		}
		if (isMerchant && !allowVendors) {
			return false;
		}
		if (!isMerchant && onlyVendors) {
			return false;
		}

		const auto containerBase = a_container->GetBaseObject()->As<RE::TESObjectCONT>();
		bool isSafeContainer = false;
		if (containerBase && !(containerBase->data.flags & RE::CONT_DATA::Flag::kRespawn)) {
			isSafeContainer = true;
		}

		bool hasParentCell = a_container->parentCell ? true : false;
		auto* parentEncounterZone = hasParentCell ? a_container->parentCell->extraList.GetEncounterZone() : nullptr;
		if (parentEncounterZone && parentEncounterZone->data.flags & RE::ENCOUNTER_ZONE_DATA::Flag::kNeverResets) {
			isSafeContainer = true;
		}

		if (isSafeContainer && !allowSafeBypass) {
			return false;
		}

		for (auto& condition : conditions) {
			if (!condition->IsValid(a_container)) {
				return false;
			}
		}

		return true;
	}

	void ContainerManager::RemoveKeywordRule::Apply(RE::TESObjectREFR* a_container)
	{
		auto inventory = a_container->GetInventory();
		if (inventory.empty()) {
			return;
		}

		//This is a awkward one. Results are mixed on whether it is faster to iterate through the inventory
		//first or the rules. When I get around to the rule matching algorithm, rules should be faster on
		//average.
		std::vector<std::pair<RE::TESBoundObject*, uint32_t>> removals{};
		for (const auto& inventoryEntry : inventory) {
			bool shouldSkip = false;
			for (auto it = keywordsToRemove.begin(); !shouldSkip && it != keywordsToRemove.end(); ++it) {
				const auto keywordForm = inventoryEntry.first->As<RE::BGSKeywordForm>();
				if (!keywordForm || !keywordForm->HasKeyword(*it)) {
					shouldSkip = true;
				}
			}
			if (shouldSkip) {
				continue;
			}

			removals.push_back({ inventoryEntry.first, inventoryEntry.second.first });
		}
		if (removals.empty()) {
			return;
		}
		if (!PreCheck(a_container)) {
			return;
		}

		for (const auto& pair : removals) {
			a_container->RemoveItem(pair.first, pair.second, RE::ITEM_REMOVE_REASON::kRemove, nullptr, nullptr);
		}
	}

	void ContainerManager::ReplaceKeywordRule::Apply(RE::TESObjectREFR* a_container)
	{
		auto inventory = a_container->GetInventory();
		if (inventory.empty()) {
			return;
		}

		size_t count = (size_t)0;
		std::vector<std::pair<RE::TESBoundObject*, uint32_t>> removals{};
		for (const auto& inventoryEntry : inventory) {
			bool shouldSkip = false;
			for (auto it = keywordsToRemove.begin(); !shouldSkip && it != keywordsToRemove.end(); ++it) {
				const auto keywordForm = inventoryEntry.first->As<RE::BGSKeywordForm>();
				if (!keywordForm || !keywordForm->HasKeyword(*it)) {
					shouldSkip = true;
				}
			}
			if (shouldSkip) {
				continue;
			}

			removals.push_back({ inventoryEntry.first, inventoryEntry.second.first });
			count += inventoryEntry.second.first;
		}
		if (removals.empty()) {
			return;
		}
		if (!PreCheck(a_container)) {
			return;
		}

		for (const auto& pair : removals) {
			a_container->RemoveItem(pair.first, pair.second, RE::ITEM_REMOVE_REASON::kRemove, nullptr, nullptr);
		}
		if (randomAdd) {
			for (auto i = (size_t)0; i < count; ++i) {
				const auto index = clib_util::RNG().generate((size_t)0, newForms.size() - 1);
				a_container->AddObjectToContainer(newForms.at(index), nullptr, 1, nullptr);
			}
		}
		else {
			for (const auto obj : newForms) {
				a_container->AddObjectToContainer(obj, nullptr, count, nullptr);
			}
		}
	}
}