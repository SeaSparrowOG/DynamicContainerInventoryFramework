#pragma once

namespace Cache
{
	class MapMarkerCache : public REX::Singleton<MapMarkerCache>
	{
	public:
		bool Initialize();
		RE::BGSLocation* GetNearestLocation(RE::TESObjectREFR* a_container);

	private:
		std::unordered_map<RE::FormID, RE::FormID>              markerToLocationMap{};
		std::unordered_map<RE::FormID, std::vector<RE::FormID>> worldspaceToMapMarkersMap{};
	};
}