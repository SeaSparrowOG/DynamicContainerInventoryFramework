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
}

namespace ContainerManager {
	bool ContainerManager::IsRuleValid(SwapRule* a_rule, RE::TESObjectREFR* a_ref) {
		auto* refLoc = a_ref->GetCurrentLocation();

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
				if (std::find(settingsParents.begin(), settingsParents.end(), parent) != settingsParents.end()) {
					hasParentLocation = true;
				}
				parent = parent->parentLoc;
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

		return (!HasRuleApplied(a_ref) &&
			hasReferenceMatch && hasLocationKeywordMatch && hasParentLocation && hasContainerMatch);
	}

	bool ContainerManager::HasRuleApplied(RE::TESObjectREFR* a_ref) {
		if (this->handledContainers.find(a_ref) != this->handledContainers.end()) {
			auto& dayAttached = this->handledContainers[a_ref];
			if (RE::Calendar::GetSingleton()->GetDaysPassed() > dayAttached + 10.0f) return false;
			return true;
		}
		return false;
	}

	void ContainerManager::CreateSwapRule(SwapRule a_rule) {
		if (!a_rule.oldForm) {
			this->addRules.push_back(a_rule);
			_loggerInfo("Registered bew <Add> rule.");
			_loggerInfo("    Form(s) that can be added:");
			for (auto form : a_rule.newForm) {
				_loggerInfo("        >{}.", form->GetName());
			}
			_loggerInfo("    Ammount to be added: {}.", a_rule.count);
			_loggerInfo("----------------------------------------------");
		}
		else if (a_rule.newForm.empty()) {
			this->removeRules.push_back(a_rule);

			_loggerInfo("Registered bew <Remove> rule.");
			_loggerInfo("    Form that will be removed: {}.", a_rule.oldForm->GetName());
			_loggerInfo("    Ammount to be removed: {}.", a_rule.count);
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

	void ContainerManager::HandleContainer(RE::TESObjectREFR* a_ref) {
		bool isVendorContainer = false;
		auto* ownerFaction = a_ref->GetFactionOwner();
		if (ownerFaction && ownerFaction->IsVendor()) {
			isVendorContainer = true;
		}
		std::vector<std::string> appliedRules;

		for (auto& rule : this->replaceRules) {
			if (isVendorContainer && !rule.distributeToVendors) continue;
			auto itemCount = a_ref->GetInventoryCounts()[rule.oldForm];
			if (itemCount < 1) continue;
			if (!IsRuleValid(&rule, a_ref)) continue;

			size_t rng = clib_util::RNG().generate<size_t>(0, rule.newForm.size() - 1);
			RE::TESBoundObject* thingToAdd = rule.newForm.at(rng);
			a_ref->RemoveItem(rule.oldForm, itemCount, RE::ITEM_REMOVE_REASON::kRemove, nullptr, nullptr);
			a_ref->AddObjectToContainer(thingToAdd, nullptr, itemCount, nullptr);

			appliedRules.push_back(rule.ruleName);
		} //Replace Rule reading end.

		for (auto& rule : this->removeRules) {
			if (isVendorContainer && !rule.distributeToVendors) continue;
			auto itemCount = a_ref->GetInventoryCounts()[rule.oldForm];
			if (itemCount < 1) continue;
			if (!IsRuleValid(&rule, a_ref)) continue;

			int32_t ruleCount = rule.count;
			if (ruleCount > itemCount) {
				ruleCount = itemCount;
			}
			a_ref->RemoveItem(rule.oldForm, ruleCount, RE::ITEM_REMOVE_REASON::kRemove, nullptr, nullptr);

			appliedRules.push_back(rule.ruleName);
		} //Remove rule reading end.

		for (auto& rule : this->addRules) {
			if (isVendorContainer && !rule.distributeToVendors) continue;
			if (!IsRuleValid(&rule, a_ref)) continue;

			int32_t ruleCount = rule.count;
			size_t rng = clib_util::RNG().generate<size_t>(0, rule.newForm.size() - 1);
			RE::TESBoundObject* thingToAdd = rule.newForm.at(rng);
			a_ref->AddObjectToContainer(thingToAdd, nullptr, ruleCount, nullptr);

			appliedRules.push_back(rule.ruleName);
		} //Add rule reading end.

		float daysPassed = RE::Calendar::GetSingleton()->GetDaysPassed();
		for (auto rule : appliedRules) {
			this->handledContainers[a_ref] = daysPassed;
		}
	}

	void ContainerManager::LoadMap(SKSE::SerializationInterface* a_intfc) {
		std::uint32_t type;
		std::uint32_t size;
		std::uint32_t version;

		while (a_intfc->GetNextRecordInfo(type, size, version)) {
			if (type == _byteswap_ulong('MAPR')) {
				std::size_t recordSize;
				a_intfc->ReadRecordData(&recordSize, sizeof(recordSize));

				for (; recordSize > 0; --recordSize) {
					RE::FormID refBuffer = 0;
					a_intfc->ReadRecordData(&refBuffer, sizeof(refBuffer));
					RE::FormID newRef = 0;
					if (a_intfc->ResolveFormID(refBuffer, newRef)) {
						float dayAttached = -1.0f;
						a_intfc->ReadRecordData(&dayAttached, sizeof(dayAttached));
						if (!dayAttached) {
							auto* foundForm = RE::TESForm::LookupByID(newRef);
							auto* foundReference = foundForm ? foundForm->As<RE::TESObjectREFR>() : nullptr;
							if (foundReference) {
								_loggerInfo("Creating entry {}:\n    >{}\n    >{}.", recordSize, foundReference->GetBaseObject()->GetName(), dayAttached);
								this->handledContainers[foundReference] = dayAttached;
							}
						}
					}
				}
				_loggerInfo("__________________________________________________");
			}
		}
	}

	void ContainerManager::SaveMap(SKSE::SerializationInterface* a_intfc) {
		if (a_intfc->OpenRecord(_byteswap_ulong('MAPR'), 0)) {
			auto size = this->handledContainers.size();
			a_intfc->WriteRecordData(&size, sizeof(size));
			for (auto& container : this->handledContainers) {
				a_intfc->WriteRecordData(&container.first->formID, sizeof(container.first->formID));
				a_intfc->WriteRecordData(&container.second, sizeof(container.second));
			}
		}
	}

	void ContainerManager::LoadParentLocations() {
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
	}

	void SaveCallback(SKSE::SerializationInterface* a_intfc) {
		ContainerManager::GetSingleton()->SaveMap(a_intfc);
	}

	void LoadCallback(SKSE::SerializationInterface* a_intfc) {

		ContainerManager::GetSingleton()->LoadMap(a_intfc);
	}
}