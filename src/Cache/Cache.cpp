#include "Cache.hpp"

namespace Cache {
    const std::vector<RE::FormID>* CachedData::GetWorldMarkers(RE::TESWorldSpace *world) const noexcept {
        if (!world) {
            return nullptr;
        }

        auto it = _markerCache.find(world->GetFormID());
        if (it == _markerCache.end()) {
            return nullptr;
        }
        return &it->second;
    }

    bool CachedData::Initialize() {
        auto* dh = RE::TESDataHandler::GetSingleton();
        if (!dh) {
            REX::CRITICAL("  - Failed to retrieve the game's internal Data Handler."sv);
            return false;
        }

        REX::INFO("  - Initializing vendor container cache..."sv);
        const auto& factions = dh->GetFormArray<RE::TESFaction>();
        if (factions.empty()) {
            REX::WARN("    >Factions array is empty - this is likely a mistake."sv);
        }
        else {
            for (const auto* faction : factions) {
                const auto* vendorContainer = faction ? faction->vendorData.merchantContainer : nullptr;
                if (!vendorContainer) {
                    continue;
                }
                _vendorFactions.insert(faction->GetFormID());
                _vendorContainers.insert(vendorContainer->GetFormID());
            }
            REX::INFO("    >Cached {} factions and {} containers."sv, _vendorFactions.size(), _vendorContainers.size());
        }

        REX::INFO("  - Initializing map marker cache..."sv);
        size_t cachedMarkers{0u};
        std::unordered_map<RE::FormID, std::vector<RE::FormID>> worldspaceMarkers{};
        const auto& worldspaces = dh->GetFormArray<RE::TESWorldSpace>();
        if (worldspaces.empty()) {
            REX::WARN("    >Worldspace array is empty - this is likely a mistake."sv);
        }
        else {
            for (const auto* world : worldspaces) {
                const auto* persistentCell = world ? world->persistentCell : nullptr;
                if (!persistentCell) {
                    continue;
                }
                std::vector<RE::FormID> markers{};
                markers.reserve(20u);
                persistentCell->ForEachReference([&](const RE::TESObjectREFR* ref){
                    if (ref && ref->extraList.HasType<RE::ExtraMapMarker>()) {
                        markers.emplace_back(ref->GetFormID());
                        ++cachedMarkers;
                    }
                    return RE::BSContainer::ForEachResult::kContinue;
                });
                const auto worldID = world->GetFormID();
                _markerCache.emplace(worldID, std::move(markers));
            }
            REX::INFO("    >Finished caching {} worldspaces with a total of {} map markers."sv, _markerCache.size(), cachedMarkers);
        }
        return true;
    }
}