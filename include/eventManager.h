#pragma once

namespace Events {
#define continueEvent RE::BSEventNotifyControl::kContinue

	class ContainerLoadedEvent :
		public RE::BSTEventSink<RE::TESCellAttachDetachEvent>,
		public ISingleton<ContainerLoadedEvent> {
	public:
		bool RegisterListener();
	private:
		RE::BSEventNotifyControl ProcessEvent(const RE::TESCellAttachDetachEvent* a_event, RE::BSTEventSource<RE::TESCellAttachDetachEvent>* a_eventSource) override;
	};

	class LocationClearedEvent :
		public RE::BSTEventSink<RE::TESTrackedStatsEvent>,
		public ISingleton<LocationClearedEvent> {
	public:
		bool RegisterListener();
	private:
		RE::BSEventNotifyControl ProcessEvent(const RE::TESTrackedStatsEvent* a_event, RE::BSTEventSource<RE::TESTrackedStatsEvent>* a_eventSource) override;
	};
}