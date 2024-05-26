#include "containerManager.h"

namespace {
	void GetParentChain(RE::BGSLocation* a_child, std::vector<RE::BGSLocation*>* a_parentArray) {
		auto* parent = a_child->parentLoc;
		if (!parent) return;

		if (std::find(a_parentArray->begin(), a_parentArray->end(), parent) != a_parentArray->end()) {
			_loggerError("IMPORTANT - Recursive parent locations. Consider fixing this.");
			_loggerError("Chain:");
			for (auto* location : *a_parentArray) {
				_loggerError("    {} ->", location->GetName());
			}
			return;
		}
		//_loggerInfo("    >{}", clib_util::editorID::get_editorID(parent));
		a_parentArray->push_back(parent);
		return GetParentChain(parent, a_parentArray);
	}

	uint32_t HandleContainerLeveledList(RE::TESLeveledList* a_leveledList, RE::TESObjectREFR* a_container, std::vector<std::string> a_keywords) {
		uint32_t response = 0;
		auto forms = a_leveledList->GetContainedForms();

		for (auto* form : forms) {
			auto* leveledForm = form->As<RE::TESLeveledList>();
			if (leveledForm) {
				response += HandleContainerLeveledList(leveledForm, a_container, a_keywords);
			}
			else {
				auto* boundObject = static_cast<RE::TESBoundObject*>(form);
				if (!boundObject) continue;
				bool missingKeyword = false;
				for (auto it = a_keywords.begin(); it != a_keywords.end() && !missingKeyword; ++it) {
					if (boundObject->HasKeywordByEditorID(*it)) continue;
					missingKeyword = true;
				}
				if (missingKeyword) continue;

				auto inventoryCount = a_container->GetInventoryCounts().find(boundObject) != a_container->GetInventoryCounts().end() ? 
					a_container->GetInventoryCounts()[boundObject] : 0;
				if (inventoryCount < 1) continue;

				a_container->RemoveItem(boundObject, inventoryCount, RE::ITEM_REMOVE_REASON::kRemove, nullptr, nullptr);
				response += inventoryCount;
			}
		}

		return response;
	}

	/*
	Credit: ThirdEyeSqueegee
	Github: https://github.com/ThirdEyeSqueegee/ContainerItemDistributor/blob/d9aae0a5e30e81db885cc28f3fcf7e11a2f97bf6/include/Utility.h#L124
	Nexus: https://next.nexusmods.com/profile/ThirdEye3301/about-me?gameId=1704
	*/
	static auto AddLeveledListToContainer(RE::TESLeveledList* list, int16_t a_count, RE::TESObjectREFR* a_container) {
		RE::BSScrapArray<RE::CALCED_OBJECT> calced_objects{};
		list->CalculateCurrentFormList(RE::PlayerCharacter::GetSingleton()->GetLevel(), a_count, calced_objects, 0, true);

		std::vector<std::pair<RE::TESBoundObject*, int>> obj_and_counts{};
		for (const auto& [form, count, pad0A, pad0C, containerItem] : calced_objects) {
			if (const auto bound_obj{ form->As<RE::TESBoundObject>() }) {
				a_container->AddObjectToContainer(bound_obj, nullptr, count, nullptr);
			}
		}
		return obj_and_counts;
	}
}

