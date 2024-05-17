#include "Hooks.h"
#include "containerManager.h"

namespace Hooks {
    void Install() noexcept {
        stl::write_vfunc<RE::TESObjectREFR, InitItemImpl>();
    }

    void InitItemImpl::Thunk(RE::TESObjectREFR* a_ref) noexcept {
        func(a_ref);

        if (a_ref->HasContainer()) {
            ContainerManager::ContainerManager::GetSingleton()->HandleContainer(a_ref);
        }
    }
}