#include "Condition.hpp"

namespace Container::Conditions
{
    std::expected<ConditionContainer, ConditionFail> create_condition(const Json::Value &val) noexcept {
        (void)val;
        auto container = ConditionContainer{};

        container.Finalize();
        return container;
    }

    void ConditionFail::Log(const std::string& prefix) const noexcept {
        (void)prefix;
    }

    void ConditionContainer::Finalize() noexcept {
        if (_finalized) {
            return;
        }

        std::ranges::sort(_conditions, [](const auto& lhs, const auto& rhs) {
            return lhs->GetType() < rhs->GetType();
        });

        _finalized = true;
    }

    void ConditionContainer::AddCondition(Condition_ptr &condition) noexcept {
        if (_finalized) {
            return;
        }
        _conditions.emplace_back(std::move(condition));
    }
    ConditionParam::ConditionParam(RE::TESObjectREFR *from) noexcept
    {
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
            const auto* markers = Cache::GetWorldMarkers(world);
            if (markers) {
                float lastDistance = maxDist;
                RE::TESObjectREFR* closestMarker = nullptr;
                for (const auto& id : *markers) {
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
}