#include "referenceCondition.h"

namespace Conditions
{
	bool ReferenceCondition::IsValid(RE::TESObjectREFR* a_container)
	{
		const auto containerID = a_container->formID;
		for (const auto referenceID : validReferences) {
			if (referenceID == containerID) {
				return !inverted;
			}
		}
		return inverted;
	}

	ReferenceCondition::ReferenceCondition(std::vector<RE::FormID> a_references)
	{
		this->validReferences = a_references;
	}

	void ReferenceCondition::Print()
	{
		REX::INFO("=========================/");
		REX::INFO("|  Reference Conditions /");
		REX::INFO("=======================/");
		for (const auto& form : validReferences) {
			REX::INFO("  ->{}{:08X}", inverted ? "Not " : "", form);
		}
		REX::INFO("");
	}
}