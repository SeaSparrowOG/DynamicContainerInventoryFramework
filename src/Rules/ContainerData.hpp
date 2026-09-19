#pragma once

namespace Rules
{
    using ContainerDeltas = std::unordered_map<RE::TESObject*, int>;

    struct ContainerData
    {
        ContainerDeltas                      deltas{};
        RE::TESObjectREFR*                   container = nullptr;
        RE::TESObjectREFR::InventoryCountMap counts;

        ContainerData(RE::TESObjectREFR* ref);
    };
}