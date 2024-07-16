#pragma once

namespace MerchantCache {
	class MerchantCache : public clib_util::singleton::ISingleton<MerchantCache> {
	public:
		void BuildCache();
		bool IsMerchantContainer(RE::TESObjectREFR* a_container);
	private:
		std::unordered_map<RE::TESObjectREFR*, bool> storedReferences;
	};
}