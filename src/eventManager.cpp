#include "eventManager.h"
#include "containerManager.h"

namespace Events {
	bool ContainerLoadedEvent::RegisterListener() {
		auto* singleton = ContainerLoadedEvent::GetSingleton();
		if (!singleton) return false;
		RE::ScriptEventSourceHolder::GetSingleton()->AddEventSink(singleton);
		return true;
	}

	RE::BSEventNotifyControl ContainerLoadedEvent::ProcessEvent(const RE::TESCellAttachDetachEvent* a_event, RE::BSTEventSource<RE::TESCellAttachDetachEvent>* a_eventSource) {
		if (!(a_event && a_eventSource)) return continueEvent;
		if (!a_event->attached) return continueEvent;
		auto* eventReference = a_event->reference.get();
		bool  eventIsContainer = eventReference ? eventReference->HasContainer() : false;
		if (!eventIsContainer) return continueEvent;

		ContainerManager::ContainerManager::GetSingleton()->HandleContainer(eventReference);
		return continueEvent;
	}
}