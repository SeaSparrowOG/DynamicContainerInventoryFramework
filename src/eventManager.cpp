#include "eventManager.h"
#include "containerManager.h"

namespace Events {
	bool ContainerLoadedEvent::RegisterListener() {
		auto* singleton = ContainerLoadedEvent::GetSingleton();
		if (!singleton) return false;
		RE::ScriptEventSourceHolder::GetSingleton()->AddEventSink(singleton);
		return true;
	}

	bool LocationClearedEvent::RegisterListener() {
		auto* singleton = LocationClearedEvent::GetSingleton();
		if (!singleton) return false;
		RE::ScriptEventSourceHolder::GetSingleton()->AddEventSink(singleton);
		return true;
	}

	RE::BSEventNotifyControl ContainerLoadedEvent::ProcessEvent(const RE::TESCellAttachDetachEvent* a_event, RE::BSTEventSource<RE::TESCellAttachDetachEvent>* a_eventSource) {
		if (!(a_event && a_eventSource)) return continueEvent;
		if (!a_event->attached) return continueEvent;
		auto* eventReference = a_event->reference.get();
		bool  eventIsContainer = eventReference ? eventReference->GetBaseObject()->As<RE::TESObjectCONT>() : false;
		if (!eventIsContainer) return continueEvent;

		ContainerManager::ContainerManager::GetSingleton()->HandleContainer(eventReference);
		return continueEvent;
	}

	RE::BSEventNotifyControl Events::LocationClearedEvent::ProcessEvent(const RE::TESTrackedStatsEvent* a_event, RE::BSTEventSource<RE::TESTrackedStatsEvent>* a_eventSource) {
		if (!(a_event && a_eventSource)) return continueEvent;
		if (a_event->stat != "Dungeons Cleared") return continueEvent;

		float daysPassed = RE::Calendar::GetSingleton()->GetDaysPassed();
		auto* containerManagerSingleton = ContainerManager::ContainerManager::GetSingleton();
		auto& handledContainers = containerManagerSingleton->handledContainers;

		for (auto& pair : handledContainers) {
			auto* pairLoc = pair.first->GetCurrentLocation();
			if (!pairLoc) continue;
			if (pairLoc->IsCleared() && !pair.second.first && pair.second.second > daysPassed) {
				pair.second.first = true;
				pair.second.second += containerManagerSingleton->fResetDaysLong - containerManagerSingleton->fResetDaysShort;
			}
		}
		return RE::BSEventNotifyControl::kContinue;
	}
}