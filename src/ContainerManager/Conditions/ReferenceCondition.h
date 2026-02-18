#pragma once

#include "ContainerManager/ContainerManager.h"

namespace ContainerManager
{
	class ReferenceCondition : public Condition
	{
	public:
		bool IsValid(const ConditionCheckParams& a_params) const override;
		ReferenceCondition(const std::vector<RE::FormID>& a_references, bool a_negate);

	private:
		bool negate{ false };
	};
}