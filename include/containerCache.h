#pragma once

namespace ContainerCache {
#define continueContainer RE::BSContainer::ForEachResult::kContinue

	class CachedData : public clib_util::singleton::ISingleton<CachedData> {
	public:
		bool                              BuildCache();
		std::vector<RE::TESBoundObject*>* GetContainerItems(RE::TESObjectCONT* a_container);
	private:
		std::unordered_map<RE::TESObjectCONT*, std::vector<RE::TESBoundObject*>> cachedContainers;
	};
}