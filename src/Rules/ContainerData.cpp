#include "ContainerData.hpp"

namespace Rules
{
    ContainerData::ContainerData(RE::TESObjectREFR *ref)
    {
        container = ref;
        counts = ref->GetInventoryCounts();
    }
}