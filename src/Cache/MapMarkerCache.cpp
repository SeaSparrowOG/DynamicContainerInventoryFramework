#include "MapMarkerCache.h"

#include "Settings/INI/INISettings.h"

namespace Cache
{
	bool MapMarkerCache::Initialize()
	{
		return true;
	}

	RE::BGSLocation* MapMarkerCache::GetNearestLocation(RE::TESObjectREFR* a_container)
	{
		if (!a_container) {
			return nullptr;
		}
		auto worldspace = a_container->GetWorldspace();
		if (!worldspace) {
			return nullptr;
		}

		auto it = worldspaceToMapMarkersMap.find(worldspace->GetFormID());
		if (it == worldspaceToMapMarkersMap.end()) {
			return nullptr;
		}

		RE::BGSLocation* nearestLocation = nullptr;
		auto containerPos = a_container->GetPosition();
		float nearestDistance = Settings::INI::GetSetting<float>(Settings::INI::GENERAL_LOOKUP_RANGE).value_or(150000.0f);
		auto& mapMarkers = (*it).second;
		if (mapMarkers.empty()) {
			return nearestLocation;
		}

		auto locationMapEnd = markerToLocationMap.end();
		for (const auto& markerFormID : mapMarkers) {
			auto markerIt = markerToLocationMap.find(markerFormID);
			if (markerIt == locationMapEnd) {
				continue;
			}

			auto location = RE::TESForm::LookupByID<RE::BGSLocation>(markerIt->second);
			if (!location) {
				continue;
			}

			auto markerRef = RE::TESForm::LookupByID<RE::TESObjectREFR>(markerFormID);
			if (!markerRef) {
				continue;
			}

			auto markerPos = markerRef->GetPosition();
			float distance = containerPos.GetDistance(markerPos);
			if (distance < nearestDistance) {
				nearestDistance = distance;
				nearestLocation = location;
			}
		}
		return nearestLocation;
	}
}