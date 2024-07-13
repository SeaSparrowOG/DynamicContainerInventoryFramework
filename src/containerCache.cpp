#include "containerCache.h"

namespace {
	void ResolveContainerLeveledList(RE::TESLeveledList* a_list, std::vector<RE::TESBoundObject*>* a_result, std::vector<RE::TESForm*>* downstreamLists = nullptr) {
		auto forms = a_list->GetContainedForms();

		for (auto* form : forms) {
			auto* leveledForm = form->As<RE::TESLeveledList>();
			if (leveledForm) {
				if (!downstreamLists) {
					auto newBranch = std::vector<RE::TESForm*>();
					newBranch.push_back(form);
					ResolveContainerLeveledList(leveledForm, a_result, &newBranch);
				}
				else {
					if (std::find(downstreamLists->begin(), downstreamLists->end(), form) != downstreamLists->end()) {
						_loggerError("WARNING: Circular Leveled list found. This MUST be resolved, else you will have random crashes and fixes (even without this mod!)");
						_loggerError("Stack:");
						for (auto* list : *downstreamLists) {
							auto edid = clib_util::editorID::get_editorID(list);
							if (edid.empty()) continue;
							_loggerError("    >{}", edid);
						}
						continue;
					}
					downstreamLists->push_back(form);
					ResolveContainerLeveledList(leveledForm, a_result, downstreamLists);
				}
				
			}
			else {
				auto* boundObject = static_cast<RE::TESBoundObject*>(form);
				if (!boundObject) continue;
				if (std::find(a_result->begin(), a_result->end(), boundObject) != a_result->end()) continue;

				a_result->push_back(boundObject);
			}
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