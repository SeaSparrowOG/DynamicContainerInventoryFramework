#include "INISettings.h"

#include "hooks/hooks.h"

#include <SimpleIni.h>

namespace Settings::INI
{
	void Read()
	{
		std::filesystem::path f{ "./Data/SKSE/Plugins/ContainerDistributionFramework.ini" };
		if (!std::filesystem::exists(f)) {
			Hooks::ContainerManager::GetSingleton()->RegisterDistance(25000.0f);
			return;
		}

		::CSimpleIniA ini{};
		ini.SetUnicode();
		ini.LoadFile(fmt::format(R"(.\Data\SKSE\Plugins\{}.ini)", Plugin::NAME).c_str());

		if (ini.KeyExists("General", "fMaxRefLookupDistance")) {
			float newDistance = static_cast<float>(ini.GetDoubleValue("General", "fMaxRefLookupDistance", 25000.0f));
			Hooks::ContainerManager::GetSingleton()->RegisterDistance(newDistance);
		}
		else {
			Hooks::ContainerManager::GetSingleton()->RegisterDistance(25000.0f);
		}
	}
}