#pragma once

namespace Cache {
    class CachedData final : 
        public REX::TSingleton<CachedData>
    {
    public:
        [[nodiscard]]
        const std::vector<RE::FormID>& GetWorldMarkers(RE::TESWorldSpace* world) const noexcept;

    private:
        std::unordered_map<RE::FormID, std::vector<RE::FormID>> _markerCache{};
    };

    [[nodiscard]]
    inline const std::vector<RE::FormID>& GetWorldMarkers(RE::TESWorldSpace* world) noexcept {
        static auto* cache = CachedData::GetSingleton();
        if (!cache) {
            return {};
        }
        return cache->GetWorldMarkers(world);
    }
}