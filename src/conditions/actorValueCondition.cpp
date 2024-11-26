#include "actorValueCondition.h"

namespace Conditions
{
	bool AVCondition::IsValid(RE::TESObjectREFR* a_container) 
	{
		(void)a_container;
		const auto player = RE::PlayerCharacter::GetSingleton();
		assert(player);
		if (player && player->GetActorValue(value) >= minValue) {
			return !inverted;
		}
		return inverted;
	}

	AVCondition::AVCondition(RE::ActorValue a_value, float a_minValue)
	{
		this->value = a_value;
		this->minValue = a_minValue;
	}
}