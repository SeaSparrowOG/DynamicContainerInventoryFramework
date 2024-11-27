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
		logger::info("=========================/");
		logger::info("|  Reference Conditions /");
		logger::info("=======================/");
		for (const auto& form : validReferences) {
			logger::info("  ->{}{}", inverted ? "Not " : "", form);
		}
		logger::info("");
	}
}