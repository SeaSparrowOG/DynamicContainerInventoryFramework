#pragma once

#include "condition.h"

namespace Conditions
{
	class QuestCondition : public Condition
	{
	public:
		bool IsValid(RE::TESObjectREFR* a_container) override;

	private:
		enum QuestState {
			kCompleted,
			kOngoing,
			kIgnored
		};

		QuestState state;
		uint16_t stageMax;
		uint16_t stageMin;
		RE::TESQuest quest;
	};
}