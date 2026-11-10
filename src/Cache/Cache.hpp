#pragma once

namespace Cache {
    class CachedData final : 
        public REX::TSingleton<CachedData>
    {
    public:
        [[nodiscard]]
        const std::vector<RE::FormID>* GetWorldMarkers(RE::TESWorldSpace* world) const noexcept;

        [[nodiscard]] bool Initialize();

    private:
        std::unordered_set<RE::FormID> _vendorFactions{};
        std::unordered_set<RE::FormID> _vendorContainers{};
        std::unordered_map<RE::FormID, std::vector<RE::FormID>> _markerCache{};
    };

    [[nodiscard]]
    inline bool InitializeCache() {
        REX::INFO("Initializing cache..."sv);
        auto* cache = CachedData::GetSingleton();
        if (!cache) {
            REX::CRITICAL("  - Failed to retrieve internal cache singleotn."sv);
            return false;
        }
        return cache->Initialize();
    }

    [[nodiscard]]
    inline const std::vector<RE::FormID>* GetWorldMarkers(RE::TESWorldSpace* world) noexcept {
        static auto* cache = CachedData::GetSingleton();
        if (!cache) {
            return nullptr;
        }
        return cache->GetWorldMarkers(world);
    }
}