#pragma once

#include "condition.h"

namespace Conditions
{
	class QuestCondition : public Condition
	{
	public:
		bool IsValid(RE::TESObjectREFR* a_container) override;

		QuestCondition(RE::TESQuest* a_quest, std::vector<uint16_t> a_stages, bool a_completed);

	private:
		enum QuestState {
			kCompleted,
			kOngoing
		};

		QuestState state;
		std::vector<uint16_t> completedStages;
		RE::TESQuest* quest;
	};
}