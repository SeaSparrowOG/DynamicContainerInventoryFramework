#include "containerCache.h"

namespace {
	inline void ResolveContainerLeveledList(RE::TESLeveledList* a_list, std::vector<RE::TESBoundObject*>* a_result, std::vector<RE::TESForm*>* downstreamLists = nullptr) {
		//Note - As of this commit, GetContainedForms doesn't check if there is a circular leveled list
		//in PO3's clib. I <roughly> patched the issue in mine, but the solution is improper.
		//Also, my clib doesn't pass the same form pointer multiple times in the result (I don't think it's needed).
		auto forms = a_list->GetContainedForms();
		_loggerDebug("        Leveled list with {} forms.", forms.size());

		for (auto* form : forms) {
			if (!form || !form->IsBoundObject()) continue;
			auto* boundObject = static_cast<RE::TESBoundObject*>(form);
			a_result->push_back(boundObject);
		}
	}
}

namespace ContainerCache {
	bool CachedData::BuildCache() {
		auto& containerArray = RE::TESDataHandler::GetSingleton()->GetFormArray<RE::TESObjectCONT>();
		for (auto* container : containerArray) {
			container->ForEachContainerObject([&](RE::ContainerObject& a_obj) {
				auto* baseObject = a_obj.obj;
				if (!baseObject) return continueContainer;
				_loggerDebug("    >Object: {}", _debugEDID(baseObject));

				auto* baseLeveledList = baseObject->As<RE::TESLeveledList>();
				if (baseLeveledList) {
					std::vector<RE::TESBoundObject*> objects{};
					ResolveContainerLeveledList(baseLeveledList, &objects);
					for (auto* item : objects) {
						if (this->cachedContainers.contains(container)) {
							this->cachedContainers[container].push_back(item);
						}
						else {
							std::vector<RE::TESBoundObject*> newItems{ item };
							this->cachedContainers[container] = newItems;
						}
					}
				}
				else {
					if (this->cachedContainers.contains(container)) {
						this->cachedContainers[container].push_back(baseObject);
					}
					else {
						std::vector<RE::TESBoundObject*> newItems{ baseObject };
						this->cachedContainers[container] = newItems;
					}
				}
				return continueContainer;
			});
		}
		return true;
	}
	std::vector<RE::TESBoundObject*>* CachedData::GetContainerItems(RE::TESObjectCONT* a_container) {
		return &this->cachedContainers[a_container];
	}
}