#pragma once

namespace Settings {
#define configPath R"(Data/SKSE/Plugins/COIN/)"
	enum ChangeType {
		ADD,
		REMOVE,
		REPLACE
	};

	bool ReadSettings();
}