namespace ContainerManager {
	bool ContainerManager::IsRuleValid(SwapRule* a_rule, RE::TESObjectREFR* a_ref) {
		auto* refLoc = a_ref->GetCurrentLocation();
		auto* refWorldspace = a_ref->GetWorldspace();

		bool hasWorldspaceLocation = a_rule->validWorldspaces.empty() ? true : false;
		if (!hasWorldspaceLocation && refWorldspace) {
			if (std::find(a_rule->validWorldspaces.begin(), a_rule->validWorldspaces.end(), refWorldspace) != a_rule->validWorldspaces.end()) {
				hasWorldspaceLocation = true;
			}
		}

		//Parent Location check. If the current location is NOT a match, this finds its parents.
		bool hasParentLocation = a_rule->validLocations.empty() ? true : false;

		if (!hasParentLocation && refLoc) {
			if (std::find(a_rule->validLocations.begin(), a_rule->validLocations.end(), refLoc) != a_rule->validLocations.end()) {
				hasParentLocation = true;
			}

			//Check parents.
			auto settingsParents = this->parentLocations.find(refLoc) != this->parentLocations.end() ? this->parentLocations[refLoc] : std::vector<RE::BGSLocation*>();
			RE::BGSLocation* parent = refLoc->parentLoc;
			for (auto it = settingsParents.begin(); it != settingsParents.end() && !hasParentLocation && parent; ++it) {
				if (std::find(a_rule->validLocations.begin(), a_rule->validLocations.end(), parent) != a_rule->validLocations.end()) {
					hasParentLocation = true;
				}
				parent = parent->parentLoc;
			}
		}
		else if (!hasParentLocation && !refLoc) {
			if (this->worldspaceMarker.find(refWorldspace) != this->worldspaceMarker.end()) {
				for (auto marker : this->worldspaceMarker[refWorldspace]) {
					if (marker->GetPosition().GetDistance(a_ref->GetPosition()) > this->fMaxLookupRadius) continue;
					auto* markerLoc = marker->GetCurrentLocation();
					if (std::find(a_rule->validLocations.begin(), a_rule->validLocations.end(), markerLoc) != a_rule->validLocations.end()) {
						hasParentLocation = true;
						break;
					}

					//Check parents.
					auto settingsParents = this->parentLocations.find(markerLoc) != this->parentLocations.end() ? this->parentLocations[markerLoc] : std::vector<RE::BGSLocation*>();
					RE::BGSLocation* parent = markerLoc->parentLoc;
					for (auto it = settingsParents.begin(); it != settingsParents.end() && !hasParentLocation && parent; ++it) {
						if (std::find(a_rule->validLocations.begin(), a_rule->validLocations.end(), parent) != a_rule->validLocations.end()) {
							hasParentLocation = true;
							break;
						}
						parent = parent->parentLoc;
					}
				}
			}
		}
		

		//Location keyword search. If these do not exist, check the parents..
		bool hasLocationKeywordMatch = a_rule->locationKeywords.empty() ? true : false;
		if (!hasLocationKeywordMatch && refLoc) {
			for (auto& locationKeyword : a_rule->locationKeywords) {
				if (refLoc->HasKeywordString(locationKeyword)) {
					hasLocationKeywordMatch = true;
				}
			}

			//Check parents.
			if (!hasLocationKeywordMatch) {
				auto refParentLocs = this->parentLocations.find(refLoc) != this->parentLocations.end() ?
					this->parentLocations[refLoc] : std::vector<RE::BGSLocation*>();

				for (auto it = refParentLocs.begin(); it != refParentLocs.end() && hasLocationKeywordMatch; ++it) {
					for (auto& locationKeyword : a_rule->locationKeywords) {
						if ((*it)->HasKeywordString(locationKeyword)) {
							hasLocationKeywordMatch = true;
						}
					}
				}
			}
		}

		//Reference check.
		bool hasReferenceMatch = a_rule->references.empty() ? true : false;
		if (!hasReferenceMatch) {
			std::stringstream stream;
			stream << std::hex << a_ref->formID;
			if (std::find(a_rule->references.begin(), a_rule->references.end(), a_ref->formID) != a_rule->references.end()) {
				hasReferenceMatch = true;
			}
		}

		//Container check.
		bool hasContainerMatch = a_rule->container.empty() ? true : false;
		auto* refBaseContainer = a_ref->GetBaseObject()->As<RE::TESObjectCONT>();
		if (!hasContainerMatch && refBaseContainer) {
			for (auto& container : a_rule->container) {
				if (refBaseContainer == container) {
					hasContainerMatch = true;
				}
			}
		}

		return (hasReferenceMatch && hasLocationKeywordMatch && hasParentLocation && hasContainerMatch && hasWorldspaceLocation);
	}

	bool ContainerManager::HasRuleApplied(RE::TESObjectREFR* a_ref) {
		if (this->handledContainers.find(a_ref) != this->handledContainers.end()) {
			auto& dayAttached = this->handledContainers[a_ref];
			if (RE::Calendar::GetSingleton()->GetDaysPassed() > dayAttached + 10.0f) return false;
			return false;
		}
		return true;
	}

