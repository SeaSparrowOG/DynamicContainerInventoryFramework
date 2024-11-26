#include "Hooks/hooks.h"

#include "utilities/utilities.h"

namespace Hooks {
	void Install()
	{
		SKSE::AllocTrampoline(28);
		ContainerManager::Install();
	}

	void ContainerManager::Install()
	{
		auto& trampoline = SKSE::GetTrampoline();
		REL::Relocation<std::uintptr_t> initializeTarget{ REL::ID(19507), 0x78C };
		REL::Relocation<std::uintptr_t> resetTarget{ REL::ID(19802), 0x12B };

		_initialize = trampoline.write_call<5>(initializeTarget.address(), Initialize);
		_reset = trampoline.write_call<5>(resetTarget.address(), Reset);
	}

	void ContainerManager::Initialize(RE::TESObjectREFR* a_container, bool a3)
	{
		_initialize(a_container, a3);
		if (a_container && a_container->GetBaseObject() && a_container->GetBaseObject()->As<RE::TESObjectCONT>()) {

		}
	}

	void ContainerManager::Reset(RE::TESObjectREFR* a_container, bool a3)
	{
		_reset(a_container, a3);
		if (a_container && a_container->GetBaseObject() && a_container->GetBaseObject()->As<RE::TESObjectCONT>()) {

		}
	}
}