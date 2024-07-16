#include "merchantFactionCache.h"

namespace MerchantCache {
	void MerchantCache::BuildCache()
	{
		auto* dataHandler = RE::TESDataHandler::GetSingleton();
		if (!dataHandler) return;

		const auto& factionArray = RE::TESDataHandler::GetSingleton() ? RE::TESDataHandler::GetSingleton()->GetFormArray<RE::TESFaction>() :
			RE::BSTArray<RE::TESFaction*, RE::BSTArrayHeapAllocator>();
		for (auto& faction : factionArray) {
			if (!faction->IsVendor()) continue;
			auto* vendorContainer = faction->vendorData.merchantContainer;
			if (!vendorContainer || storedReferences.contains(vendorContainer)) continue;

			storedReferences.try_emplace(vendorContainer, true);
		}
	}
	bool MerchantCache::IsMerchantContainer(RE::TESObjectREFR* a_container)
	{
		if (!a_container) return false;
		return storedReferences.contains(a_container);
	}
}