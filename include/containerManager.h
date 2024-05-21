#pragma once
#include "settings.h"

namespace ContainerManager {
	struct SwapRule {
		int                              count;
		Settings::ChangeType             changeType;
		RE::TESBoundObject*              oldForm { nullptr };
		std::vector<std::string>         locationKeywords;
		std::vector<RE::BGSLocation*>    validLocations;
		std::vector<RE::TESBoundObject*> newForm;
	};

	class ContainerManager : public clib_util::singleton::ISingleton<ContainerManager> {
	public:
		bool CreateSwapRule(SwapRule a_rule);
		void HandleContainer(RE::TESObjectREFR* a_ref);

	private:
		std::vector<SwapRule> rules;
	};
}