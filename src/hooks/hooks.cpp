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
		auto inventory = a_container->GetInventory();
		for (auto& add : adds) {
			add.Apply(a_container, inventory);
		}
		for (auto& remove : removes) {
			remove.Apply(a_container, inventory);
		}
		for (auto& replace : replaces) {
			replace.Apply(a_container, inventory);
		}
	}

	void ContainerManager::AddRule::Apply(RE::TESObjectREFR* a_container, RE::TESObjectREFR::InventoryItemMap& a_inventory)
	{		
		(void)a_inventory;
		if (!PreCheck(a_container)) {
			return;
		}
		for (const auto& baseObj : newForms) {
			a_container->AddObjectToContainer(baseObj, nullptr, count, nullptr);
		}
	}

	void ContainerManager::RemoveRule::Apply(RE::TESObjectREFR* a_container, RE::TESObjectREFR::InventoryItemMap& a_inventory)
	{
		if (!a_inventory.contains(form)) {
			return;
		}
		if (!PreCheck(a_container)) {
			return;
		}
		int32_t countToRemove = count;
		if (countToRemove == 0) {
			countToRemove = a_inventory[form].first;
		}
		a_container->RemoveItem(form, countToRemove, RE::ITEM_REMOVE_REASON::kRemove, nullptr, nullptr);
	}

	void ContainerManager::ReplaceRule::Apply(RE::TESObjectREFR* a_container, RE::TESObjectREFR::InventoryItemMap& a_inventory)
	{
		if (!a_inventory.contains(oldForm)) {
			return;
		}
		if (!PreCheck(a_container)) {
			return;
		}
		int32_t count = a_inventory[oldForm].first;
		a_container->RemoveItem(oldForm, count, RE::ITEM_REMOVE_REASON::kRemove, nullptr, nullptr);
		for (const auto& baseObj : newForms) {
			a_container->AddObjectToContainer(baseObj, nullptr, count, nullptr);
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
			if (!condition.IsValid(a_container)) {
				return false;
			}
		}

		return true;
	}
}