	void ContainerManager::CreateSwapRule(SwapRule a_rule) {
		if (a_rule.removeKeywords.empty()) {
			if (!a_rule.oldForm) {
				this->addRules.push_back(a_rule);
				_loggerInfo("Registered bew <Add> rule.");
				_loggerInfo("    Form(s) that can be added:");
				for (auto form : a_rule.newForm) {
					_loggerInfo("        >{}.", form->GetName());
				}
				if (a_rule.count < 1) {
					_loggerInfo("    Ammount to be added: {}.", 1);
				}
				else {
					_loggerInfo("    Ammount to be added: {}.", a_rule.count);
				}
				_loggerInfo("----------------------------------------------");
			}
			else if (a_rule.newForm.empty()) {
				this->removeRules.push_back(a_rule);

				_loggerInfo("Registered bew <Remove> rule.");
				_loggerInfo("    Form that will be removed: {}.", a_rule.oldForm->GetName());
				if (a_rule.count < 1) {
					_loggerInfo("    All instances of the item will be removed.");
				}
				else {
					_loggerInfo("    Ammount to be removed: {}.", a_rule.count);
				}
				_loggerInfo("----------------------------------------------");
			}
			else {
				this->replaceRules.push_back(a_rule);

				_loggerInfo("Registered bew <Replace> rule.");
				_loggerInfo("    Form(s) that can be added:");
				for (auto form : a_rule.newForm) {
					_loggerInfo("        >{}.", form->GetName());
				}
				_loggerInfo("    Form that will be removed: {}.", a_rule.oldForm->GetName());
				_loggerInfo("----------------------------------------------");
			}
		}
		else {
			if (!a_rule.newForm.empty()) {
				this->replaceRules.push_back(a_rule);
				_loggerInfo("Registered bew <Replace> rule.");
				_loggerInfo("    Form(s) that can be added:");
				for (auto form : a_rule.newForm) {
					_loggerInfo("        >{}.", form->GetName());
				}
				_loggerInfo("    Items marked with these keywords will be removed:");
				for (auto& keyword : a_rule.removeKeywords) {
					_loggerInfo("        >{}", keyword);
				}
				_loggerInfo("----------------------------------------------");
			}
			else {
				this->removeRules.push_back(a_rule);

				_loggerInfo("Registered bew <Remove> rule.");
				_loggerInfo("    Items marked with these keywords will be removed:");
				for (auto& keyword : a_rule.removeKeywords) {
					_loggerInfo("        >{}", keyword);
				}
				_loggerInfo("----------------------------------------------");
			}
		}
	}

