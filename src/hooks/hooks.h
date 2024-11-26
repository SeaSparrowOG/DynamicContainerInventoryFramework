#pragma once

#include "conditions/condition.h"
#include "conditions/actorValueCondition.h"
#include "conditions/containerCondition.h"
#include "conditions/globalCondition.h"
#include "conditions/locationCondition.h"
#include "conditions/locationKeywordCondition.h"
#include "conditions/questCondition.h"
#include "conditions/referenceCondition.h"
#include "conditions/worldspaceCondition.h"
#include "utilities/utilities.h"

namespace Hooks {
	void Install();

	class ContainerManager : public Utilities::Singleton::ISingleton<ContainerManager> 
	{
	public:
		static void Install();

	private:
		static void Initialize(RE::TESObjectREFR* a_container, bool a3);
		static void Reset(RE::TESObjectREFR* a_container, bool a3);

		inline static REL::Relocation<decltype(&Initialize)> _initialize;
		inline static REL::Relocation<decltype(&Reset)> _reset;
	};
}