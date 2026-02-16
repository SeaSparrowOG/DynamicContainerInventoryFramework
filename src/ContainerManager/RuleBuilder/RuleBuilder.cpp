#include "RuleBuilder.h"

namespace ContainerManager::RuleBuilder
{
	void Builder::AddChanges(const Json::Value& a_changes)
	{
		const auto& addField = a_changes["add"];
		const auto& removeField = a_changes["remove"];
		const auto& removeByKeywordsField = a_changes["removeByKeywords"];
		if (!addField && !removeField && !removeByKeywordsField) {
			errors.missingRequiredFields.push_back("Both Add, RemoveByKeywords and Remove fields are missing.");
			valid = false;
			return;
		}
	}
}