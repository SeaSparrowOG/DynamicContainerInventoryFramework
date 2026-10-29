#pragma once

#include "Cache/Cache.hpp"
#include "Settings/INI/INISettings.hpp"

namespace Container::Conditions {
    
    class ConditionParam {
    public:
        bool noReset =  false;
        bool vendorChest = false;
        RE::BGSLocation*   location = nullptr;
        RE::TESObjectREFR* ref = nullptr;
        RE::TESWorldSpace* world = nullptr;
        RE::TESObjectCELL* cell = nullptr;
        RE::TESBoundObject* base = nullptr;

        RE::LOCK_LEVEL lock;

        std::unordered_set<RE::BGSKeyword*> locKeywords{};

        std::unordered_set<RE::BGSLocation*> locParents{};
        std::unordered_set<RE::TESWorldSpace*> worldParents{};

        ConditionParam(RE::TESObjectREFR* from) noexcept {
            assert(from);
            lock = from->GetLockLevel();
            base = from->GetBaseObject();
            cell = from->GetParentCell();
            if (cell) {
                world = cell->worldSpace;
                auto* parent = world ? world->parentWorld : nullptr;
                while (parent) {
                    worldParents.insert(parent);
                    parent = parent->parentWorld;
                }
            }

            location = from->GetCurrentLocation();
            if (!location && world) {
                static float maxDist = Settings::INI::GetSetting<float>("General|fMaxRefLookupDistance", 25000.0f);
                const auto pos = ref->GetPosition();
                const auto& markers = Cache::GetWorldMarkers(world);
                float lastDistance = maxDist;
                RE::TESObjectREFR* closestMarker = nullptr;
                for (const auto& id : markers) {
                    auto* markerRef = RE::TESForm::LookupByID<RE::TESObjectREFR>(id);
                    if (!markerRef) {
                        continue;
                    }

                    float dist = pos.GetDistance(markerRef->GetPosition());
                    if (dist > lastDistance) {
                        continue;
                    }

                    lastDistance = dist;
                    closestMarker = markerRef;
                }
                if (closestMarker) {
                    location = closestMarker->GetCurrentLocation();
                }
            }
            if (location) {
                auto* parent = location->parentLoc;
                while (parent) {
                    for (auto* keyword : std::span(parent->keywords, parent->numKeywords)) {
                        if (!keyword) {
                            continue;
                        }
                        locKeywords.insert(keyword);
                    }
                    locParents.insert(parent);
                    parent = parent->parentLoc;
                }
            }            
        }
    };

    class Condition {
        
    };
}