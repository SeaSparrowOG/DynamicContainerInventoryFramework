#include "iniReader.h"
#include "containerManager.h"

namespace INISettings {
	bool ShouldRebuildINI(CSimpleIniA* a_ini) {
		const char* section = "General";
		const char* keys[] = { "fMaxRefLookupDistance", "fResetDays" };
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
			ini.SetDoubleValue("General", "fResetDays", 30.0, ";The framework does not constantly distribute to containers. After a container is evaluated, it is only evaluated again after this many days.");
			ini.SaveFile(f.c_str());
		}

		double range = ini.GetDoubleValue("General", "fMaxRefLookupDistance", 25000.0);
		double reset = ini.GetDoubleValue("General", "fResetDays", 30.0);
		if (range < 0.0) range = 0.0;
		if (range > 150000.0) range = 150000.0;
		if (reset < 10.0) reset = 10.0f;
		if (reset > 90.0) reset = 90.0;

		ContainerManager::ContainerManager::GetSingleton()->fMaxLookupRadius = range;
		ContainerManager::ContainerManager::GetSingleton()->fResetDays = reset;
		return true;
	}
}