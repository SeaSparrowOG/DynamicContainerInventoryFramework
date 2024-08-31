#include "containerManager.h"
#include "merchantFactionCache.h"
#include "utility.h"

namespace {
	static void AddLeveledListToContainer(RE::TESLeveledList* list, RE::TESObjectREFR* a_container, uint32_t a_count) {
		RE::BSScrapArray<RE::CALCED_OBJECT> result{};
		Utility::ResolveLeveledList(list, &result, a_count);
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
		if (!a_rule->questCondition.IsValid()) return false;

		//Reference check.
		bool hasReferenceMatch = a_rule->references.empty() ? true : false;
		if (!hasReferenceMatch) {
			if (std::find(a_rule->references.begin(), a_rule->references.end(), a_ref->formID) != a_rule->references.end()) {
				hasReferenceMatch = true;
			}
		}
		if (!hasReferenceMatch) return false;

		//Container check.
		bool hasContainerMatch = a_rule->container.empty() ? true : false;
		auto* refBaseContainer = a_ref->GetBaseObject()->As<RE::TESObjectCONT>();
		if (!hasContainerMatch && refBaseContainer) {
			for (auto it = a_rule->container.begin(); it != a_rule->container.end() && !hasContainerMatch; ++it) {
				if (refBaseContainer == *it) hasContainerMatch = true;
			}
		}
		if (!hasContainerMatch) return false;

		bool hasAVMatch = a_rule->requiredAVs.empty() ? true : false;
		if (!hasAVMatch) {
			auto* player = RE::PlayerCharacter::GetSingleton();
			bool hasFullMatch = false;

			for (auto it = a_rule->requiredAVs.begin(); it != a_rule->requiredAVs.end() && !hasAVMatch; ++it) {
				auto& valueSet = *it;
				if (player->GetActorValue(valueSet.first) >= valueSet.second.first) {
					hasFullMatch = true;
				}
				if (hasFullMatch && valueSet.second.second > -1.0f) {
					if (player->GetActorValue(valueSet.first) > valueSet.second.second) {
						hasFullMatch = false;
					}
				}
				hasAVMatch = hasFullMatch;
			}
		}
		if (!hasAVMatch) return false;

		//We can get lazy here, since this is the last condition.
		bool hasGlobalMatch = a_rule->requiredGlobalValues.empty() ? true : false;
		if (!hasGlobalMatch) {
			for (auto it = a_rule->requiredGlobalValues.begin(); it != a_rule->requiredGlobalValues.end() && !hasGlobalMatch; ++it) {
				if ((*it).first->value == (*it).second) hasGlobalMatch = true;
			}
		}
		if (!hasGlobalMatch) return false;

		auto* refLoc = a_ref->GetCurrentLocation();
		auto* refWorldspace = a_ref->GetWorldspace();

		bool hasWorldspaceLocation = a_rule->validWorldspaces.empty() ? true : false;
		if (!hasWorldspaceLocation && refWorldspace) {
			if (std::find(a_rule->validWorldspaces.begin(), a_rule->validWorldspaces.end(), refWorldspace) != a_rule->validWorldspaces.end()) {
				hasWorldspaceLocation = true;
			}
		}

		//The following are SLOW because they have a lot to check.
		//Thus, they are moved to the end of the validation to give the first a chance to fail early.
		//Parent Location check. If the current location is NOT a match, this finds its parents.
		bool hasParentLocation = a_rule->validLocations.empty() ? true : false;
		if (!hasParentLocation && refLoc) {
			if (std::find(a_rule->validLocations.begin(), a_rule->validLocations.end(), refLoc) != a_rule->validLocations.end()) {
				hasParentLocation = true;
			}

			//Check parents.
			auto settingsParents = this->parentLocations.find(refLoc) != this->parentLocations.end() ? this->parentLocations[refLoc] : std::vector<RE::BGSLocation*>();
			RE::BGSLocation* parent = refLoc->parentLoc;
			if (!hasParentLocation) {
				for (auto it = settingsParents.begin(); it != settingsParents.end() && !hasParentLocation && parent; ++it) {
					if (std::find(a_rule->validLocations.begin(), a_rule->validLocations.end(), parent) != a_rule->validLocations.end()) {
						hasParentLocation = true;
					}
					parent = parent->parentLoc;
				}
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
		if (!hasParentLocation) return false;

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
		if (!hasLocationKeywordMatch) return false;

		bool hasParentWorldspaceMatch = a_rule->validWorldspaces.empty() ? true : false;
		if (!hasParentWorldspaceMatch) {
			auto* refWorldSpace = a_ref->GetWorldspace();

			for (auto* ruleWorld : a_rule->validWorldspaces) {
				if (refWorldSpace == ruleWorld) return true;
			}
		}
		if (!hasParentWorldspaceMatch) return false;
		
		return true;
	}

	void ContainerManager::CreateSwapRule(SwapRule a_rule) {
		enum type {
			kAdd,
			kRemove,
			kReplace
		};
		type ruleType = kAdd;
		bool removeByKeywirds = false;

		if (a_rule.removeKeywords.empty()) {
			if (!a_rule.oldForm) {
				this->addRules.push_back(a_rule);
			}
			else if (a_rule.newForm.empty()) {
				this->removeRules.push_back(a_rule);
				ruleType = kRemove;
			}
			else {
				this->replaceRules.push_back(a_rule);
				ruleType = kReplace;
			}
		}
		else {
			removeByKeywirds = true;
			if (!a_rule.newForm.empty()) {
				this->replaceRules.push_back(a_rule);
				ruleType = kReplace;
			}
			else {
				this->removeRules.push_back(a_rule);
				ruleType = kRemove;
			}
		}

		_loggerInfo("Registered new rule of type: {}.", ruleType == kAdd ? "Add" : ruleType == kRemove ? "Remove" : "Replace");
		_loggerInfo("    Rule name: {}", a_rule.ruleName);
		if (ruleType == kRemove && removeByKeywirds) {
			_loggerInfo("    Forms with these keywords will be removed:");
			for (auto& keyword : a_rule.removeKeywords) {
				_loggerInfo("        >{}", keyword);
			}
		}
		else if (ruleType == kRemove) {
			_loggerInfo("    This form will be removed: {}", strcmp(a_rule.oldForm->GetName(), "") == 0 ?
				clib_util::editorID::get_editorID(a_rule.oldForm).empty() ? std::to_string(a_rule.oldForm->GetLocalFormID()) :
				clib_util::editorID::get_editorID(a_rule.oldForm)
				: a_rule.oldForm->GetName());
			_loggerInfo("    Count: {}", a_rule.count > 0 ? std::to_string(a_rule.count) : "All");
		}
		else if (ruleType == kReplace && removeByKeywirds) {
			_loggerInfo("    Forms with these keywords will be removed:");
			for (auto& keyword : a_rule.removeKeywords) {
				_loggerInfo("        >{}", keyword);
			}

			_loggerInfo("    And replaced by any of these:");
			for (auto* form : a_rule.newForm) {
				_loggerInfo("        >{}", strcmp(form->GetName(), "") == 0 ?
					clib_util::editorID::get_editorID(form).empty() ? std::to_string(form->GetLocalFormID()) :
					clib_util::editorID::get_editorID(form)
					: form->GetName());
			}
		}
		else if (ruleType == kReplace) {
			_loggerInfo("    This form will be removed: {}", strcmp(a_rule.oldForm->GetName(), "") == 0 ?
				clib_util::editorID::get_editorID(a_rule.oldForm).empty() ? std::to_string(a_rule.oldForm->GetLocalFormID()) :
				clib_util::editorID::get_editorID(a_rule.oldForm)
				: a_rule.oldForm->GetName());

			_loggerInfo("    And replaced by any of these:");
			for (auto* form : a_rule.newForm) {
				_loggerInfo("        >{}", strcmp(form->GetName(), "") == 0 ?
					clib_util::editorID::get_editorID(form).empty() ? std::to_string(form->GetLocalFormID()) :
					clib_util::editorID::get_editorID(form)
					: form->GetName());
			}
		}
		else {
			_loggerInfo("    Any of the following forms may be added, with a count of {}:", a_rule.count > 1 ? a_rule.count : 1);
			for (auto* form : a_rule.newForm) {
				_loggerInfo("        >{}", strcmp(form->GetName(), "") == 0 ?
					clib_util::editorID::get_editorID(form).empty() ? std::to_string(form->GetLocalFormID()) :
					clib_util::editorID::get_editorID(form)
					: form->GetName());
			}
		}

		if (a_rule.bypassSafeEdits) {
			_loggerInfo("");
			_loggerInfo("    This rule can distribute to safe containers.");
		}

		if (a_rule.allowVendors) {
			_loggerInfo("");
			_loggerInfo("    This rule can distribute to vendor containers.");
		}

		if (!a_rule.container.empty()) {
			_loggerInfo("");
			_loggerInfo("    This rule will only apply to these containers:");
			for (auto* it : a_rule.container) {
				_loggerInfo("        >{}", strcmp(it->GetName(), "") == 0 ?
					clib_util::editorID::get_editorID(it).empty() ? std::to_string(it->GetLocalFormID()) :
					clib_util::editorID::get_editorID(it)
					: it->GetName());
			}
		}
		if (!a_rule.validLocations.empty()) {
			_loggerInfo("");
			_loggerInfo("    This rule will only apply to these locations:");
			for (auto* it : a_rule.validLocations) {
				_loggerInfo("        >{}", strcmp(it->GetName(), "") == 0 ?
					clib_util::editorID::get_editorID(it).empty() ? std::to_string(it->GetLocalFormID()) :
					clib_util::editorID::get_editorID(it)
					: it->GetName());
			}
		}
		if (!a_rule.validWorldspaces.empty()) {
			_loggerInfo("");
			_loggerInfo("    This rule will only apply to these worldspaces:");
			for (auto* it : a_rule.validWorldspaces) {
				_loggerInfo("        >{}", strcmp(it->GetName(), "") == 0 ?
					clib_util::editorID::get_editorID(it).empty() ? std::to_string(it->GetLocalFormID()) :
					clib_util::editorID::get_editorID(it)
					: it->GetName());
			}
		}
		if (!a_rule.locationKeywords.empty()) {
			_loggerInfo("");
			_loggerInfo("    This rule will only apply to locations with these keywords:");
			for (auto& it : a_rule.locationKeywords) {
				_loggerInfo("        >{}", it);
			}
		}
		if (!a_rule.references.empty()) {
			_loggerInfo("");
			_loggerInfo("    This rule will only apply to these references:");
			for (auto reference : a_rule.references) {
				_loggerInfo("        >{}", reference);
			}
		}
		if (!a_rule.requiredGlobalValues.empty()) {
			_loggerInfo("");
			_loggerInfo("    This rule will only apply if these globals have these values:");
			for (auto pair : a_rule.requiredGlobalValues) {
				_loggerInfo("        >Global: {}, Value: {}", pair.first->GetFormEditorID(), pair.second);
			}
		}
		if (a_rule.randomAdd) {
			_loggerInfo("");
			_loggerInfo("    This rule will randomly add a random item in the add field.");
		}
		if (!a_rule.questCondition.questEDID.empty()) {
			_loggerInfo("");
			_loggerInfo("    This rule will only apply if certain quest conditions are met.");
		}
		if (!a_rule.requiredAVs.empty()) {
			_loggerInfo("");
			_loggerInfo("    This rule will only apply while these actor values are all valid");
			for (auto& pair : a_rule.requiredAVs) {
				std::string valueName = "";
				switch (pair.first) {
				case RE::ActorValue::kIllusion:
					valueName = "Illusion";
					break;
				case RE::ActorValue::kConjuration:
					valueName = "Conjuration";
					break;
				case RE::ActorValue::kDestruction:
					valueName = "Destruction";
					break;
				case RE::ActorValue::kRestoration:
					valueName = "Restoration";
					break;
				case RE::ActorValue::kAlteration:
					valueName = "Ateration";
					break;
				case RE::ActorValue::kEnchanting:
					valueName = "Enchanting";
					break;
				case RE::ActorValue::kSmithing:
					valueName = "Smithing";
					break;
				case RE::ActorValue::kHeavyArmor:
					valueName = "Heavy Armor";
					break;
				case RE::ActorValue::kBlock:
					valueName = "Block";
					break;
				case RE::ActorValue::kTwoHanded:
					valueName = "Two Handed";
					break;
				case RE::ActorValue::kOneHanded:
					valueName = "One Handed";
					break;
				case RE::ActorValue::kArchery:
					valueName = "Archery";
					break;
				case RE::ActorValue::kLightArmor:
					valueName = "Light Armor";
					break;
				case RE::ActorValue::kSneak:
					valueName = "Sneak";
					break;
				case RE::ActorValue::kLockpicking:
					valueName = "Lockpicking";
					break;
				case RE::ActorValue::kPickpocket:
					valueName = "Pickpocket";
					break;
				case RE::ActorValue::kSpeech:
					valueName = "Speech";
					break;
				default:
					valueName = "Alchemy";
					break;
				}

				_loggerInfo("        >Value: {} -> Min Level: {}, Max Level: {}", valueName, 
					pair.second.first, 
					pair.second.second > -1.0f ? std::to_string(pair.second.second) : "MAX");
			}
		}
		_loggerInfo("-------------------------------------------------------------------------------------");
	}

	void ContainerManager::HandleContainer(RE::TESObjectREFR* a_ref) {
#ifdef DEBUG
		auto then = std::chrono::high_resolution_clock::now();
		size_t rulesApplied = 0;
#endif 
		bool merchantContainer = a_ref->GetFactionOwner() ? a_ref->GetFactionOwner()->IsVendor() : false;
		if (!merchantContainer) {
			merchantContainer = MerchantCache::MerchantCache::GetSingleton()->IsMerchantContainer(a_ref);
		}

		auto* containerBase = a_ref->GetBaseObject() ? a_ref->GetBaseObject()->As<RE::TESObjectCONT>() : nullptr;
		bool isContainerUnsafe = false;
		if (containerBase && !(containerBase->data.flags & RE::CONT_DATA::Flag::kRespawn)) {
			isContainerUnsafe = true;
		}

		bool hasParentCell = a_ref->parentCell ? true : false;
		auto* parentEncounterZone = hasParentCell ? a_ref->parentCell->extraList.GetEncounterZone() : nullptr;
		if (parentEncounterZone && parentEncounterZone->data.flags & RE::ENCOUNTER_ZONE_DATA::Flag::kNeverResets) {
			isContainerUnsafe = true;
		}

		auto inventoryCounts = a_ref->GetInventoryCounts();
		for (auto& rule : this->replaceRules) {
			if (merchantContainer && !rule.allowVendors) continue;
			if (!merchantContainer && rule.onlyVendors) continue;
			if (!rule.bypassSafeEdits && isContainerUnsafe) continue;
			if (!IsRuleValid(&rule, a_ref)) continue;

			if (rule.removeKeywords.empty()) {
				auto itemCount = a_ref->GetInventoryCounts()[rule.oldForm];
				if (itemCount < 1) continue;

				a_ref->RemoveItem(rule.oldForm, itemCount, RE::ITEM_REMOVE_REASON::kRemove, nullptr, nullptr);

				if (rule.randomAdd) {
					size_t index = clib_util::RNG().generate<size_t>(0, rule.newForm.size() - 1);
					auto* thingToAdd = rule.newForm.at(index);
					auto* leveledThing = thingToAdd->As<RE::TESLeveledList>();
					if (leveledThing) {
						AddLeveledListToContainer(leveledThing, a_ref, itemCount);
					}
					else {
						a_ref->AddObjectToContainer(thingToAdd, nullptr, itemCount, nullptr);
					}
				}
				else {
					for (auto* obj : rule.newForm) {
						auto leveledThing = obj->As<RE::TESLeveledList>();
						if (leveledThing) {
#ifdef DEBUG
							_loggerDebug("Adding leveled list to container:");
#endif
							AddLeveledListToContainer(leveledThing, a_ref, itemCount);
						}
						else {
							a_ref->AddObjectToContainer(obj, nullptr, itemCount, nullptr);
						}
					}
				}
#ifdef DEBUG
				rulesApplied++;
#endif
			}
			else {
				uint32_t itemCount = 0;
				for (auto& pair : inventoryCounts) {
					auto* item = pair.first;
					bool missingKeyword = false;
					for (auto it = rule.removeKeywords.begin(); it != rule.removeKeywords.end() && !missingKeyword; ++it) {
						if (item->HasKeywordByEditorID(*it)) continue;
						missingKeyword = true;
					}
					if (missingKeyword) continue;

					itemCount += pair.second;
					a_ref->RemoveItem(item, pair.second, RE::ITEM_REMOVE_REASON::kRemove, nullptr, nullptr);
				}

				if (rule.randomAdd) {
					size_t index = clib_util::RNG().generate<size_t>(0, rule.newForm.size() - 1);
					auto* thingToAdd = rule.newForm.at(index);
					auto* leveledThing = thingToAdd->As<RE::TESLeveledList>();
					if (leveledThing) {
						AddLeveledListToContainer(leveledThing, a_ref, itemCount);
					}
					else {
						a_ref->AddObjectToContainer(thingToAdd, nullptr, itemCount, nullptr);
					}
				}
				else {
					for (auto* obj : rule.newForm) {
						auto leveledThing = obj->As<RE::TESLeveledList>();
						if (leveledThing) {
#ifdef DEBUG
							_loggerDebug("Adding leveled list to container:");
#endif
							AddLeveledListToContainer(leveledThing, a_ref, itemCount);
						}
						else {
							a_ref->AddObjectToContainer(obj, nullptr, itemCount, nullptr);
						}
					}
				}
#ifdef DEBUG
				rulesApplied++;
#endif
			}
		} //Replace Rule reading end.

		for (auto& rule : this->removeRules) {
			if (merchantContainer && !rule.allowVendors) continue;
			if (!merchantContainer && rule.onlyVendors) continue;
			if (!rule.bypassSafeEdits && isContainerUnsafe) continue;
			int32_t ruleCount = rule.count;

			if (rule.removeKeywords.empty()) {
				auto itemCount = a_ref->GetInventory()[rule.oldForm].first;
				if (itemCount < 1) continue;
				if (!IsRuleValid(&rule, a_ref)) continue;

				if (ruleCount > itemCount || ruleCount < 1) {
					ruleCount = itemCount;
				}
				a_ref->RemoveItem(rule.oldForm, ruleCount, RE::ITEM_REMOVE_REASON::kRemove, nullptr, nullptr);
#ifdef DEBUG
				rulesApplied++;
#endif
			}
			else {
				for (auto& pair : inventoryCounts) {
					auto* item = pair.first;
					bool missingKeyword = false;
					for (auto it = rule.removeKeywords.begin(); it != rule.removeKeywords.end() && !missingKeyword; ++it) {
						if (item->HasKeywordByEditorID(*it)) continue;
						missingKeyword = true;
					}
					if (missingKeyword) continue;

					a_ref->RemoveItem(item, pair.second, RE::ITEM_REMOVE_REASON::kRemove, nullptr, nullptr);
				}
#ifdef DEBUG
				rulesApplied++;
#endif
			}
		} //Remove rule reading end.

		for (auto& rule : this->addRules) {
			if (merchantContainer && !rule.allowVendors) continue;
			if (!merchantContainer && rule.onlyVendors) continue;
			if (!rule.bypassSafeEdits && isContainerUnsafe) continue;
			if (!IsRuleValid(&rule, a_ref)) continue;
			int32_t ruleCount = rule.count;
			if (rule.count < 1) {
				ruleCount = 1;
			}

			if (rule.randomAdd) {
				size_t index = clib_util::RNG().generate<size_t>(0, rule.newForm.size() - 1);
				auto* thingToAdd = rule.newForm.at(index);
				auto* leveledThing = thingToAdd->As<RE::TESLeveledList>();
				if (leveledThing) {
					AddLeveledListToContainer(leveledThing, a_ref, ruleCount);
				}
				else {
					a_ref->AddObjectToContainer(thingToAdd, nullptr, ruleCount, nullptr);
				}
			}
			else {
				for (auto* obj : rule.newForm) {
					auto leveledThing = obj->As<RE::TESLeveledList>();
					if (leveledThing) {
#ifdef DEBUG
						_loggerDebug("Adding leveled list to container:");
#endif
						AddLeveledListToContainer(leveledThing, a_ref, ruleCount);
					}
					else {
						a_ref->AddObjectToContainer(obj, nullptr, ruleCount, nullptr);
					}
				}
			}
#ifdef DEBUG
			rulesApplied++;
#endif
		} //Add rule reading end.

#ifdef DEBUG
		auto now = std::chrono::high_resolution_clock::now();
		auto elapsed = std::chrono::duration_cast<std::chrono::nanoseconds>(now - then).count();
		if (rulesApplied > 0) {
			_loggerDebug("Finished processing container: {}.\n    Applied {} rules in {} nanoseconds.", _debugEDID(containerBase), rulesApplied, elapsed);
		}
#endif
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
			Utility::GetParentChain(parentLocation, &parents);
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

	void ContainerManager::ContainerManager::SetMaxLookupRange(float a_range) {
		if (a_range < 2500.0f) {
			a_range = 2500.0f;
		}
		else if (a_range > 50000.0f) {
			a_range = 50000.0f;
		}

		this->fMaxLookupRadius = a_range;
	}
}