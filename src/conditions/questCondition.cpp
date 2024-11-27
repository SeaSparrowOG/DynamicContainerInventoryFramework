#include "questCondition.h"

#include "RE/misc.h"

namespace Conditions
{
	bool QuestCondition::IsValid(RE::TESObjectREFR* a_container)
	{
		(void)a_container;
		if (inverted) {
			if (state == kCompleted && !quest->IsCompleted()) {
				for (const auto& stage : completedStages) {
					if (RE::IsQuestStageDone(quest, stage)) {
						return false;
					}
				}
				return true;
			}
			else if (state == kOngoing && quest->IsCompleted()) {
				for (const auto& stage : completedStages) {
					if (RE::IsQuestStageDone(quest, stage)) {
						return false;
					}
				}
				return true;
			}
		}
		else {
			if (state == kCompleted && quest->IsCompleted()) {
				for (const auto& stage : completedStages) {
					if (!RE::IsQuestStageDone(quest, stage)) {
						return false;
					}
				}
				return true;
			}
			else if (state == kOngoing && !quest->IsCompleted()) {
				for (const auto& stage : completedStages) {
					if (!RE::IsQuestStageDone(quest, stage)) {
						return false;
					}
				}
				return true;
			}
		}
		return false;
	}

	QuestCondition::QuestCondition(RE::TESQuest* a_quest, std::vector<uint16_t> a_stages, bool a_completed)
	{
		this->quest = a_quest;
		this->completedStages = a_stages;
		if (a_completed) {
			this->state = kCompleted;
		}
		else {
			this->state = kOngoing;
		}
	}
}