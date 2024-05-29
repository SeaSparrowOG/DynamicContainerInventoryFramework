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

	uint32_t HandleContainerLeveledList(RE::TESLeveledList* a_leveledList, RE::TESObjectREFR* a_container, std::vector<std::string> a_keywords, int32_t a_count) {
		uint32_t response = 0;
		auto forms = a_leveledList->GetContainedForms();

		for (auto* form : forms) {
			auto* leveledForm = form->As<RE::TESLeveledList>();
			if (leveledForm) {
				response += HandleContainerLeveledList(leveledForm, a_container, a_keywords, a_count);
			}
			else {
				auto* boundObject = static_cast<RE::TESBoundObject*>(form);
				if (!boundObject) continue;

				auto inventoryCount = a_container->GetInventoryCounts().find(boundObject) != a_container->GetInventoryCounts().end() ?
					a_container->GetInventoryCounts()[boundObject] : 0;
				if (inventoryCount < 1) continue;

				bool missingKeyword = false;
				for (auto it = a_keywords.begin(); it != a_keywords.end() && !missingKeyword; ++it) {
					if (boundObject->HasKeywordByEditorID(*it)) continue;
					missingKeyword = true;
				}
				if (missingKeyword) continue;

				if (inventoryCount > a_count && a_count > 0) {
					inventoryCount = a_count;
				}
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
	static void ResolveLeveledList(RE::TESLeveledList* a_levItem, RE::BSScrapArray<RE::CALCED_OBJECT>* a_result) {
		RE::BSScrapArray<RE::CALCED_OBJECT> temp{};
		a_levItem->CalculateCurrentFormList(RE::PlayerCharacter::GetSingleton()->GetLevel(), 1, temp, 0, true);

		for (auto& it : temp) {
			auto* form = it.form;
			auto* leveledForm = form->As<RE::TESLeveledList>();
			if (leveledForm) {
				ResolveLeveledList(leveledForm, a_result);
			}
			else {
				a_result->push_back(it);
			}
		}
	}

	static void AddLeveledListToContainer(RE::TESLeveledList* list, RE::TESObjectREFR* a_container) {
		RE::BSScrapArray<RE::CALCED_OBJECT> result{};
		ResolveLeveledList(list, &result);
		if (result.size() < 1) return;

		for (auto& obj : result) {
			auto* thingToAdd = static_cast<RE::TESBoundObject*>(obj.form);
			if (!thingToAdd) continue;
			a_container->AddObjectToContainer(thingToAdd, nullptr, obj.count, nullptr);
		}
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
					if (!markerLoc) continue;
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

	bool ContainerManager::HasRuleApplied(RE::TESObjectREFR* a_ref, bool a_unsafeContainer) {
		float daysPassed = RE::Calendar::GetSingleton()->GetDaysPassed();
		float dayToStore = daysPassed;
		bool cleared = a_ref->GetCurrentLocation() ? a_ref->GetCurrentLocation()->IsCleared() : false;
		if (cleared) {
			dayToStore += this->fResetDaysLong;
		}
		else {
			dayToStore += this->fResetDaysShort;
		}

		bool returnVal = false;

		if (!a_unsafeContainer) {
			if (this->handledContainers.contains(a_ref->formID)) {
				float currentDay = RE::Calendar::GetSingleton()->GetDaysPassed();
				auto resetDay = this->handledContainers[a_ref->formID].second;

				if (currentDay > resetDay) {
					returnVal = true;
				}
			}
		}
		else {
			if (this->handledUnsafeContainers.contains(a_ref->formID)) {
				returnVal = true;
			}
		}

		RegisterInMap(a_ref, cleared, dayToStore, a_unsafeContainer);
		return returnVal;
	}

	void ContainerManager::CreateSwapRule(SwapRule a_rule) {
		if (a_rule.removeKeywords.empty()) {
			if (!a_rule.oldForm) {
				this->addRules.push_back(a_rule);
				_loggerInfo("Registered new <Add> rule.");
				_loggerInfo("    Form(s) that can be added:");
				for (auto form : a_rule.newForm) {
					_loggerInfo("        >{}.", form->GetName());
				}
				if (a_rule.count < 1) {
					a_rule.count = 1;
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
		auto* ownerFaction = a_ref->GetFactionOwner();
		if (ownerFaction && ownerFaction->IsVendor()) {
			return;
		}

		bool isContainerUnsafe = false;
		auto* containerBase = a_ref->GetBaseObject()->As<RE::TESObjectCONT>();
		if (containerBase && !(containerBase->data.flags & RE::CONT_DATA::Flag::kRespawn)) {
			isContainerUnsafe = true;
		}
		auto* parentEncounterZone = a_ref->parentCell->extraList.GetEncounterZone();
		if (parentEncounterZone && parentEncounterZone->data.flags & RE::ENCOUNTER_ZONE_DATA::Flag::kNeverResets) {
			isContainerUnsafe = true;
		}

		if (HasRuleApplied(a_ref, isContainerUnsafe)) return;

		for (auto& rule : this->replaceRules) {
			if (!rule.bypassSafeEdits && isContainerUnsafe) continue;
			if (!IsRuleValid(&rule, a_ref)) continue;

			if (rule.removeKeywords.empty()) {
				auto itemCount = a_ref->GetInventoryCounts()[rule.oldForm];
				if (itemCount < 1) continue;

				size_t rng = clib_util::RNG().generate<size_t>(0, rule.newForm.size() - 1);
				RE::TESBoundObject* thingToAdd = rule.newForm.at(rng);
				a_ref->RemoveItem(rule.oldForm, itemCount, RE::ITEM_REMOVE_REASON::kRemove, nullptr, nullptr);
				if (thingToAdd->As<RE::TESLeveledList>()) {
					AddLeveledListToContainer(thingToAdd->As<RE::TESLeveledList>(), a_ref);

					if (isContainerUnsafe) {
						this->handledUnsafeContainers[a_ref->formID] = true;
					}
				}
				else {
					a_ref->AddObjectToContainer(thingToAdd, nullptr, itemCount, nullptr);

					if (isContainerUnsafe) {
						this->handledUnsafeContainers[a_ref->formID] = true;
					}
				}
			}
			else {
				uint32_t itemCount = 0;
				int ruleCount = rule.count;

				auto* container = a_ref->GetContainer();
				if (!container) continue;

				container->ForEachContainerObject([&](RE::ContainerObject& a_obj) {
					auto* baseObj = a_obj.obj;
					if (!baseObj) return RE::BSContainer::ForEachResult::kContinue;

					auto inventoryCount = a_ref->GetInventoryCounts().find(baseObj) != a_ref->GetInventoryCounts().end() ?
						a_ref->GetInventoryCounts()[baseObj] : 0;
					if (inventoryCount < 1) return RE::BSContainer::ForEachResult::kContinue;

					auto* leveledBase = baseObj->As<RE::TESLeveledList>();
					if (!leveledBase) {
						bool missingKeyword = false;
						for (auto it = rule.removeKeywords.begin(); it != rule.removeKeywords.end() && !missingKeyword; ++it) {
							if (baseObj->HasKeywordByEditorID(*it)) continue;
							missingKeyword = true;
						}
						if (missingKeyword) return RE::BSContainer::ForEachResult::kContinue;

						if (inventoryCount < ruleCount && ruleCount > 0) {
							itemCount += inventoryCount;
							a_ref->RemoveItem(baseObj, inventoryCount, RE::ITEM_REMOVE_REASON::kRemove, nullptr, nullptr);

							if (isContainerUnsafe) {
								this->handledUnsafeContainers[a_ref->formID] = true;
							}
						}
						else {
							itemCount += ruleCount;
							a_ref->RemoveItem(baseObj, ruleCount, RE::ITEM_REMOVE_REASON::kRemove, nullptr, nullptr);

							if (isContainerUnsafe) {
								this->handledUnsafeContainers[a_ref->formID] = true;
							}
						}
					}
					else {
						itemCount += HandleContainerLeveledList(leveledBase, a_ref, rule.removeKeywords, ruleCount);
					}
					return RE::BSContainer::ForEachResult::kContinue;
					});

				for (uint32_t i = 0; i < itemCount; ++i) {
					size_t rng = clib_util::RNG().generate<size_t>(0, rule.newForm.size() - 1);
					RE::TESBoundObject* thingToAdd = rule.newForm.at(rng);
					auto* leveledThing = thingToAdd->As<RE::TESLeveledList>();
					if (leveledThing) {
						AddLeveledListToContainer(leveledThing, a_ref);
					}
					else {
						a_ref->AddObjectToContainer(thingToAdd, nullptr, itemCount, nullptr);
					}
				}
			}
		} //Replace Rule reading end.

		for (auto& rule : this->removeRules) {
			if (!rule.bypassSafeEdits && isContainerUnsafe) continue;
			int32_t ruleCount = rule.count;

			if (rule.removeKeywords.empty()) {
				auto itemCount = a_ref->GetInventoryCounts()[rule.oldForm];
				if (itemCount < 1) continue;
				if (!IsRuleValid(&rule, a_ref)) continue;

				if (ruleCount > itemCount || ruleCount < 1) {
					ruleCount = itemCount;
				}
				a_ref->RemoveItem(rule.oldForm, ruleCount, RE::ITEM_REMOVE_REASON::kRemove, nullptr, nullptr);

				if (isContainerUnsafe) {
					this->handledUnsafeContainers[a_ref->formID] = true;
				}
			}
			else {
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

						auto itemCount = a_ref->GetInventoryCounts()[baseObj];
						if (itemCount > rule.count && ruleCount > 0) {
							itemCount = rule.count;
						}
						a_ref->RemoveItem(baseObj, itemCount, RE::ITEM_REMOVE_REASON::kRemove, nullptr, nullptr);

						if (isContainerUnsafe) {
							this->handledUnsafeContainers[a_ref->formID] = true;
						}
					}
					else {
						HandleContainerLeveledList(leveledBase, a_ref, rule.removeKeywords, ruleCount);
					}

					return RE::BSContainer::ForEachResult::kContinue;
					});
			}
		} //Remove rule reading end.

		for (auto& rule : this->addRules) {
			if (!rule.bypassSafeEdits && isContainerUnsafe) continue;
			if (!IsRuleValid(&rule, a_ref)) continue;
			int32_t ruleCount = rule.count;
			if (rule.count < 1) {
				ruleCount = 1;
			}

			size_t rng = clib_util::RNG().generate<size_t>(0, rule.newForm.size() - 1);
			RE::TESBoundObject* thingToAdd = rule.newForm.at(rng);
			auto* leveledThing = thingToAdd->As<RE::TESLeveledList>();

			if (leveledThing) {
				AddLeveledListToContainer(leveledThing, a_ref);
			}
			else {
				a_ref->AddObjectToContainer(thingToAdd, nullptr, ruleCount, nullptr);
			}

			if (isContainerUnsafe) {
				this->handledUnsafeContainers[a_ref->formID] = true;
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

		auto* longGS = RE::GameSettingCollection::GetSingleton()->GetSetting("iHoursToRespawnCellCleared");
		uint32_t longDelay = longGS->GetUInt() / 24;
		auto* shortGS = RE::GameSettingCollection::GetSingleton()->GetSetting("iHoursToRespawnCell");
		uint32_t shortDelay = shortGS->GetUInt() / 24;
		this->fResetDaysShort = shortDelay;
		this->fResetDaysLong = longDelay;
	}

	void ContainerManager::RegisterInMap(RE::TESObjectREFR* a_ref, bool a_cleared, float a_resetTime, bool a_unsafeContainer) {
		if (!a_unsafeContainer) {
			auto newVal = std::pair<bool, float>(a_cleared, a_resetTime);
			this->handledContainers[a_ref->formID] = newVal;
		}
		else {
			this->handledUnsafeContainers[a_ref->formID] = false;
		}
	}
}