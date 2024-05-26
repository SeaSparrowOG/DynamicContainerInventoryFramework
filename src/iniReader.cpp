#include "iniReader.h"
#include "containerManager.h"

namespace INISettings {
	bool ShouldRebuildINI(CSimpleIniA* a_ini) {
		const char* section = "General";
		const char* keys[] = { "fMaxRefLookupDistance" };
		int sectionLength = sizeof(keys) / sizeof(keys[0]);
		std::list<CSimpleIniA::Entry> keyHolder;

		a_ini->GetAllKeys(section, keyHolder);
		if (std::size(keyHolder) != sectionLength) return true;
		for (auto* key : keys) {
			if (!a_ini->KeyExists(section, key)) return true;
		}
		return false;
	}

	bool BuildINI() {
		std::filesystem::path f{ "./Data/SKSE/Plugins/ContainerDistributionFramework.ini" };
		bool createEntries = false;
		if (!std::filesystem::exists(f)) {
			std::fstream createdINI;
			createdINI.open(f, std::ios::out);
			createdINI.close();
			createEntries = true;
		}

		CSimpleIniA ini;
		ini.SetUnicode();
		ini.LoadFile(f.c_str());
		if (!createEntries) { createEntries = ShouldRebuildINI(&ini); }

		if (createEntries) {
			ini.Delete("General", NULL);
			ini.SetDoubleValue("General", "fMaxRefLookupDistance", 25000.0, ";If a reference does not have a location specified, search this distance for a marker with a location to substitute.");
			ini.SaveFile(f.c_str());
		}

		double range = ini.GetDoubleValue("General", "fMaxRefLookupDistance", 25000.0);
		if (range < 0.0) range = 0.0;
		if (range > 150000.0) range = 150000.0;

		ContainerManager::ContainerManager::GetSingleton()->fMaxLookupRadius = range;
		return true;
	}
}