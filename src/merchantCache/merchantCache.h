#pragma once

#include "utilities/utilities.h"

namespace MerchantCache {
	class MerchantCache : public Utilities::Singleton::ISingleton<MerchantCache> {
	public:
		void BuildCache();
		bool IsMerchantContainer(RE::TESObjectREFR* a_container);
	private:
		std::unordered_map<RE::TESObjectREFR*, bool> storedReferences;
	};
}