	void ContainerManager::HandleContainer(RE::TESObjectREFR* a_ref) {
		float daysPassed = RE::Calendar::GetSingleton()->GetDaysPassed();
		this->handledContainers[a_ref] = daysPassed;

		bool isVendorContainer = false;
		auto* ownerFaction = a_ref->GetFactionOwner();
		if (ownerFaction && ownerFaction->IsVendor()) {
			isVendorContainer = true;
		}

		if (HasRuleApplied(a_ref)) return;

		for (auto& rule : this->replaceRules) {
			if (isVendorContainer) continue;
			if (!IsRuleValid(&rule, a_ref)) continue;

			if (rule.removeKeywords.empty()) {
				auto itemCount = a_ref->GetInventoryCounts()[rule.oldForm];
				if (itemCount < 1) continue;

				size_t rng = clib_util::RNG().generate<size_t>(0, rule.newForm.size() - 1);
				RE::TESBoundObject* thingToAdd = rule.newForm.at(rng);
				a_ref->RemoveItem(rule.oldForm, itemCount, RE::ITEM_REMOVE_REASON::kRemove, nullptr, nullptr);
				a_ref->AddObjectToContainer(thingToAdd, nullptr, itemCount, nullptr);
			}
			else {
				uint32_t itemCount = 0;
				auto* container = a_ref->GetContainer();
				if (!container) continue;

				container->ForEachContainerObject([&](RE::ContainerObject& a_obj) {
					auto* baseObj = a_obj.obj;
					if (!baseObj) return RE::BSContainer::ForEachResult::kContinue;

					auto* leveledBase = baseObj->As<RE::TESLeveledList>();
					if (!leveledBase) {
						bool missingKeyword = false;
						for (auto it = rule.removeKeywords.begin(); it != rule.removeKeywords.end() && !missingKeyword; ++it) {
							if (baseObj->HasKeywordByEditorID(*it)) continue;
							missingKeyword = true;
						}
						if (missingKeyword) return RE::BSContainer::ForEachResult::kContinue;

						itemCount += a_obj.count;
						a_ref->RemoveItem(baseObj, a_obj.count, RE::ITEM_REMOVE_REASON::kRemove, nullptr, nullptr);
					}
					else {
						itemCount += HandleContainerLeveledList(leveledBase, a_ref, rule.removeKeywords);
					}

					return RE::BSContainer::ForEachResult::kContinue;
				});

				for (uint32_t i = 0; i < itemCount; ++i) {
					size_t rng = clib_util::RNG().generate<size_t>(0, rule.newForm.size() - 1);
					RE::TESBoundObject* thingToAdd = rule.newForm.at(rng);
					auto* leveledThing = thingToAdd->As<RE::TESLeveledList>();
					if (leveledThing) {
						AddLeveledListToContainer(leveledThing, itemCount, a_ref);
					}
					else {
						a_ref->AddObjectToContainer(thingToAdd, nullptr, itemCount, nullptr);
					}
				}
			}
		} //Replace Rule reading end.

		for (auto& rule : this->removeRules) {
			if (isVendorContainer) continue;

			if (rule.removeKeywords.empty()) {
				auto itemCount = a_ref->GetInventoryCounts()[rule.oldForm];
				if (itemCount < 1) continue;
				if (!IsRuleValid(&rule, a_ref)) continue;

				int32_t ruleCount = rule.count;
				if (ruleCount > itemCount || ruleCount < 1) {
					ruleCount = itemCount;
				}
				a_ref->RemoveItem(rule.oldForm, ruleCount, RE::ITEM_REMOVE_REASON::kRemove, nullptr, nullptr);
			} 
			else {
				auto* container = a_ref->GetContainer();
				if (!container) continue;

				container->ForEachContainerObject([&](RE::ContainerObject& a_obj) {
					auto* baseObj = a_obj.obj;
					if (!baseObj) return RE::BSContainer::ForEachResult::kContinue;

					auto* leveledBase = baseObj->As<RE::TESLeveledList>();
					if (!leveledBase) {
						auto itemCount = a_ref->GetInventoryCounts()[baseObj];
						bool missingKeyword = false;
						for (auto it = rule.removeKeywords.begin(); it != rule.removeKeywords.end() && !missingKeyword; ++it) {
							if (baseObj->HasKeywordByEditorID(*it)) continue;
							missingKeyword = true;
						}
						if (missingKeyword) return RE::BSContainer::ForEachResult::kContinue;
						a_ref->RemoveItem(baseObj, a_obj.count, RE::ITEM_REMOVE_REASON::kRemove, nullptr, nullptr);
					}
					else {
						HandleContainerLeveledList(leveledBase, a_ref, rule.removeKeywords);
					}

					return RE::BSContainer::ForEachResult::kContinue;
				});
			}
		} //Remove rule reading end.

		for (auto& rule : this->addRules) {
			if (isVendorContainer) continue;
			if (!IsRuleValid(&rule, a_ref)) continue;

			int32_t ruleCount = rule.count;
			if (ruleCount < 1) {
				ruleCount = 1;
			}

			size_t rng = clib_util::RNG().generate<size_t>(0, rule.newForm.size() - 1);
			RE::TESBoundObject* thingToAdd = rule.newForm.at(rng);
			auto* leveledThing = thingToAdd->As<RE::TESLeveledList>();

			if (leveledThing) {
				AddLeveledListToContainer(leveledThing, ruleCount, a_ref);
			}
			else {
				a_ref->AddObjectToContainer(thingToAdd, nullptr, ruleCount, nullptr);
			}
		} //Add rule reading end.
	}

	void ContainerManager::InitializeData() {
		auto& locationArray = RE::TESDataHandler::GetSingleton()->GetFormArray<RE::BGSLocation>();
		for (auto* location : locationArray) {
			std::vector<RE::BGSLocation*> parents;
			auto* parentLocation = location->parentLoc;
			if (!parentLocation) continue;

			parents.push_back(parentLocation);
			//_loggerInfo("Location: {} - Parents:", clib_util::editorID::get_editorID(location));
			//_loggerInfo("    >{}", clib_util::editorID::get_editorID(parentLocation));
			GetParentChain(parentLocation, &parents);
			this->parentLocations[location] = parents;
		}

		auto& worldspaceArray = RE::TESDataHandler::GetSingleton()->GetFormArray<RE::TESWorldSpace>();
		for (auto* worldspace : worldspaceArray) {
			auto* persistentCell = worldspace->persistentCell;
			if (!persistentCell) continue;
			std::vector<RE::TESObjectREFR*> references{};

			persistentCell->ForEachReference([&](RE::TESObjectREFR* a_marker) {
				auto* markerLoc = a_marker->GetCurrentLocation();
				if (!markerLoc) return RE::BSContainer::ForEachResult::kContinue;
				if (!a_marker->extraList.GetByType(RE::ExtraDataType::kMapMarker)) return RE::BSContainer::ForEachResult::kContinue;
				references.push_back(a_marker);
				return RE::BSContainer::ForEachResult::kContinue;
			});

			this->worldspaceMarker[worldspace] = references;
		}
	}
}