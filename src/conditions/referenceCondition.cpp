#include "referenceCondition.h"

namespace Conditions
{
	bool ReferenceCondition::IsValid(RE::TESObjectREFR* a_container)
	{
		const auto containerID = a_container->formID;
		for (const auto referenceID : validReferences) {
			if (referenceID == containerID) {
				return true;
			}
		}
		return false;
	}

	ReferenceCondition::ReferenceCondition(std::vector<RE::FormID> a_references)
	{
		this->validReferences = a_references;
	}
}